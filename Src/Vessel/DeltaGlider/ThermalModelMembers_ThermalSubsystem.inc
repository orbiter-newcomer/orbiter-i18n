// ==============================================================
// ThermalModelMembers_ThermalSubsystem.inc - project extension member declarations for class ThermalSubsystem.
// Included inside the class body in the official header: the official
// header only grows by the include line.
// ==============================================================

public:
	void clbkPostCreation ();

private:

	// Structural equivalent masses of the dynamically loaded compartments
	// (CABIN and the propellant tanks). Each compartment holds a share of the
	// vessel's true empty mass as structural mass; that mass is converted to an
	// equivalent mass for the compartment's own specific heat:
	//     m_equiv = m_struct * c_struct / cp_compartment
	// so that m_equiv*cp_compartment = m_struct*c_struct (physical structural
	// heat capacity, independent of the load). Fixed compartments keep their
	// structural mass directly.
	double mass_struct_equiv[13];
	double m0_struct;   // cached true empty mass the current masses were derived from

	// Recompute all compartment masses from the vessel's true empty mass.
	// Called from clbkPostCreation and whenever the empty mass changes (passenger
	// changes, SCRAM conversion).
	void UpdateStructuralMasses ();
	void InitCompartmentParams ();

	// Update the thermal state for one simulation step (explicit or half-implicit,
	// depending on the step size).
	void UpdateThermalStep (double simt, double simdt, double mjd);

	// Accumulate the current net heat fluxes [W] for all compartments.
	void ComputeHeatFlux (double *compartmentQ, bool includeCoolant);


	// Detect non-finite / out-of-range temperatures. Repairs them and reports a
	// numerical failure to the wreck/destruction system (once per vessel).
	bool CheckTemperatureSanity ();
	void ReportNumericFailure (double Tmax, int compartment);

private:
	// Structural and numerical parameters for the compartment mass model and the
	// integration path (implemented in ThermalSolver.inc).
	static const double c_struct_ref;    // structural reference specific heat [J/kg/K]
	static const double THERMAL_DT_STABLE; // explicit integration step limit [s]; larger
	                                       // steps use the half-implicit solver
	static const double T_MAX_SANE;      // upper sanity bound for compartment temperature [K]


private:

	// location of the sun (global frame). The sun is not necessarily at the global
	// origin, so all solar computations use its actual position.
	OBJHANDLE hSun;
	VECTOR3 Spos;
	void UpdateSun ();   // locate the sun and cache its global position
	OBJHANDLE hSunRef;   // surface reference used for the last sun search

	bool bNumericFailureReported;   // numerical failure already reported to the wreck system
	int  nQuasiFail;                // consecutive failed implicit steps

	// simulation time up to which the thermal state has been integrated
	// (used to advance the thermal state over large simulation steps)
	double t_thermal;

	// half-implicit (backward Euler) integration of one step (ThermalSolver.inc)
	bool SolveImplicitStep (double dt);
	void AssembleImplicitSystem (double dt, double *Amat, double *bvec);
	bool SolveLinear13 (const double *Amat, const double *bvec, double *x);

