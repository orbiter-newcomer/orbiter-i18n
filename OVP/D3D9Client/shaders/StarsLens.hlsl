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
	float beta = Const.fParams.x;
	float3 vdir = Const.vVelDir.xyz;

	float c  = dot(d, vdir);
	float c2 = (c - beta) / max(1.0f - beta * c, 1e-6f);
	c2 = clamp(c2, -1.0f, 1.0f);

	float3 perp = d - vdir * c;
	float pl = length(perp);
	perp = (pl > 1e-6f) ? perp / pl : float3(0.0f, 0.0f, 0.0f);
	return normalize(vdir * c2 + perp * sqrt(max(0.0f, 1.0f - c2 * c2)));
}

// Schwarzschild light deflection angle: post-Newtonian weak field blended into
// the logarithmic strong field divergence near the photon sphere.
// b_c = 3*sqrt(3)*mu is the critical (photon sphere) impact parameter.
float LensDeflectionSchw(float mu, float b)
{
	const float BC_PER_MU = 5.19615242f;	// 3*sqrt(3)
	float bc = BC_PER_MU * mu;
	float x = max(b / bc, 1.0001f);

	float u = mu / b;
	float aPN = 4.0f*u + 15.0f*3.14159265f*0.25f*u*u + 128.0f/3.0f*u*u*u;

	// Bozza (2008) strong field leading term plus a linear correction. The
	// correction coefficient is tuned so that the logarithmic strong field
	// branch meets the post-Newtonian expansion at the blend window ends
	// (a mismatch there folds the mapping and shows up as radial spikes).
	float aS = -log(x - 1.0f) - 0.4016f + 0.825f*(x - 1.0f);

	// wide smooth blend from strong (x<1.2) to weak (x>1.8) field
	float w = saturate((x - 1.2f) * (1.6f));
	w = w*w*(3.0f - 2.0f*w);
	return max(lerp(aS, aPN, w), 0.0f);
}

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

	float mu = Const.fFrame.x;
	float r  = Const.fFrame.y;
	if (mu <= 1e-6f || r <= 1.0f) return d;

	float3 inward = Const.vFrameDir.xyz;		// observer -> source
	float3 er = -inward;						// outward radial
	float rs = 2.0f * mu;
	float f = 1.0f - rs / r;

	// Model tier: fParams.w = 0 -> weak field (1PN), 1 -> strong field (Kerr).
	// The weak-field tier uses the first order frame factor far from the body,
	// but near the horizon the first order expansion is invalid (it would keep
	// sqrt(1-rs/r) ~ 0.5 while the exact value tends to 0) and the mapping
	// tears. Upgrade to the exact expression inside the strong-field zone.
	bool bStrong = (Const.fParams.w > 0.5f);
	bool bNear   = (r < 10.0f * rs);	// strong-field neighbourhood
	bool bExact  = bStrong || bNear;

	float fs;
	if (bExact) {
		if (f <= 1e-4f) { shadow = 1.0f; return d; }	// inside/at the horizon
		fs = sqrt(f);
	}
	else {
		fs = max(1.0f - rs / (2.0f * r), 1e-3f);	// weak field: sqrt(1-rs/r) ~ 1-rs/(2r)
	}

	float cosA = dot(d, er);
	float sin2 = max(0.0f, 1.0f - cosA * cosA);
	float sinA = sqrt(sin2);

	// Photon-sphere capture test (black hole shadow). The shadow is a real
	// physical feature, so it is also rendered for the weak-field tier in the
	// strong-field neighbourhood (bExact = Kerr tier or r < 10 rs). Farther
	// out the weak-field tier keeps the plain approximation (no capture).
	if (bExact) {
		float b = r * sinA / fs;
		float bc = 5.19615242f * mu;
		if (cosA < 0.0f && b < bc) { shadow = 1.0f; return d; }
	}

	// local frame -> coordinate frame direction (with numerical limiters)
	float denom = sqrt(max(sin2 + cosA * cosA * f, 1e-9f));
	if (denom < 1e-6f) return d;
	float cosG = clamp(cosA * fs / denom, -1.0f, 1.0f);
	float sinG = sqrt(max(0.0f, 1.0f - cosG * cosG));
	float3 eperp = d - er * cosA;
	float pl = length(eperp);
	if (pl > 1e-6f) eperp = eperp / pl;
	else eperp = float3(0.0f, 0.0f, 0.0f);
	return normalize(er * cosG + eperp * sinG);
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
	float4 sp = Const.vFrameSpin;
	if (sp.w <= 1e-6f) return d;

	float mu = Const.fFrame.x;
	float r  = Const.fFrame.y;
	if (mu <= 1e-6f || r <= 1.0f) return d;

	float aM = min(sp.w, 1.0f);
	float q = min(mu / r, 0.999f);
	float cs = dot(d, sp.xyz);

	// Model tier: weak field (1PN) uses the first order Lense-Thirring term;
	// the strong field (Kerr) tier enhances it by 1/(1-rs/r) near the horizon.
	float dphi;
	if (Const.fParams.w > 0.5f) {
		float f = max(1.0f - 2.0f * mu / r, 1e-3f);
		dphi = 2.0f * aM * q * q * cs / f;
	}
	else {
		dphi = 2.0f * aM * q * q * cs;
	}
	dphi = clamp(dphi, -1.0f, 1.0f);
	if (abs(dphi) < 1e-7f) return d;

	float ca = cos(dphi);
	float sa = sin(dphi);
	float3 axis = sp.xyz;
	return normalize(d * ca + cross(axis, d) * sa + axis * dot(axis, d) * (1.0f - ca));
}

// single inverse mapping step: bent direction -> source direction
// Only the 4 most significant bodies are mapped (pixel shader 3.0 instruction
// budget); the remaining bodies contribute a negligible deflection.
float3 LensInverseOnce(float3 d, int n, bool bKerr)
{
	[loop] for (int i = 0; i < 4; i++) {
		if (i >= n) continue;

		float4 ld = Const.vLens[i];
		float dst = ld.w;
		float mu = GetLensMu(i);
		if (dst < 1.0f || mu < 1e-6f) continue;

		// Only rays whose line of sight heads towards the body are deflected.
		// For the opposite half of the sky the starlight approaches the observer
		// from behind and never passes near the body; without this test the
		// weak-field formula diverges exactly behind the observer (b -> 0) and
		// applies a huge fake deflection there.
		if (dot(d, ld.xyz) <= 0.0f) continue;

		float cosA = dot(d, ld.xyz);
		float sinA = sqrt(max(1e-8f, 1.0f - cosA * cosA));
		float b = dst * sinA;
		if (b < 1e-3f) b = 1e-3f;

		// The plain 4*mu/b weak-field deflection diverges as b -> 0 (rays
		// passing close to the photon sphere), which folds the inverse mapping
		// into radial streaks. Use the finite strong-field expression near the
		// body (within ~20 mu ~ 10 rs) and keep the weak-field formula farther
		// out.
		bool bNear = (dst < 20.0f * mu);
		float alpha;
		if (bKerr || bNear) alpha = LensDeflectionSchw(mu, b);
		else                alpha = 4.0f * mu / b;
		if (!(alpha >= 0.0f)) alpha = 0.0f;		// NaN/negative guard
		alpha = min(alpha, 1.2f);					// limiter (rad)

		// First order Kerr spin correction (weak field limit): the deflection
		// is asymmetric about the spin axis (spin-orbit coupling). The term is
		// proportional to a/M and to the projection of the spin axis on the
		// normal of the (body, ray) plane.
		if (bKerr) {
			float4 sp = Const.vLensSpin[i];
			if (sp.w > 1e-6f) {
				float3 n = normalize(sp.xyz);
				float3 pn = cross(ld.xyz, d);
				float pl = length(pn);
				if (pl > 1e-6f)
					alpha += -4.0f * sp.w * mu / (b * b) * dot(n, pn / pl);
			}
		}

		alpha = min(alpha, 0.8f);		// ~46 deg, keeps the mapping stable
		if (alpha < 0.0f) alpha = 0.0f;
		if (alpha < 1e-12f) continue;

		// Inverse lensing: the observed direction is rotated AWAY from the body
		// by alpha (the body appears displaced towards the source, so the
		// source direction is further away). A proper rotation about the axis
		// perpendicular to the (body, direction) plane is used instead of a
		// tangential translation: the translation is only valid for small
		// alpha and tears the mapping apart in the strong field.
		float3 rot = cross(ld.xyz, d);
		float rl = length(rot);
		float ca = cos(alpha);
		float sa = sin(alpha);
		if (rl > 1e-6f) {
			rot = rot / rl;
			d = d * ca + cross(rot, d) * sa + rot * dot(rot, d) * (1.0f - ca);
		}
		else {
			// direction parallel to the body: rotate away from it about any
			// perpendicular axis
			float3 refv = (abs(ld.xyz.y) < 0.9f) ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
			rot = normalize(cross(ld.xyz, refv));
			d = ld.xyz * ca + cross(rot, ld.xyz) * sa;
		}
		d = normalize(d);
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
	float w = smoothstep(max(Const.fBlend.x, 1e-5f), max(Const.fBlend.y, Const.fBlend.x + 1e-5f), shift);

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
