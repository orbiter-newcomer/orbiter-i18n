// ==============================================================
// Relativity.hlsl
// Relativistic visual effects post-processing for D3D9Client
//
//   Phase 1:  Doppler shift (red/blue) + relativistic aberration
//
// All maths are the exact special-relativistic expressions:
//   Doppler:    delta = 1 / (gamma * (1 - beta*cos(theta)))
//               frequency ratio; bolometric beaming ~ delta^4;
//               colour from a Planck spectral tilt (T = 6500K * delta)
//   Aberration: cos(theta) = (cos(theta') + beta) / (1 + beta*cos(theta'))
//               applied as an exact inverse boost of the view direction
//               (all orders in beta, not a small-angle approximation)
//
// NOTE: aberration is applied as a screen-space resampling of the already
// rendered image. For a wide rendered FOV the rest-frame direction of edge
// pixels can fall outside the rendered image; the effect is therefore
// smoothly faded out towards the rendered FOV boundary (see EdgeFade) so
// that no streaking/clamping artefacts appear.
//
// Lensing (gravitational light bending) is NOT part of phase 1.
// ==============================================================

sampler2D tBack : register(s0);
sampler2D tDepth : register(s1);  // vessel depth buffer (alpha = camera distance [m])

uniform extern float2 vTan;       // (tan half-FOV x, tan half-FOV y)
uniform extern float3 vVelCam;    // unit velocity direction in camera space
uniform extern float  fBeta;      // |v| / c
uniform extern float  fGamma;     // 1 / sqrt(1 - beta^2)
uniform extern float  fDoppler;   // 0..1 master strength
uniform extern float  fAberr;     // 0..1 master strength
uniform extern float  fExposure;  // observer exposure adaptation white point (0 = disabled)
uniform extern float  fNearGuard; // near-field protection distance [m] (0 = disabled)

#define PI 3.14159265358979

// ---------------------------------------------------------------------
// camera direction for a screen uv
// ---------------------------------------------------------------------
float3 ViewDir(float2 uv)
{
	float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
	return normalize(float3(ndc.x * vTan.x, ndc.y * vTan.y, 1.0));
}

// ---------------------------------------------------------------------
// screen uv for a camera-space direction
// ---------------------------------------------------------------------
float2 ProjUV(float3 d)
{
	float z = max(d.z, 1e-4);
	return float2(0.5 + 0.5 * (d.x / z) / vTan.x,
	              0.5 - 0.5 * (d.y / z) / vTan.y);
}

// ---------------------------------------------------------------------
// Exact inverse aberration boost: observer-frame direction -> rest-frame
// direction.  cos(theta) = (cos(theta') - beta) / (1 - beta*cos(theta'))
// (the component of d perpendicular to the boost direction is preserved)
// ---------------------------------------------------------------------
float3 AberrationInverse(float3 d, float3 vdir, float beta)
{
	float c1 = dot(d, vdir);
	float c0 = (c1 - beta) / max(1.0 - beta * c1, 1e-6);
	c0 = clamp(c0, -1.0, 1.0);
	float3 perp = d - c1 * vdir;
	float pl = length(perp);
	perp = (pl > 1e-5) ? perp / pl : float3(0.0, 0.0, 0.0);
	return normalize(vdir * c0 + perp * sqrt(max(0.0, 1.0 - c0 * c0)));
}

// ---------------------------------------------------------------------
// Doppler colour tilt: ratio of Planck radiance at T = T0*delta and T0,
// evaluated at representative R/G/B wavelengths, luminance-normalised.
// At delta = 1 this is exactly (1,1,1).
// ---------------------------------------------------------------------
float3 DopplerTint(float delta)
{
	float d = clamp(delta, 0.1, 10.0);
	// x = hc/(lambda*k*T0) with T0 = 6500K ; numerator = exp(x)-1
	const float xR = 3.68916;  // lambda = 600 nm
	const float xG = 4.02454;  // lambda = 550 nm
	const float xB = 4.91888;  // lambda = 450 nm
	const float nR = 39.028;   // exp(xR)-1
	const float nG = 54.868;   // exp(xG)-1
	const float nB = 135.546;  // exp(xB)-1
	float3 r = float3(nR / max(exp(xR / d) - 1.0, 1e-6),
	                  nG / max(exp(xG / d) - 1.0, 1e-6),
	                  nB / max(exp(xB / d) - 1.0, 1e-6));
	float lum = dot(r, float3(0.2126, 0.7152, 0.0722));
	r = r / max(lum, 1e-6);
	return clamp(r, 0.05, 20.0);
}

// ---------------------------------------------------------------------
// Fade the aberration towards the rendered FOV boundary: a source
// direction inside the rendered image gets full strength, outside it
// fades to zero (smoothly) to avoid border streaking.
// ---------------------------------------------------------------------
float EdgeFade(float3 dsrc)
{
	if (dsrc.z <= 1e-4) return 0.0;
	float nx = abs((dsrc.x / dsrc.z) / vTan.x);
	float ny = abs((dsrc.y / dsrc.z) / vTan.y);
	float r = max(nx, ny);
	return saturate(1.0 - smoothstep(0.90, 1.05, r));
}

// ---------------------------------------------------------------------
// main
// NOTE: ImageProcessing/IPI.hlsl delivers the screen position as two
// separate TEXCOORD registers (u in TEXCOORD0, v in TEXCOORD1), not as a
// float2. The entry point signature must match that convention.
// ---------------------------------------------------------------------
float4 PSMain(float x : TEXCOORD0, float y : TEXCOORD1) : COLOR
{
	float2 uv = float2(x, y);

	// Near-field protection: pixels belonging to the observer's own vessel (or
	// to the cockpit interior) are at rest with respect to the observer, so
	// they must not receive aberration or Doppler shift. The vessel depth
	// buffer stores the camera distance in its alpha channel (0 = no vessel).
	float nearFade = 1.0;
	if (fNearGuard > 0.0) {
		float dist = tex2D(tDepth, uv).a;
		if (dist > 0.0 && dist < fNearGuard) nearFade = 0.0;
	}
	float fAb  = fAberr * nearFade;
	float fDop = fDoppler * nearFade;

	float3 d = ViewDir(uv);          // observed direction (moving frame)
	float3 ds = d;                   // sampling direction (rest-frame image)

	// ---- aberration: resample the image along the rest-frame direction ----
	if (fAb > 0.0 && fBeta > 1e-6)
	{
		float3 da = AberrationInverse(d, vVelCam, fBeta);
		float fade = EdgeFade(da);
		ds = normalize(lerp(d, da, saturate(fAb * fade)));
	}

	float2 uv2 = clamp(ProjUV(ds), 0.001, 0.999);
	float3 col = tex2D(tBack, uv2).rgb;

	// ---- Doppler: colour tilt + relativistic beaming ----
	if (fDop > 0.0)
	{
		float cosT = dot(d, vVelCam);
		float delta = 1.0 / max(fGamma * (1.0 - fBeta * cosT), 1e-3);
		float3 tint = DopplerTint(delta);
		float beam = clamp(pow(abs(delta), 4.0), 0.02, 64.0);
		col = lerp(col, col * tint * beam, saturate(fDop));

		// ---- observer exposure adaptation ----
		// The beamed (blue-shifted) sky is up to delta^4 times brighter and
		// saturates to white on a fixed-exposure display. A human observer /
		// an auto-exposure camera adapts so the hue stays visible. This keeps
		// the hue ratio and compresses only values above the white point K:
		//   m > K:  m' = m / (1 + (m-K)/K)      (smooth knee, limit -> K)
		// Values below K are untouched. Strongly beamed bright stars still
		// clip to white, as physically expected; dim stars show their hue.
		if (fExposure > 0.0)
		{
			float m = max(max(col.r, col.g), col.b);
			if (m > fExposure) {
				float mOut = m / (1.0 + (m - fExposure) / fExposure);
				col *= mOut / m;
			}
		}
	}

	return float4(col, 1.0);
}
