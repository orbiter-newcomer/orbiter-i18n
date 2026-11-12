// ============================================================================
// Relativity: physics extension implementation (merged include).
//
// Each host compilation unit enables exactly one section via a REL_PHYS_* macro:
//   Psys.cpp      : (default)            REL_PHYS_PSYS
//   Rigidbody.cpp : REL_PHYS_RIGIDBODY
//   Vessel.cpp    : REL_PHYS_VESSEL
//   Orbiter.cpp   : REL_PHYS_WARP
// ============================================================================

#if !defined(REL_PHYS_RIGIDBODY) && !defined(REL_PHYS_VESSEL) && !defined(REL_PHYS_WARP)
#define REL_PHYS_PSYS
#endif

#ifdef REL_PHYS_PSYS
// ---- Psys.cpp: 1PN + Kerr single-source gravitational acceleration ----------

// ============================================================================
// Relativistic gravitational acceleration (1PN + exact Kerr / Kerr-Schild).
//
// This file is included by Psys.cpp. It contains the free functions that were
// originally defined inline in Psys.cpp, moved here to keep the official Orbiter
// source file free of the relativity extension implementation:
//   SingleGacc_pn  - 1PN (Schwarzschild + Lense-Thirring) single-source acceleration
//   ks_*           - Kerr-Schild metric helpers (geometrised units, M = 1)
//   SingleGacc_kerr- exact Kerr single-source acceleration (with KS -> BL time conversion)
//
// Required context (provided by Psys.cpp before this include):
//   Vector/CelestialBody types, Ggrav/c_light constants, Log.h LOGOUT, td (TimeData)
// ============================================================================
// ====================================================================
// Relativistic (1PN) single-source gravitational acceleration.
// rpos = body position - reference position (points from reference to body)
// rvel = reference velocity - body velocity (relative velocity)
// Returns total acceleration (Newtonian + first post-Newtonian correction).
// ====================================================================
Vector SingleGacc_pn (const Vector &rpos, const Vector &rvel, const CelestialBody *body)
{
	double d = rpos.length();
	if (d <= 0) return Vector(0,0,0);
	double GM = Ggrav * body->Mass();
	double c2 = c_light * c_light;
	double d3 = d*d*d;
	Vector R (-rpos);          // from body to reference point
	double v2 = rvel.length2();
	double rv = dotp (R, rvel);
	// 1PN correction (Schwarzschild, coordinate time):
	// a_pn = GM/(c^2 d^3) * [ (4GM/d - v^2) R + 4 (R.v) v ]
	Vector a_pn = (R * (4.0*GM/d - v2) + rvel * (4.0*rv)) * (GM/(c2*d3));
	// Lense-Thirring (gravitomagnetic frame dragging) correction
	// J = inertia_factor * M * R^2 * omega * axis
	Vector J = body->RotAxis() * (body->InertiaFactor() * GM/Ggrav * body->Size()*body->Size() * body->RotOmega());
	Vector rhat = R * (1.0/d);   // unit vector from body to reference
	// Gravitomagnetic field B_g = [3(J.rhat) rhat - J] G/(c^2 d^3). The vector
	// factor has to be the *unit* vector rhat: using R (length d) here makes
	// the Lense-Thirring term dimensionally wrong (m^2/s^2 instead of m/s^2)
	// and scales it by a factor d (1e6 for the black hole test scenario).
	Vector Bg = (rhat * (3.0*dotp(rhat, J)) - J) * (Ggrav/(c2*d3));
	Vector a_lt = crossp (rvel, Bg) * -2.0;

	// Diagnostic (sampled every 0.5 s of simulation time): dump the actual
	// Lense-Thirring parameters of the main (heavy) source. This confirms
	// whether the relativistic path is active, which spin axis the source
	// actually has, and how large the LT term is compared with Newtonian
	// gravity. axisNdot = |axis . orbit_normal|: 0 = polar orbit (LAN is
	// sensitive to frame dragging), 1 = equatorial (LAN does not precess).
	{
		static double dbgLastT = -1e100;
		if (body->Mass() > 1e29 && td.SimT0 - dbgLastT > 0.5) {
			dbgLastT = td.SimT0;
			Vector nrm = crossp (rpos, rvel);
			double nlen = nrm.length();
			Vector axis = body->RotAxis();
			double axisNdot = (nlen > 0.0 ? fabs (dotp (axis, nrm/nlen)) : -1.0);
			double aN = GM/(d*d);
			double Jlen = J.length();
			double aM = Jlen*c_light/(Ggrav*body->Mass()*body->Mass());
			LOGOUT ("RELPN: body=%s mass=%.4e axis=(%.4f,%.4f,%.4f) omega=%.4f inertia=%.3f J=%.4e aM=%.6f r=%.4e v=%.4e aN=%.4e aLT=%.4e |aLT|/aN=%.3e axisNdot=%.3f",
				body->Name(), body->Mass(), axis.x, axis.y, axis.z, body->RotOmega(), body->InertiaFactor(),
				Jlen, aM, d, rvel.length(), aN, a_lt.length(), a_lt.length()/aN, axisNdot);
		}
	}
	return rpos * (GM/d3) + a_pn + a_lt + SingleGacc_perturbation (rpos, body);
}

// ====================================================================
// Kerr-Schild metric helpers (geometrised units: G = c = 1, M = 1)
// ====================================================================

static double ks_radius (double x, double y, double z, double a2)
{
	double rho2 = x*x + y*y + z*z;
	double b = 0.5*(rho2 - a2);
	double r2 = b + sqrt (b*b + a2*z*z);
	return sqrt (r2 > 0.0 ? r2 : 0.0);
}

// Kerr-Schild metric: g_munu = eta_munu + f k_mu k_nu  (M = 1)
static void ks_metric (const double p[3], double a, double g[4][4], double k[4], double &f)
{
	double x = p[0], y = p[1], z = p[2];
	double a2 = a*a;
	double r = ks_radius (x, y, z, a2);
	if (r < 1e-12) r = 1e-12;
	double r2 = r*r;
	double denom = r2*r2 + a2*z*z;
	f = (denom > 1e-300 ? 2.0*r2*r/denom : 0.0);
	k[0] = 1.0;
	k[1] = (r*x + a*y)/(r2 + a2);
	k[2] = (r*y - a*x)/(r2 + a2);
	k[3] = z/r;
	for (int mu = 0; mu < 4; mu++)
		for (int nu = 0; nu < 4; nu++)
			g[mu][nu] = (mu == nu ? (mu == 0 ? -1.0 : 1.0) : 0.0) + f*k[mu]*k[nu];
}

// inverse: g^munu = eta^munu - f k^mu k^nu, k^mu = (-k_0, k_1, k_2, k_3)
static void ks_inverse (const double k[4], double f, double gi[4][4])
{
	double ku[4] = { -k[0], k[1], k[2], k[3] };
	for (int mu = 0; mu < 4; mu++)
		for (int nu = 0; nu < 4; nu++) {
			double eta = (mu == nu ? (mu == 0 ? -1.0 : 1.0) : 0.0);
			gi[mu][nu] = eta - f*ku[mu]*ku[nu];
		}
}

// Kerr-Schild term H_munu = f k_mu k_nu, i.e. g_munu - eta_munu. Computing it
// directly (instead of subtracting eta from the full metric) keeps the full
// relative precision of the small weak-field correction: g = eta + H with
// H ~ 1e-9 for a planet, so differentiating g would lose most of the digits
// to catastrophic cancellation.
static void ks_schild (const double p[3], double a, double H[4][4])
{
	double g[4][4], k[4], f;
	ks_metric (p, a, g, k, f);
	for (int mu = 0; mu < 4; mu++)
		for (int nu = 0; nu < 4; nu++)
			H[mu][nu] = f*k[mu]*k[nu];
}

// numerical Christoffel symbols of the Kerr-Schild metric
static void ks_christoffel (const double p[3], double a, double Gamma[4][4][4])
{
	double g[4][4], gp[4][4][4], k[4], f;
	// The metric is static: the time derivatives (nu = 0) are zero. The
	// remaining derivatives are ordered like the spacetime index: nu = 1,2,3
	// correspond to the spatial coordinates x, y, z.
	for (int mu = 0; mu < 4; mu++)
		for (int nu = 0; nu < 4; nu++) gp[0][mu][nu] = 0.0;
	for (int nu = 1; nu < 4; nu++) {
		int al = nu - 1;               // spatial coordinate index
		double h = 1e-6 * (fabs (p[0]) + fabs (p[1]) + fabs (p[2]) + 1.0);
		double pp[3] = {p[0], p[1], p[2]}, pm[3] = {p[0], p[1], p[2]};
		pp[al] += h; pm[al] -= h;
		double Hpp[4][4], Hpm[4][4];
		ks_schild (pp, a, Hpp);
		ks_schild (pm, a, Hpm);
		for (int mu = 0; mu < 4; mu++)
			for (int be = 0; be < 4; be++)
				gp[nu][mu][be] = (Hpp[mu][be] - Hpm[mu][be])/(2.0*h);
	}
	ks_metric (p, a, g, k, f);
	double gi[4][4];
	ks_inverse (k, f, gi);
	for (int mu = 0; mu < 4; mu++)
		for (int al = 0; al < 4; al++)
			for (int be = 0; be < 4; be++) {
				double sum = 0.0;
				for (int nu = 0; nu < 4; nu++)
					sum += gi[mu][nu]*(gp[al][nu][be] + gp[be][nu][al] - gp[nu][al][be]);
				Gamma[mu][al][be] = 0.5*sum;
			}
}

// ====================================================================
// Exact Kerr (Kerr-Schild Cartesian) gravitational acceleration.
// rpos: body position - reference position (from reference to body)
// rvel: reference velocity - body velocity (relative velocity)
// Returns the coordinate acceleration of a test particle in the global
// (Cartesian) frame. Geometrised internally, converted back to SI.
// ====================================================================
Vector SingleGacc_kerr (const Vector &rpos, const Vector &rvel, const CelestialBody *body)
{
	double mass = body->Mass();
	double GM = Ggrav * mass;
	double c = c_light;
	double L = GM/(c*c);              // geometric length scale [m]
	if (L <= 0.0 || mass <= 1e-300) return SingleGacc_pn (rpos, rvel, body);

	// spin angular momentum J = I * omega * axis
	double R = body->Size();
	double Imom = body->InertiaFactor() * mass * R * R;
	Vector J = body->RotAxis() * (Imom * body->RotOmega());
	double Jmag = J.length();
	Vector ez = (Jmag > 1e-300 ? J * (1.0/Jmag) : body->RotAxis());
	// orthonormal basis (z = spin axis)
	Vector ref = (fabs (ez.x) < 0.9 ? Vector(1,0,0) : Vector(0,1,0));
	Vector ex = crossp (ez, ref);
	double exl = ex.length();
	if (exl < 1e-12) return SingleGacc_pn (rpos, rvel, body);
	ex *= (1.0/exl);
	Vector ey = crossp (ez, ex);

	// Kerr spin parameter (dimensionless): a/M = J c / (G M^2)
	double aM = Jmag * c / (Ggrav * mass * mass);
	if (aM > 1000.0) aM = 1000.0;

	// Position and velocity in Kerr coordinates (geometrised). The metric is
	// centred on the body, while rpos runs from the particle to the body, so
	// the particle sits at -rpos in the body-centred frame. (Using rpos here
	// flips the sign of the radial acceleration and makes orbits escape.)
	double p[3];
	p[0] = -dotp (rpos, ex)/L;
	p[1] = -dotp (rpos, ey)/L;
	p[2] = -dotp (rpos, ez)/L;
	double v[3];
	v[0] = dotp (rvel, ex)/c;
	v[1] = dotp (rvel, ey)/c;
	v[2] = dotp (rvel, ez)/c;
	if (v[0]*v[0] + v[1]*v[1] + v[2]*v[2] >= 1.0) return SingleGacc_pn (rpos, rvel, body);

	// --- Kerr-Schild coordinate-time conversion (part 1) -------------------
	// The metric and the geodesic acceleration below are expressed in
	// Kerr-Schild (ingoing) coordinates, whose time coordinate is related to
	// the global (inertial / Boyer-Lindquist) coordinate time by
	//   t_KS = t_BL + F(r),   F'(r) = (r^2 + a^2)/Delta - 1 = 2 r/Delta,
	//   Delta = r^2 - 2 r + a^2      (geometrised units, M = 1)
	// Speeds and accelerations must be converted accordingly before the result
	// can be used in the global frame, otherwise the O(v^2/c^2) coordinate
	// mismatch acts as a spurious acceleration (Kerr orbits run away at high
	// speed: |a_Kerr|/a_Newton reached 1.14 for v=0.27c in the test scenario).
	double vBL[3] = { v[0], v[1], v[2] };   // velocity in the global frame
	double a2m = aM*aM;
	double rks = ks_radius (p[0], p[1], p[2], a2m);
	double delta = rks*rks - 2.0*rks + a2m;
	double rhat[3] = { 0.0, 0.0, 0.0 };
	double rdot = 0.0, dF = 0.0, dF2 = 0.0, s = 1.0;
	bool reltime = (rks > 0.5 && delta > 1e-6);   // outside the event horizon
	if (reltime) {
		double inv_r = 1.0/rks;
		rhat[0] = p[0]*inv_r; rhat[1] = p[1]*inv_r; rhat[2] = p[2]*inv_r;
		rdot = vBL[0]*rhat[0] + vBL[1]*rhat[1] + vBL[2]*rhat[2];
		dF  = 2.0*rks/delta;                                     // F'(r) = (r^2+a^2)/Delta - 1
		dF2 = (2.0*delta - 4.0*rks*(rks - 1.0))/(delta*delta);   // F''(r)
		s = 1.0 + dF*rdot;
		if (s < 1e-3) s = 1e-3;
		// coordinate velocity used in the geodesic equation: v_KS = v_BL/s
		v[0] = vBL[0]/s; v[1] = vBL[1]/s; v[2] = vBL[2]/s;
	}

	// metric
	double g[4][4], k[4], f;
	ks_metric (p, aM, g, k, f);

	// 4-velocity u^mu (coordinate time): u^i = v^i u^t, normalized g_munu u^mu u^nu = -1
	double svv = 0.0, s0v = 0.0;
	for (int i = 1; i < 4; i++) {
		for (int j = 1; j < 4; j++) svv += g[i][j]*v[i-1]*v[j-1];
		s0v += 2.0*g[0][i]*v[i-1];
	}
	// g_00 = -1 + f, so the normalisation g_munu u^mu u^nu = -1 gives
	// (u^t)^2 = 1/(1 - f - g_ij v^i v^j - 2 g_0i v^i).
	double norm = 1.0 - f - svv - s0v;
	if (norm <= 1e-12) return SingleGacc_pn (rpos, rvel, body);
	double ut = 1.0/sqrt (norm);
	double u[4] = { ut, v[0]*ut, v[1]*ut, v[2]*ut };

	// Coordinate acceleration a^i = d^2 x^i / dt^2 from the geodesic equation:
	//   a^i = [ -Gamma^i_ab u^a u^b + v^i Gamma^0_ab u^a u^b ] / (u^t)^2
	// The second term comes from du^t/dtau. It is O(v^2/c^2) (negligible for
	// planets) but matters in the strong-field / high-speed regime.
	double Gamma[4][4][4];
	ks_christoffel (p, aM, Gamma);
	double att = 0.0;
	for (int al = 0; al < 4; al++)
		for (int be = 0; be < 4; be++)
			att += Gamma[0][al][be]*u[al]*u[be];
	double at[3];
	for (int i = 1; i < 4; i++) {
		double sum = 0.0;
		for (int al = 0; al < 4; al++)
			for (int be = 0; be < 4; be++)
				sum += Gamma[i][al][be]*u[al]*u[be];
		at[i-1] = (-sum + v[i-1]*att)/(ut*ut);
	}

	// --- Kerr-Schild coordinate-time conversion (part 2) -------------------
	// From a_KS = a_BL/s^2 - v_BL*s'/s^3 (s = dt_KS/dt_BL, s' = ds/dt_BL):
	//   a_BL = a_KS*s^2 + v_BL*s'/s,   s' = F''*rdot^2 + F'*r_ddot
	// The radial acceleration follows from the spherical-coordinate identity
	//   r_ddot = (a_BL . rhat) + (v_BL^2 - rdot^2)/r
	// and, using s - F'*rdot = 1, is obtained explicitly as
	//   r_ddot = s*( (a_KS . rhat)*s^2 + F''*rdot^3/s + (v_BL^2 - rdot^2)/r )
	double ag[3] = { at[0], at[1], at[2] };
	if (reltime) {
		double inv_r = 1.0/rks;
		double v2BL = vBL[0]*vBL[0] + vBL[1]*vBL[1] + vBL[2]*vBL[2];
		double aKSr = at[0]*rhat[0] + at[1]*rhat[1] + at[2]*rhat[2];
		double rddot = s*(aKSr*s*s + dF2*rdot*rdot*rdot/s + (v2BL - rdot*rdot)*inv_r);
		double fac = (dF2*rdot*rdot + dF*rddot)/s;
		for (int i = 0; i < 3; i++)
			ag[i] = at[i]*s*s + vBL[i]*fac;
	}

	// back to SI units and global frame
	double scale = c*c/L;
	Vector acc = (ex*ag[0] + ey*ag[1] + ez*ag[2]) * scale;

	// Diagnostic (sampled every 0.5 s of simulation time): dump the spin
	// parameters and the ratio of the Kerr acceleration to the Newtonian one.
	{
		static double dbgLastT = -1e100;
		if (td.SimT0 - dbgLastT > 0.5) {
			dbgLastT = td.SimT0;
			Vector nrm = crossp (rpos, rvel);
			double nlen = nrm.length();
			Vector axis = body->RotAxis();
			double axisNdot = (nlen > 0.0 ? fabs (dotp (axis, nrm/nlen)) : -1.0);
			double rgeo = (reltime ? rks : sqrt (p[0]*p[0] + p[1]*p[1] + p[2]*p[2]));
			double aN = GM/rpos.length2();
			LOGOUT ("RELKERR: body=%s aM=%.6f axis=(%.4f,%.4f,%.4f) r_geom=%.3f r=%.4e v_c=%.5f |aK|/aN=%.6f axisNdot=%.3f",
				body->Name(), aM, axis.x, axis.y, axis.z, rgeo, rpos.length(), rvel.length()/c,
				acc.length()/aN, axisNdot);
		}
	}
	return acc;
}

#endif // REL_PHYS_PSYS

#ifdef REL_PHYS_RIGIDBODY
// ---- Rigidbody.cpp: Encke (stabilised orbit) relativistic perturbation -----

// -----------------------------------------------------------------------------
// Relativity: relativistic correction for the Encke (stabilised) propagator.
//
// Numerically stabilised (Encke) propagation normally ignores all gravitational
// perturbations to stay on the two-body reference orbit. Relativistic
// corrections, however, are small *secular* perturbations (they drive effects
// such as the Lense-Thirring node precession) and must not be dropped here.
// They are added as a perturbation: exact relativistic single-source accelera-
// tion minus the Newtonian two-body reference acceleration. The correction is
// O(v^2/c^2 or GM/(rc^2)) of the primary gravity, i.e. well within the range
// Encke's method is designed for.
//
// Implementation moved out of the official Rigidbody.cpp (included there).
// -----------------------------------------------------------------------------
static bool RelativityEnckePerturbation (Vector &acc, const CelestialBody *cbody, const StateVectors &state_rel)
{
	if (!cbody || !g_pOrbiter->Cfg()->CfgPhysicsPrm.bRelativity) return false;
	// Newtonian gravity model: no additional gravitational correction
	if (g_pOrbiter->Cfg()->CfgPhysicsPrm.RelativityGravityMethod == 2) return false;
	Vector rpos, rvel;
	rpos.Set (-state_rel.pos.x, -state_rel.pos.y, -state_rel.pos.z); // central body -> vessel
	rvel.Set (state_rel.vel.x, state_rel.vel.y, state_rel.vel.z);    // vessel velocity relative to central body
	Vector a_newton = SingleGacc (rpos, cbody);
	// SingleGacc_kerr converts its coordinate acceleration to the global
	// coordinate time (the Kerr-Schild time differs from the global time by
	// a radial term), so the exact Kerr solution can be used here as well.
	Vector a_rel = (g_pOrbiter->Cfg()->CfgPhysicsPrm.RelativityGravityMethod == 1 ?
		SingleGacc_kerr (rpos, rvel, cbody) : SingleGacc_pn (rpos, rvel, cbody));
	acc.Set (a_rel.x - a_newton.x, a_rel.y - a_newton.y, a_rel.z - a_newton.z);
	return true;
}

#endif // REL_PHYS_RIGIDBODY

#ifdef REL_PHYS_VESSEL
// ---- Vessel.cpp: non-gravitational correction + FTL velocity protection ----

// -----------------------------------------------------------------------------
// Relativity: vessel dynamics corrections.
//
//  - RelativityNonGravAccel: relativistic correction of the non-gravitational
//    acceleration (a = [a0 - (v.a0) v/c^2] / gamma).
//  - RelativityFTLGuard: superluminal-velocity protection for the vessel update
//    (clamp to 0.9c or pause the simulation, depending on the configured mode).
//
// Implementation moved out of the official Vessel.cpp (included there).
// -----------------------------------------------------------------------------
static void RelativityNonGravAccel (Vector &a_ng, const Vector &vel)
{
	if (!g_pOrbiter->Cfg()->CfgPhysicsPrm.bRelativity) return;
	// relativistic correction of non-gravitational accelerations:
	// a = [a0 - (v.a0) v/c^2] / gamma  (longitudinal -> gamma^3, transversal -> gamma)
	double v2 = vel.length2();
	double c2 = c_light*c_light;
	if (v2 < c2*0.999999) {
		double gamma = 1.0/sqrt (1.0 - v2/c2);
		a_ng = (a_ng - vel * (dotp (vel, a_ng)/c2)) / gamma;
	}
}

// Returns true if the vessel update must be aborted (FTL error report mode).
static bool RelativityFTLGuard (StateVectors &st0, StateVectors *st1)
{
	if (!g_pOrbiter->Cfg()->CfgPhysicsPrm.bRelativity) return false;
	double v2 = st0.vel.length2();
	double c2 = c_light*c_light;
	if (v2 < c2) return false;
	if (g_pOrbiter->Cfg()->CfgPhysicsPrm.RelativityFTLMode == 1) {
		// clamp velocity to 0.9c along its current direction
		double vl = sqrt (v2);
		double fac = 0.9 * c_light / vl;
		st0.vel *= fac;
		if (st1) st1->vel *= fac;
		return false;
	}
	static bool bFTLReported = false;
	if (!bFTLReported) {
		bFTLReported = true;
		g_pOrbiter->Pause (true);
		char msg[512];
		sprintf_s (msg, 512, "Relativistic mode: FTL velocity detected (|v| = %.4e m/s). Simulation paused.", sqrt (v2));
		LOGOUT_ERR (msg);
	}
	return true;
}

#endif // REL_PHYS_VESSEL

#ifdef REL_PHYS_WARP
// ---- Orbiter.cpp: relativistic time acceleration (Lorentz factor) ----------

// -----------------------------------------------------------------------------
// Relativity: time acceleration scaling (Lorentz factor) and display handling.
//
// Implementation moved out of the official Orbiter.cpp (included at the
// original location). Declarations live in Orbiter.h.
// -----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: RelWarpScale()
// Desc: Lorentz factor used to scale the player-selected time acceleration.
//       Returns 1 when relativistic physics is disabled (the acceleration is
//       then used as-is). The scaling is a simulation-internal detail: the
//       player still selects and sees the unscaled factor.
//-----------------------------------------------------------------------------
double Orbiter::RelWarpScale () const
{
	if (!pConfig->CfgPhysicsPrm.bRelativity) return 1.0;
	if (!g_focusobj) return 1.0;
	double v2 = g_focusobj->GVel().length2();
	double c2 = c_light*c_light;
	if (v2 >= c2*0.999999) return 1.0; // guard (velocity clamp is applied elsewhere)
	return 1.0/sqrt (1.0 - v2/c2);
}

//-----------------------------------------------------------------------------
// Name: UpdateRelativityWarp()
// Desc: Relativistic time acceleration: when relativistic physics is enabled
//       and the camera focus is a vessel, scale the global time acceleration
//       by the vessel Lorentz factor gamma. This is applied "under water":
//       the effective warp used by the simulation differs from the factor the
//       player selects and sees (m_relUserWarp).
//-----------------------------------------------------------------------------
void Orbiter::UpdateRelativityWarp ()
{
	double gamma = RelWarpScale();
	double target = m_relUserWarp * gamma;
	if      (target < MinWarpLimit) target = MinWarpLimit;
	else if (target > MaxWarpLimit) target = MaxWarpLimit;
	if (fabs (target - td.Warp()) > 1e-6) {
		td.SetWarp (target, 0);
		if (td.WarpChanged()) ApplyWarpFactor();
	}
	m_relWarpActive = (gamma != 1.0);
}

//-----------------------------------------------------------------------------
// Name: DisplayWarp()
// Desc: Return the time acceleration value to display to the user. The player
//       only controls the unscaled warp factor: the automatic relativistic
//       scaling stays invisible.
//-----------------------------------------------------------------------------
double Orbiter::DisplayWarp () const
{
	if (pConfig->CfgPhysicsPrm.bRelativity) return m_relUserWarp;
	return td.Warp();
}

#endif // REL_PHYS_WARP
