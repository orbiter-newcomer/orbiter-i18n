// ==============================================================
// ContactModel.inc
// Contact force solver (project extension).
//
// Included at the end of the official Vessel.cpp. The contact friction model
// (traditional / experimental) is selected in Vessel::AddSurfaceForces; this
// file only provides the normal force solver used by both models.
//
// The contact normal force for a touchdown point is
//
//     F_n = k*delta - c*v_n
//
// where delta is the compression depth and v_n the normal velocity of the
// contact point. Three solver modes are available:
//
//   0 (official)   explicit: the force is evaluated with the current velocity.
//                  This is what the original Orbiter code does. Stable only for
//                  small time steps compared to sqrt(m/k) and m/c.
//   1 (implicit damping)  the damping term is evaluated at the end of the step
//                  (backward Euler for the damper, spring still explicit):
//                      v_n* = v_n / (1 + dt*c/m)
//                      F_n  = k*delta - c*v_n*
//   2 (fully implicit)  both spring and damper are evaluated at the end of the
//                  step (backward Euler for the contact constraint):
//                      v_n* = (v_n + dt*k*delta/m) / (1 + dt*c/m + dt^2*k/m)
//                      F_n  = k*delta - (k*dt + c)*v_n*
//                  This removes the explicit stability limit of the contact
//                  spring/damper for the normal direction.
//
// Note: the coupling between multiple contact points and the rigid body
// rotation is approximated by using the total vessel mass. This keeps the
// solver local and cheap; it improves stability without changing the contact
// stiffness/damping parameters.
// ==============================================================

inline double ContactNormalForce (double k, double c, double tdy, double gv_n,
                                  double mass, double dt, int solver)
{
	double h = -tdy;                 // compression depth (>0: penetrating)
	if (h <= 0.0) return 0.0;        // no contact
	if (solver <= 0)                 // official explicit model
		return k*h - c*gv_n;

	double m = (mass > 1.0 ? mass : 1.0);
	if (solver == 1) {               // implicit damping only
		double vn = gv_n / (1.0 + dt*c/m);
		return k*h - c*vn;
	}

	// fully implicit (backward Euler for spring and damper)
	double vn = (gv_n + dt*k*h/m) / (1.0 + dt*c/m + dt*dt*k/m);
	return k*h - (k*dt + c)*vn;
}
