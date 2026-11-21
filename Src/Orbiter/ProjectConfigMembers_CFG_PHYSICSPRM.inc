// ==============================================================
// ProjectConfigMembers_CFG_PHYSICSPRM.inc - project extension member declarations for class CFG_PHYSICSPRM.
// Included inside the class body in the official header: the official
// header only grows by the include line.
// ==============================================================

	bool   bWrecks;              // wreck/debris system enable flag (independent of relativity)

	int    ContactSolver;           // contact force solver: 0=explicit, 1=implicit damping, 2=fully implicit
	double ContactGearStiffness;    // landing gear stiffness [N/m] (0 = original)
	double ContactGearDampingRatio; // landing gear damping ratio zeta (0 = original damping)
	double ContactGearFriction;     // landing gear friction coefficient (0 = original)
	double ContactHullStiffness;    // hull/body stiffness [N/m] (0 = original)
	double ContactHullDampingRatio; // hull/body damping ratio zeta (0 = original damping)
	double ContactHullFriction;     // hull/body friction coefficient (0 = original)
	bool   bParallelVesselUpdate;   // multi-core vessel state update (project extension)
	double ParallelVesselRange;     // vessels closer than this are updated in sequence [m]

