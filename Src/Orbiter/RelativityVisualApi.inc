// ============================================================================
// RelativityVisualApi.inc
// Host-side implementation of the relativistic-visual API:
//   oapiGetRelativitySpin() -- dimensionless Kerr spin parameter (a/M) and
//   spin axis of a celestial body, derived the same way as in the physics
//   engine:  J = I * omega * axis,  a/M = J c / (G M^2)
//
// Included by Src/Orbiter/OrbiterAPI.cpp (which keeps a 1-line forwarder
// with the DLLEXPORT attribute).
// ============================================================================

bool RelGetRelativitySpin (OBJHANDLE hObj, double *aM, VECTOR3 *axis)
{
	if (!hObj) return false;
	Body *body = (Body*)hObj;
	CelestialBody *cb = dynamic_cast<CelestialBody*>(body);
	if (!cb) return false;

	double mass = cb->Mass();
	double R = cb->Size();
	if (mass <= 1e-300 || R <= 0.0) return false;

	double Imom = cb->InertiaFactor() * mass * R * R;
	Vector Jv = cb->RotAxis() * (Imom * cb->RotOmega());
	double Jmag = Jv.length();
	double am = Jmag * c_light / (Ggrav * mass * mass);
	if (am > 1e3) am = 1e3;

	double jx, jy, jz;
	if (Jmag > 1e-300) {
		jx = Jv.x / Jmag;
		jy = Jv.y / Jmag;
		jz = Jv.z / Jmag;
	}
	else {
		Vector ax = cb->RotAxis();
		jx = ax.x; jy = ax.y; jz = ax.z;
	}

	if (aM) *aM = am;
	if (axis) *axis = _V(jx, jy, jz);
	return true;
}
