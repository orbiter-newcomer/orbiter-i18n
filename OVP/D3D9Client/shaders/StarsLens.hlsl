// =====================================================================
// Gravitational lensing: composite the star field cubemap (source frame)
// onto the screen, applying the observer aberration and the light deflection
// near the celestial bodies.
//
// The cubemap contains the star field as seen in the rest frame (no
// aberration, no lensing), so a screen pixel is resolved back to a source
// direction: observed direction -> inverse aberration -> inverse lensing ->
// cubemap sample. This has no "off-screen data" problem because the cubemap
// covers all directions.
// =====================================================================

#include "RelativityMapping.hlsl"

struct StarsLensConst
{
	float4x4 mVP;			// screen space quad transform
	float4   vTan;			// (tan(fovx/2), tan(fovy/2), 0, 0)
	float4   vCamX;			// camera right   (world frame)
	float4   vCamY;			// camera up      (world frame)
	float4   vCamZ;			// camera forward (world frame)
	float4   vVelDir;		// observer velocity direction (world frame)
	float4   fParams;		// x = beta, y = aberration on, z = lens count, w = lens model
	float4   vLens[12];		// xyz = body direction (world frame), w = distance [m]
	float4   fLensMu[3];	// GM/c^2 of the 12 bodies (packed as 3 float4)
	float4   vLensSpin[12];	// xyz = spin axis (world frame), w = Kerr a/M
	float4   vFrameDir;		// xyz = observer -> dominant source (inward radial, world frame)
	float4   fFrame;		// x = GM/c^2 [m], y = observer distance [m], z = enable flag
	float4   vFrameSpin;	// xyz = dominant source spin axis (world frame), w = Kerr a/M
	float4   fBlend;		// x = blend start [rad], y = blend end [rad], z = brightness gain
	float4   vHoleDisk;		// xyz = dominant lens body direction (world frame), w = disk radius [rad]
	float4   vCharge;		// x = charge effect on, y = q = r_Q/r_s
};

uniform extern StarsLensConst Const;

samplerCUBE tCube;

struct VSInput
{
	float3 pos	: POSITION0;
	float2 tex	: TEXCOORD0;
};

struct VSOutput
{
	float4 posH	: POSITION0;
	float2 tex	: TEXCOORD0;
};

VSOutput VSMain(VSInput v)
{
	VSOutput o = (VSOutput)0;
	o.posH = mul(float4(v.pos, 1.0f), Const.mVP);
	o.tex  = v.tex;
	return o;
}

// equirectangular panorama mapping: direction -> (u,v) texture coordinates
float2 DirToPanoUV(float3 d)
{
	return float2(atan2(d.z, d.x) * (0.5f / 3.14159265f) + 0.5f,
	              acos(clamp(d.y, -1.0f, 1.0f)) / 3.14159265f);
}

float GetLensMu(int i)
{
	int k = i / 4;
	int c = i % 4;
	float4 v = Const.fLensMu[k];
	return (c == 0) ? v.x : ((c == 1) ? v.y : ((c == 2) ? v.z : v.w));
}

// observed (moving observer frame) direction -> rest frame direction
float3 AberrateInverse(float3 d)
{
	// shared implementation: RelativityMapping.hlsl
	return RelAberrateInv(d, Const.fParams.x, Const.vVelDir.xyz);
}

// Schwarzschild light deflection angle: post-Newtonian weak field blended into
// the logarithmic strong field divergence near the photon sphere.
// b_c = 3*sqrt(3)*mu is the critical (photon sphere) impact parameter.
// (LensDeflectionSchw lives in RelativityMapping.hlsl)

// ---------------------------------------------------------------------
// Gravitational aberration: static observer inside the potential well of
// the dominant source. The local (observer frame) direction is squeezed
// when mapped to the coordinate frame:
//     tan(alpha_geom) = tan(alpha_loc) / sqrt(1 - rs/r)
// where alpha is measured from the outward radial direction. Rays captured
// by the photon sphere (inward-going with impact parameter below
// b_c = 3*sqrt(3)*mu) are marked as shadowed (black).
// ---------------------------------------------------------------------
float3 FrameInverse(float3 d, out float shadow)
{
	shadow = 0.0f;
	if (Const.fFrame.z < 0.5f) return d;
	// shared implementation: RelativityMapping.hlsl
	// charge (RN) term: r_Q^2 = q^2 * rs^2 = q^2 * (2 mu)^2
	float rQ2 = (Const.vCharge.x > 0.5f) ?
		(Const.vCharge.y * Const.vCharge.y * 4.0f * Const.fFrame.x * Const.fFrame.x) : 0.0f;
	return RelFrameMap(d, Const.vFrameDir.xyz, Const.fFrame.x, Const.fFrame.y,
	                   (Const.fParams.w > 0.5f), rQ2, shadow);
}

// ---------------------------------------------------------------------
// Kerr frame dragging: the local (ZAMO) frame is dragged around the spin
// axis of the dominant source. Weak/strong-field analytic approximation:
//   dphi = 2 * a/M * (mu/r)^2 * cos(theta)
// where theta is the angle between the ray direction and the spin axis.
// At r ~ 3 rs and a/M = 0.5 this is ~2 degrees, growing towards the horizon.
// ---------------------------------------------------------------------
float3 DragRotate(float3 d)
{
	if (Const.fFrame.z < 0.5f) return d;
	// shared implementation: RelativityMapping.hlsl
	return RelDragRotate(d, Const.vFrameSpin.xyz, Const.vFrameSpin.w,
	                     Const.fFrame.x, Const.fFrame.y, (Const.fParams.w > 0.5f));
}

// single inverse mapping step: bent direction -> source direction
// Only the 4 most significant bodies are mapped (pixel shader 3.0 instruction
// budget); the remaining bodies contribute a negligible deflection.
float3 LensInverseOnce(float3 d, int n, bool bKerr)
{
	// shared single-body lensing step (RelativityMapping.hlsl)
	float qCharge = (Const.vCharge.x > 0.5f) ? Const.vCharge.y : 0.0f;
	[loop] for (int i = 0; i < 4; i++) {
		if (i >= n) continue;
		float4 ld = Const.vLens[i];
		float4 sp = Const.vLensSpin[i];
		d = RelLensStep(d, ld.xyz, ld.w, GetLensMu(i), bKerr, sp.xyz, sp.w, qCharge);
	}
	return d;
}

// bent (rest frame) direction -> source direction
float3 LensInverse(float3 d)
{
	int n = (int)Const.fParams.z;
	bool bKerr = (Const.fParams.w > 0.5f);
	float3 d0 = d;
	// single mapping step with damping: a second full iteration tends to
	// overshoot and fold the mapping in the strong field (sharp spikes).
	d = normalize(lerp(d0, LensInverseOnce(d0, n, bKerr), 0.75f));
	// guard against degenerate directions (NaN / zero vector) produced by the
	// extreme deflection near the photon sphere: fall back to the observed
	// direction, which keeps a valid (if not lensed) sample.
	if (!(dot(d, d) > 0.25f)) d = d0;
	return normalize(d);
}

// Sample the star field cubemap. Near the cubemap face boundaries (where the
// D3D9 hardware filtering is not seamless) a few offset samples are averaged
// so that the face seams fade out instead of cutting hard edges.
float3 SampleCubeSmooth(float3 s)
{
	float3 a = abs(s);
	float mx = max(a.x, max(a.y, a.z));
	float mn = min(a.x, min(a.y, a.z));
	float md = a.x + a.y + a.z - mx - mn;
	float edge = md / max(mx, 1e-6f);		// 1 = on a face boundary

	float3 col = texCUBE(tCube, s).rgb;
	if (edge > 0.85f) {
		// build a stable tangent frame
		float3 refv = (mx == a.x) ? float3(1.0f, 0.0f, 0.0f)
		            : ((mx == a.y) ? float3(0.0f, 1.0f, 0.0f) : float3(0.0f, 0.0f, 1.0f));
		float3 t1 = cross(s, refv);
		float tl = length(t1);
		if (tl > 1e-5f) {
			t1 = t1 / tl;
			float3 t2 = cross(s, t1);
			float d = 0.01f + 0.03f * saturate((edge - 0.85f) * 6.6667f);
			col += texCUBE(tCube, normalize(s + t1 * d)).rgb;
			col += texCUBE(tCube, normalize(s - t1 * d)).rgb;
			col += texCUBE(tCube, normalize(s + t2 * d)).rgb;
			col += texCUBE(tCube, normalize(s - t2 * d)).rgb;
			col *= 0.2f;
		}
	}
	return col;
}

// Forward mapping of the plain star field vertex path (source -> observed).
// The vertex shader (CelSphere.hlsl) solves LensInverse(observed) = source by
// the same fixed-point iteration and limits the displacement to ~5 degrees;
// repeating it here lets the hole criterion track the actual vertex
// displacement of the background tiles. Regions where the vertex path
// stretches/folds the tiles (visible as torn patches near the blend band)
// are then covered by the lensed cubemap.
float3 SourceToObservedVertex(float3 dSrc)
{
	float3 dObs = dSrc;
	[unroll] for (int itV = 0; itV < 3; itV++) {
		float3 dCheck = LensInverse(dObs);		// observed -> source
		dObs = normalize(dObs + (dSrc - dCheck));
	}
	float3 delta = dObs - dSrc;
	float dl = length(delta);
	// same limit as the vertex path (bound to the hole blend parameters;
	// fBlend.w = min(2 deg, LensBlendEnd), fallback 2 deg)
	float maxA = (Const.fBlend.w > 1e-5f) ? Const.fBlend.w : 0.034906585f;
	if (dl > maxA) dObs = normalize(dSrc + delta * (maxA / dl));
	return dObs;
}

float4 PSMain(float2 uv : TEXCOORD0) : COLOR
{
	// camera frame direction for this screen position
	float3 d = normalize(float3((uv.x * 2.0f - 1.0f) * Const.vTan.x,
	                            (1.0f - uv.y * 2.0f) * Const.vTan.y,
	                            1.0f));

	// world frame direction
	float3 dw = d.x * Const.vCamX.xyz + d.y * Const.vCamY.xyz + d.z * Const.vCamZ.xyz;

	// observed -> rest frame (inverse aberration)
	float3 s1 = dw;
	if (Const.fParams.y > 0.5f && Const.fParams.x > 1e-6f)
		s1 = AberrateInverse(dw);

	// rest frame -> coordinate frame (observer inside the gravitational well)
	float shadow = 0.0f;
	float3 s2 = FrameInverse(s1, shadow);
	if (shadow > 0.5f) {
		// capture region (black hole shadow): like any other lensing effect it is
		// only composited where the hole blend is active, so that large blend
		// angles keep the plain star field everywhere.
		float ws = smoothstep(max(Const.fBlend.x, 1e-5f), max(Const.fBlend.y, Const.fBlend.x + 1e-5f), 3.14159f);
		return float4(0.0f, 0.0f, 0.0f, ws);
	}

	// Kerr frame dragging (spin-drag rotation of the local frame)
	s2 = DragRotate(s2);

	// rest frame -> source direction (inverse lensing)
	float3 s = (Const.fParams.z > 0.5f) ? LensInverse(s2) : s2;

	// degenerate direction guard: keep a valid sample
	if (!(dot(s, s) > 0.25f)) s = s2;

	// sample the source frame star field (with a soft blend across the cubemap
	// face seams)
	float3 col = SampleCubeSmooth(s);

	// "hole" blend weight: the apparent shift caused by the gravitational lensing
	// (s2 -> s, excluding the kinematic aberration and the global gravitational
	// aberration/frame compression) decides where the lensed cubemap replaces
	// the plain sharp star field. Shift below fBlend.x keeps the plain star
	// field (alpha 0), above fBlend.y uses the cubemap (alpha 1).
	float3 s2n = normalize(s2);
	float3 sn  = normalize(s);
	float shift = acos(clamp(dot(s2n, sn), -1.0f, 1.0f));

	// second criterion: the displacement the plain star field vertex path
	// would apply to the background tiles at this direction. The blend band
	// (hole boundary) is where the vertex mapping is most stressed; use the
	// larger of the two criteria so that distorted tiles are always covered.
	float3 dV = SourceToObservedVertex(sn);
	float shiftV = acos(clamp(dot(dV, sn), -1.0f, 1.0f));
	shift = max(shift, shiftV);

	float w = smoothstep(max(Const.fBlend.x, 1e-5f), max(Const.fBlend.y, Const.fBlend.x + 1e-5f), shift);

	// minimum-size circular hole: the lensed region always includes a disk of
	// fixed angular radius around the dominant lens body's apparent direction,
	// so the fully lensed core can never degenerate into a small isolated
	// patch (which would pop in/out as an abrupt dark spot). The disk is
	// combined with (not replacing) the displacement criterion above.
	if (Const.vHoleDisk.w > 0.0f) {
		float3 hd = normalize(Const.vHoleDisk.xyz);
		float ang = acos(clamp(dot(normalize(dw), hd), -1.0f, 1.0f));
		float r = Const.vHoleDisk.w;					// disk radius [rad]
		float feather = 0.008726646f;					// 0.5 deg soft edge
		float disk = 1.0f - smoothstep(r - feather, r + feather, ang);
		w = max(w, disk);
	}

	// debug modes (negative brightness):
	// -1: show the raw lens cubemap full screen;
	// -2: show the hole weight map (red = lensed cubemap, black = plain star field);
	// -3: normal compositing with a green boundary line of the blend band drawn on top.
	float zz = (Const.fBlend.z < 0.0f) ? 1.0f : Const.fBlend.z;
	col *= zz;

	if (Const.fBlend.z < -2.5f) {
		// boundary line at the centre of the blend band, one pixel wide
		float mid = 0.5f * (Const.fBlend.x + Const.fBlend.y);
		float grad = length(float2(ddx(shift), ddy(shift)));
		float d = abs(shift - mid);
		float bw = 1.0f - smoothstep(grad * 0.5f, grad * 1.5f, d);
		if (bw > 0.01f) return float4(0.0f, 1.0f, 0.2f, 1.0f);
		return float4(col, w);
	}
	if (Const.fBlend.z < -1.5f) return float4(w, w * 0.25f, 0.0f, 1.0f);
	if (Const.fBlend.z < 0.0f)  return float4(col, 1.0f);

	return float4(col, w);
}
