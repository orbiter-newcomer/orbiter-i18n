// ==============================================================
// Part of the ORBITER VISUALISATION PROJECT (OVP)
// Licensed under LGPL v2
//
// Relativistic shader functions: aberration and gravitational lensing.
// Moved out of the official CelSphere.hlsl into a separate include file.
// Requires: CelDataStruct Const (uniform) to be declared before inclusion.
// ==============================================================

#include "RelativityMapping.hlsl"

// Relativistic aberration: shift a celestial-sphere direction from the rest
// frame to the observer frame.  cos(theta') = (cos(theta)+beta)/(1+beta*cos(theta))
// (all orders in beta; the transverse direction is preserved).
float3 AberrateDir(float3 d)
{
	// shared implementation: RelativityMapping.hlsl
	return RelAberrate(d, Const.vAberr.w, Const.vAberr.xyz);
}

// Gravitational lensing: bend the sampling direction near the celestial
// bodies. The returned direction is the source direction to sample for a given
// observed direction. The weak field model (1PN) uses alpha = 4*mu/b, the Kerr
// model adds the higher order Schwarzschild post-Newtonian terms.
float3 LensDirection(float3 d)
{
	if (Const.fLensCount < 0.5) return d;

	for (int i = 0; i < 12; i++) {
		if (float(i) >= Const.fLensCount) break;

		float4 ld = Const.vLens[i];
		float dst = ld.w;
		float mu = Const.fLensMu[i];
		if (dst < 1.0 || mu < 1e-6) continue;

		float cosA = dot(d, ld.xyz);
		float sinA = sqrt(max(1e-8, 1.0 - cosA * cosA));
		float b = dst * sinA;					// impact parameter [m]
		if (b < 1e-3) b = 1e-3;

		float alpha;
		if (Const.fLensModel > 0.5) {
			// Schwarzschild post-Newtonian expansion (higher order terms)
			float x = mu / b;
			alpha = 4.0*x
			      + 15.0*3.14159265*0.25*x*x
			      + 128.0/3.0*x*x*x
			      + 3465.0*3.14159265/64.0*x*x*x*x;
		}
		else {
			alpha = 4.0 * mu / b;				// 1PN weak field
		}
		alpha = min(alpha, 2.5);				// keep the sampling stable

		if (alpha < 1e-12) continue;

		float3 perp = ld.xyz - d * cosA;
		float pl = length(perp);
		if (pl < 1e-6) continue;
		perp = perp / pl;
		// the source appears shifted towards the body: sample away from it
		d = normalize(d - perp * alpha);
	}
	return d;
}

// ---------------------------------------------------------------------
// Exact gravitational lensing (same math as the lensed cubemap composite):
// Schwarzschild deflection with the strong/weak field blend, applied to all
// significant bodies as a proper rotation. This is the observed -> source
// mapping LensInverseCompat(); the plain star field vertex path inverts it
// numerically, so both paths follow exactly the same mapping and stay
// continuous at the "hole" boundary.
// The Kerr spin correction of the composite version is omitted here (it is a
// small term; the spin-drag rotation is applied separately).
// ---------------------------------------------------------------------
// (LensDeflectionSchw lives in RelativityMapping.hlsl)

float3 LensInverseOnceCompat(float3 d, int n, bool bKerr)
{
	// shared single-body lensing step (RelativityMapping.hlsl). The spin-drag
	// correction of the composite path is omitted for the vertex path (it is a
	// small term and is applied separately as a frame rotation).
	float3 noSpin = float3(0.0f, 1.0f, 0.0f);
	[loop] for (int i = 0; i < 4; i++) {
		if (i >= n) continue;
		float4 ld = Const.vLens[i];
		d = RelLensStep(d, ld.xyz, ld.w, Const.fLensMu[i], bKerr, noSpin, 0.0f);
	}
	return d;
}

// observed direction -> source direction (single damped mapping step)
float3 LensInverseCompat(float3 d)
{
	int n = (int)Const.fLensCount;
	bool bKerr = (Const.fLensModel > 0.5f);
	float3 d0 = d;
	d = normalize(lerp(d0, LensInverseOnceCompat(d0, n, bKerr), 0.75f));
	if (!(dot(d, d) > 0.25f)) d = d0;
	return normalize(d);
}
