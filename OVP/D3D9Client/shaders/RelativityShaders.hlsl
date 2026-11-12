// ==============================================================
// Part of the ORBITER VISUALISATION PROJECT (OVP)
// Licensed under LGPL v2
//
// Relativistic shader functions: aberration and gravitational lensing.
// Moved out of the official CelSphere.hlsl into a separate include file.
// Requires: CelDataStruct Const (uniform) to be declared before inclusion.
// ==============================================================

// Relativistic aberration: shift a celestial-sphere direction from the rest
// frame to the observer frame.  cos(theta') = (cos(theta)+beta)/(1+beta*cos(theta))
// (all orders in beta; the transverse direction is preserved).
float3 AberrateDir(float3 d)
{
	float beta = Const.vAberr.w;
	float3 vdir = Const.vAberr.xyz;

	float c  = dot(d, vdir);
	float c2 = (c + beta) / max(1.0 + beta * c, 1e-6);
	c2 = clamp(c2, -1.0, 1.0);

	float3 perp = d - vdir * c;
	float pl = length(perp);
	perp = (pl > 1e-6) ? perp / pl : float3(0.0, 0.0, 0.0);
	return normalize(vdir * c2 + perp * sqrt(max(0.0, 1.0 - c2 * c2)));
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
