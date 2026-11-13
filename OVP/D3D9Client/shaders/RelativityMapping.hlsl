// =====================================================================
// Shared relativistic direction-mapping physics (single source of truth).
//
// All functions here are pure: no global constants or uniforms are read,
// every quantity is passed explicitly. Both render paths call these same
// implementations:
//
//   1. the plain star field (vertex path, CelSphere.hlsl):
//        source frame -> aberration -> gravitational aberration -> screen
//   2. the lensed cubemap composite (pixel path, StarsLens.hlsl):
//        screen -> inverse aberration -> frame map -> spin drag -> lensing
//        -> source frame
//
// Physical changes (new effects, tuned coefficients, model tiers) are made
// here only; the two callers are thin assembly layers.
// =====================================================================

// ---------------------------------------------------------------------
// Velocity aberration (special relativity): relativistic direction shift
// between the rest frame and the observer frame.
//   positive: rest frame -> observer frame
//   inverse : observer frame -> rest frame
// cos(theta') = (cos(theta) +/- beta) / (1 +/- beta*cos(theta))
// ---------------------------------------------------------------------
float3 RelAberrate(float3 d, float beta, float3 vdir)
{
	float c  = dot(d, vdir);
	float c2 = (c + beta) / max(1.0 + beta * c, 1e-6);
	c2 = clamp(c2, -1.0, 1.0);

	float3 perp = d - vdir * c;
	float pl = length(perp);
	perp = (pl > 1e-6) ? perp / pl : float3(0.0, 0.0, 0.0);
	return normalize(vdir * c2 + perp * sqrt(max(0.0, 1.0 - c2 * c2)));
}

float3 RelAberrateInv(float3 d, float beta, float3 vdir)
{
	float c  = dot(d, vdir);
	float c2 = (c - beta) / max(1.0 - beta * c, 1e-6);
	c2 = clamp(c2, -1.0, 1.0);

	float3 perp = d - vdir * c;
	float pl = length(perp);
	perp = (pl > 1e-6) ? perp / pl : float3(0.0, 0.0, 0.0);
	return normalize(vdir * c2 + perp * sqrt(max(0.0, 1.0 - c2 * c2)));
}

// ---------------------------------------------------------------------
// Schwarzschild light deflection angle: post-Newtonian weak field blended
// into the logarithmic strong field divergence near the photon sphere.
// b_c = 3*sqrt(3)*mu is the critical (photon sphere) impact parameter.
// ---------------------------------------------------------------------
float RelLensDeflectionSchw(float mu, float b)
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
// Gravitational aberration (static observer inside the potential well of a
// dominant source): the local (observer frame) direction is squeezed when
// mapped to the coordinate frame:
//     tan(alpha_geom) = tan(alpha_loc) / sqrt(1 - rs/r)
// where alpha is measured from the outward radial direction. Rays captured
// by the photon sphere (inward-going with impact parameter below
// b_c = 3*sqrt(3)*mu) are marked as shadowed (black).
//   positive: local frame -> coordinate frame (shadow flag out)
//   inverse : coordinate frame -> local frame
// ---------------------------------------------------------------------
float3 RelFrameMap(float3 d, float3 inwardDir, float mu, float r, bool bStrong, out float shadow)
{
	shadow = 0.0f;
	if (mu <= 1e-6f || r <= 1.0f) return d;

	float3 er = -inwardDir;					// outward radial
	float rs = 2.0f * mu;
	float f = 1.0f - rs / r;

	// Model tier: weak field (1PN) uses the first order frame factor far from
	// the body, but near the horizon the first order expansion is invalid (it
	// would keep sqrt(1-rs/r) ~ 0.5 while the exact value tends to 0) and the
	// mapping tears. Upgrade to the exact expression inside the strong field.
	bool bNear   = (r < 10.0f * rs);		// strong-field neighbourhood
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

float3 RelFrameMapInv(float3 d, float3 inwardDir, float mu, float r)
{
	if (mu <= 1e-6f || r <= 1.0f) return d;
	float rs = 2.0f * mu;
	float f = 1.0f - rs / r;
	if (f <= 1e-4f) return d;			// inside/near the horizon: leave unchanged
	float fs = sqrt(max(f, 1e-6f));
	float3 er = normalize(-inwardDir);	// outward radial
	float cosA = dot(d, er);
	float sin2 = max(0.0f, 1.0f - cosA * cosA);
	float L = sqrt(max(cosA * cosA + sin2 * fs * fs, 1e-9f));
	float cosL = clamp(cosA / L, -1.0f, 1.0f);
	float sinL = sqrt(max(0.0f, 1.0f - cosL * cosL));
	float3 eperp = d - er * cosA;
	float pl = length(eperp);
	if (pl > 1e-6f) eperp = eperp / pl;
	else eperp = float3(0.0f, 0.0f, 0.0f);
	return normalize(er * cosL + eperp * sinL);
}

// ---------------------------------------------------------------------
// Kerr frame dragging: the local (ZAMO) frame is dragged around the spin
// axis of the dominant source. Weak/strong-field analytic approximation:
//   dphi = 2 * a/M * (mu/r)^2 * cos(theta)
// where theta is the angle between the ray direction and the spin axis.
// At r ~ 3 rs and a/M = 0.5 this is ~2 degrees, growing towards the horizon.
// ---------------------------------------------------------------------
float3 RelDragRotate(float3 d, float3 spinAxis, float aM, float mu, float r, bool bStrong)
{
	if (aM <= 1e-6f) return d;
	if (mu <= 1e-6f || r <= 1.0f) return d;

	float a = min(aM, 1.0f);
	float q = min(mu / r, 0.999f);
	float cs = dot(d, spinAxis);

	// Model tier: weak field (1PN) uses the first order Lense-Thirring term;
	// the strong field (Kerr) tier enhances it by 1/(1-rs/r) near the horizon.
	float dphi;
	if (bStrong) {
		float f = max(1.0f - 2.0f * mu / r, 1e-3f);
		dphi = 2.0f * a * q * q * cs / f;
	}
	else {
		dphi = 2.0f * a * q * q * cs;
	}
	dphi = clamp(dphi, -1.0f, 1.0f);
	if (abs(dphi) < 1e-7f) return d;

	float ca = cos(dphi);
	float sa = sin(dphi);
	return normalize(d * ca + cross(spinAxis, d) * sa + spinAxis * dot(spinAxis, d) * (1.0f - ca));
}

// ---------------------------------------------------------------------
// Gravitational lensing: single inverse mapping step for one lens body
// (observed direction -> source direction). The caller loops over the
// significant bodies. Only rays heading towards the body are deflected; the
// weak-field formula would diverge exactly behind the observer (b -> 0),
// which would apply a huge fake deflection there.
// The Kerr spin correction (weak field limit) is included: the deflection is
// asymmetric about the spin axis (spin-orbit coupling).
// ---------------------------------------------------------------------
float3 RelLensStep(float3 d, float3 bodyDir, float dist, float mu, bool bKerr,
                   float3 spinAxis, float aM)
{
	if (dist < 1.0f || mu < 1e-6f) return d;
	if (dot(d, bodyDir) <= 0.0f) return d;

	float cosA = dot(d, bodyDir);
	float sinA = sqrt(max(1e-8f, 1.0f - cosA * cosA));
	float b = dist * sinA;
	if (b < 1e-3f) b = 1e-3f;

	// The plain 4*mu/b weak-field deflection diverges as b -> 0 (rays passing
	// close to the photon sphere), which folds the inverse mapping into radial
	// streaks. Use the finite strong-field expression near the body (within
	// ~20 mu ~ 10 rs) and keep the weak-field formula farther out.
	bool bNear = (dist < 20.0f * mu);
	float alpha;
	if (bKerr || bNear) alpha = RelLensDeflectionSchw(mu, b);
	else                alpha = 4.0f * mu / b;
	if (!(alpha >= 0.0f)) alpha = 0.0f;		// NaN/negative guard
	alpha = min(alpha, 1.2f);					// limiter (rad)

	// First order Kerr spin correction (weak field limit)
	if (bKerr) {
		if (aM > 1e-6f) {
			float3 n = normalize(spinAxis);
			float3 pn = cross(bodyDir, d);
			float pl = length(pn);
			if (pl > 1e-6f)
				alpha += -4.0f * aM * mu / (b * b) * dot(n, pn / pl);
		}
	}

	alpha = min(alpha, 0.8f);		// ~46 deg, keeps the mapping stable
	if (alpha < 0.0f) alpha = 0.0f;
	if (alpha < 1e-12f) return d;

	// Inverse lensing: the observed direction is rotated AWAY from the body by
	// alpha (the body appears displaced towards the source, so the source
	// direction is further away). A proper rotation about the axis
	// perpendicular to the (body, direction) plane is used instead of a
	// tangential translation: the translation is only valid for small alpha
	// and tears the mapping apart in the strong field.
	float3 rot = cross(bodyDir, d);
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
		float3 refv = (abs(bodyDir.y) < 0.9f) ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
		rot = normalize(cross(bodyDir, refv));
		d = bodyDir * ca + cross(rot, bodyDir) * sa;
	}
	return normalize(d);
}
