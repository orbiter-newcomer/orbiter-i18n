// ==============================================================
// ThermalSolver.inc
// DeltaGlider thermal control - project extension.
//
// Implementation of the project extensions to the official DeltaGlider
// thermal subsystem (Src/Vessel/DeltaGlider/ThermalSubsys.cpp):
//   - compartment mass model (true empty mass -> structural equivalent mass)
//   - heat flux accumulation and quasi-static equilibrium solver
//   - half-implicit (backward Euler) thermal integration for large time steps
//   - coolant loop quasi-static / explicit support
//   - temperature sanity guard and numerical-failure report
//
// This file is included at the end of ThermalSubsys.cpp. Keeping the
// implementation here keeps the changes to the official Orbiter sources
// minimal (the official file only contains declarations, small hooks and the
// #include of this file).
// ==============================================================

// ---- mass model parameters ------------------------------------------

// structural reference specific heat [J/kg/K]
const double ThermalSubsystem::c_struct_ref = 600.0;

// largest step handled by the explicit integrator [s]
const double ThermalSubsystem::THERMAL_DT_STABLE = 35.0;

// upper sanity bound for compartment temperatures [K]
const double ThermalSubsystem::T_MAX_SANE = 5000.0;

// --------------------------------------------------------------

// Initialise the compartment parameters (masses are derived from the vessel's
// true empty mass in UpdateStructuralMasses()).
void ThermalSubsystem::InitCompartmentParams ()
{
	for (int i = 0; i < 13; i++) {
		cprm[i].mass     = 0.0;
		cprm[i].mass_air = 0.0;
		mass_struct_equiv[i] = 0.0;
	}
	m0_struct = 0.0;

	mass_struct_equiv[CABIN] = 0.0;
	t_thermal = -1e10;
	nQuasiFail = 0;
}

// --------------------------------------------------------------

// Finalise vessel creation: now the core vessel holds its real empty mass.
void ThermalSubsystem::clbkPostCreation ()
{
	UpdateStructuralMasses();
}

// --------------------------------------------------------------

// Derive all compartment masses from the vessel's true empty mass.
//
// The structural mass of the dynamically loaded compartments (cabin, propellant
// tanks) is a share of the empty mass and is taken out of the fixed compartments
// they physically belong to:
//   - cabin structure          <- interior fuselage
//   - wing tank structure      <- upper/lower wing surface, split equally
//   - main tank structure      <- upper/lower fuselage, proportional to their base share
// The structural mass is converted into an equivalent mass for the compartment's
// own specific heat:
//     m_equiv = m_struct * c_struct_ref / cp_compartment
// so that m_equiv*cp_compartment = m_struct*c_struct_ref (physical heat capacity).
// All other fixed compartments keep their original share of the empty mass.
void ThermalSubsystem::UpdateStructuralMasses ()
{
	m0_struct = DG()->GetEmptyMass();

	// structural shares of the empty mass belonging to the loaded compartments
	static const double f_cabin    = 0.012;
	static const double f_wingtank = 0.02;    // each wing tank
	static const double f_maintank = 0.03;

	const double m_cabin    = m0_struct * f_cabin;
	const double m_wingtank = m0_struct * f_wingtank;
	const double m_maintank = m0_struct * f_maintank;

	// base shares of the fixed compartments (fraction of the empty mass)
	static const double base_coef[13] = {
		0.20, 0.15, 0.075, 0.075, 0.075, 0.075, 0.31, 0.03,
		0.0, 0.0, 0.0, 0.0, 0.01
	};

	double m[13];
	int i;
	for (i = 0; i < 13; i++) m[i] = base_coef[i] * m0_struct;

	// move the structural mass of the loaded compartments out of the fixed
	// compartments they belong to
	m[INTERIORFUSELAGE] -= m_cabin;
	m[SURFUPPERLEFTWING]  -= 0.5*m_wingtank;
	m[SURFLOWERLEFTWING]  -= 0.5*m_wingtank;
	m[SURFUPPERRIGHTWING] -= 0.5*m_wingtank;
	m[SURFLOWERRIGHTWING] -= 0.5*m_wingtank;
	const double fuse_base = base_coef[SURFUPPERFUSELAGE] + base_coef[SURFLOWERFUSELAGE];
	m[SURFUPPERFUSELAGE] -= m_maintank * base_coef[SURFUPPERFUSELAGE] / fuse_base;
	m[SURFLOWERFUSELAGE] -= m_maintank * base_coef[SURFLOWERFUSELAGE] / fuse_base;

	for (i = 0; i < 13; i++)
		if (m[i] < 0.0) m[i] = 0.0;

	// structural equivalent masses of the loaded compartments
	mass_struct_equiv[CABIN] = m_cabin    * c_struct_ref / cprm[CABIN].cp;
	mass_struct_equiv[PROPELLANT_LEFTWING]  = m_wingtank * c_struct_ref / cprm[PROPELLANT_LEFTWING].cp;
	mass_struct_equiv[PROPELLANT_RIGHTWING] = m_wingtank * c_struct_ref / cprm[PROPELLANT_RIGHTWING].cp;
	mass_struct_equiv[PROPELLANT_MAIN]      = m_maintank * c_struct_ref / cprm[PROPELLANT_MAIN].cp;

	// fixed compartments carry their structural mass; loaded compartments
	// currently carry their structural equivalent mass only (the load mass is
	// added in UpdateThermalStep)
	for (i = 0; i < 13; i++) {
		if (i == CABIN || i == PROPELLANT_LEFTWING || i == PROPELLANT_RIGHTWING || i == PROPELLANT_MAIN)
			cprm[i].mass = mass_struct_equiv[i];
		else
			cprm[i].mass = m[i];
	}
	cprm[CABIN].mass_air = 0.0;
}

// --------------------------------------------------------------

// Locates the sun (star) and caches its global position. The sun is not
// necessarily at the origin of the global frame (the old code assumed that).
void ThermalSubsystem::UpdateSun ()
{
	// Re-resolve the sun when the surface reference changes (the vessel may
	// switch reference bodies during the simulation; never cache it forever).
	OBJHANDLE hRef = DG()->GetSurfaceRef();
	if (!hSun || hRef != hSunRef) {
		hSunRef = hRef;
		hSun = NULL;
		for (OBJHANDLE hObj = hRef; hObj; hObj = oapiGetGbodyParent(hObj)) {
			if (oapiGetObjectType(hObj) == OBJTP_STAR) { hSun = hObj; break; }
		}
		if (!hSun) hSun = oapiGetGbodyByIndex(0);   // fallback: old behaviour
	}
	if (hSun) oapiGetGlobalPos(hSun, &Spos);
}

// --------------------------------------------------------------

// Update the thermal state for the current simulation step.
//
// Time bookkeeping: t_thermal is the simulation time up to which the thermal
// state has been integrated. Normally it equals simt (state at the beginning of
// the step). If the vessel enters the simulation with a large jump (scenario
// load, time jump) the state is resynchronised.
//
// Small steps use the explicit integrator. Steps larger than
// THERMAL_DT_STABLE use a half-implicit (backward Euler) step which is
// unconditionally stable, so the accelerated simulation can advance in one
// step without the explicit stability limit.
void ThermalSubsystem::UpdateThermalStep (double simt, double simdt, double mjd)
{
	// the vessel's empty mass can change during the simulation (passengers,
	// SCRAM conversion); the thermal compartment masses have to follow it
	if (DG()->GetEmptyMass() != m0_struct)
		UpdateStructuralMasses();

	UpdateSun();   // cache the actual sun position (the sun is not at the origin)

	// constants and spacecraft parameters
	static const double R = 287.0;          // gas constant [J/kg/K]
	static const double v_cabin = 24.0;     // cabin volume
	static const double v_airlock = 4.0;    // airlock volume

	// dynamic compartment masses: structural equivalent mass + load mass
	// (CMB floor also protects the cabin mass computation against T=0)
	double T_cab = max (cprm[CABIN].T, T_CMB);
	cprm[CABIN].mass_air = (DG()->SubsysPressure()->PCabin() * v_cabin + DG()->SubsysPressure()->PAirlock() * v_airlock)/(R*T_cab);
	cprm[CABIN].mass = mass_struct_equiv[CABIN] + cprm[CABIN].mass_air;
	if (DG()->ScramVersion()) {
		cprm[PROPELLANT_LEFTWING].mass = cprm[PROPELLANT_RIGHTWING].mass =
			mass_struct_equiv[PROPELLANT_LEFTWING] + 0.5*DG()->GetPropellantMass(DG()->ph_main);
		cprm[PROPELLANT_MAIN].mass = mass_struct_equiv[PROPELLANT_MAIN] +
			DG()->GetPropellantMass(DG()->ph_rcs) + DG()->SubsysScram()->GetPropellantMass();
	} else {
		cprm[PROPELLANT_LEFTWING].mass = cprm[PROPELLANT_RIGHTWING].mass =
			mass_struct_equiv[PROPELLANT_LEFTWING] + 0.35*DG()->GetPropellantMass(DG()->ph_main);
		cprm[PROPELLANT_MAIN].mass = mass_struct_equiv[PROPELLANT_MAIN] +
			DG()->GetPropellantMass(DG()->ph_main)*0.3 + DG()->GetPropellantMass(DG()->ph_rcs);
	}

	atm_p = DG()->GetAtmPressure();
	atm_T = DG()->GetAtmTemperature();
	eps = (atm_p ? exp(-0.5e-5*atm_p)*0.7 : 0.7);

	int i;

	// solar/planetary environment (updated once per simulation second)
	if (simt > sr_updt + 1.0 || simt < sr_updt) {
		// compute solar irradiance at vessel position (0 if in shadow)
		H0 = SolarRadiation (&sdir);
		if (atm_p) H0 *= exp(-3.63e-6*atm_p);
		// compute IR irradiation from orbited body
		H1 = PlanetRadiation(&pdir);
		sr_updt = simt;
	}

	// time bookkeeping
	double t_target = simt + simdt;
	if (!std::isfinite (t_thermal) || t_target < t_thermal || t_target - t_thermal > 1.0e5) {
		t_thermal = simt;   // resynchronise (scenario load / time jump)
	}
	double dt = t_target - t_thermal;
	if (dt <= 0.0) {
		CheckTemperatureSanity();
		return;
	}

	if (dt <= THERMAL_DT_STABLE) {
		// ---- explicit path (small steps) -------------------------------
		coolantloop->UpdateExplicit (dt);
		double dQ[13];
		ComputeHeatFlux (dQ, false);
		for (i = 0; i < 13; i++) {
			if (cprm[i].mass > 1e-9)
				cprm[i].T += dQ[i] * dt / (cprm[i].mass * cprm[i].cp);
			cprm[i].T = max (cprm[i].T, T_CMB);   // CMB floor
		}
		nQuasiFail = 0;
	} else {
		// ---- half-implicit path (large steps) --------------------------
		if (!SolveImplicitStep (dt)) {
			// keep the previous state and freeze the thermal state; report if
			// the failure persists
			double Tmax = cprm[0].T;
			int cmax = 0;
			for (i = 1; i < 13; i++)
				if (cprm[i].T > Tmax) { Tmax = cprm[i].T; cmax = i; }
			if (++nQuasiFail >= 3)
				ReportNumericFailure (Tmax, cmax);
		} else {
			nQuasiFail = 0;
		}
	}

	t_thermal = t_target;
	CheckTemperatureSanity();
}

// --------------------------------------------------------------

// Accumulate the current net heat fluxes [W] for all compartments.
// includeCoolant: the heat flows of the coolant loop exchangers are included
// (used by the equilibrium solver; in the explicit path the coolant loop
// updates the reservoir temperatures directly instead).
void ThermalSubsystem::ComputeHeatFlux (double *dQ, bool includeCoolant)
{
	int i;
	for (i = 0; i < 13; i++) dQ[i] = 0.0;

	// irradiance
	if (H0) AddIrradiance (H0, sdir, dQ); // add solar irradiance
	if (H1) AddIrradiance (H1, pdir, dQ); // add planet IR irradiance
	if (H0 && H1) AddAlbedoReflection (H0, pdir, dQ); // add reflected planet irradiance

	// add internal heat generation
	bool active = (DG()->GetFlightStatus() & 1) == 0;
	dQ[AVIONICS] = (active ? 6e3 : 2e3); // simplistic power management for now

	// heat exchange from crew to cabin atmosphere (convection)
	double p = DG()->SubsysPressure()->PCabin();
	double dQ_crew = 1e-3 * p; // heat exchange is pressure-dependent
	dQ[CABIN] = dQ_crew; // pilot
	for (i = 0; i < 4; i++)
		if (DG()->psngr[i]) dQ[CABIN] += dQ_crew;

	// subtract black-body radiation
	SubtractBlackbodyRadiation (dQ);

	// atmospheric heat convection
	if (atm_p && atm_T)
		AtmosphericConvection (dQ);

	// internal heat conduction
	HeatConduction (dQ);

	// coolant loop heat flows (quasi-static, for the equilibrium solver)
	if (includeCoolant && coolantloop) {
		coolantloop->UpdateQuasiStatic ();
		coolantloop->AddHeatFlux (dQ);
	}
}

// --------------------------------------------------------------

// Solve the steady state  dT_i/dt = 0  (heat production = heat dissipation) for
// all compartments at the current environment.
//
// For each compartment, with the other compartments kept at their current
// temperature, the net heat flow is a monotonically decreasing function of its
// own temperature (black-body radiation, conduction and exchanger coupling all
// remove heat with increasing temperature). This allows a robust bracketed root
// search per compartment, iterated over the whole vessel (Gauss-Seidel).

// --------------------------------------------------------------

// Assemble the linearised backward-Euler system for one large step:
//
//   C_i (T_i - T_i^n)/dt = Q_i^ext + sum_j G_ij (T_j - T_i)
//                          - eps*sigma*A_i*T_i^4 + q_loop_i
//
// The black-body term is linearised around the current temperature and the
// coolant loop is represented by an effective conductance (from the NTU model,
// evaluated quasi-statically at the current state):
//
//   T_i^4    ~= (T^n)^4 + 4(T^n)^3 (T_i - T^n)
//   q_loop_i ~= g_i (T_cool,i - T_i)
//
// The result is a 13x13 linear system A*T = b.
void ThermalSubsystem::AssembleImplicitSystem (double dt, double *Amat, double *bvec)
{
	int i, j;
	for (i = 0; i < 13*13; i++) Amat[i] = 0.0;
	for (i = 0; i < 13; i++) bvec[i] = 0.0;

	// external irradiance and internal heat production (temperature independent)
	double dQ0[13];
	{
		for (i = 0; i < 13; i++) dQ0[i] = 0.0;
		if (H0) AddIrradiance (H0, sdir, dQ0);
		if (H1) AddIrradiance (H1, pdir, dQ0);
		if (H0 && H1) AddAlbedoReflection (H0, pdir, dQ0);
		bool active = (DG()->GetFlightStatus() & 1) == 0;
		dQ0[AVIONICS] = (active ? 6e3 : 2e3);
		double p = DG()->SubsysPressure()->PCabin();
		double dQ_crew = 1e-3 * p;
		dQ0[CABIN] = dQ_crew;
		for (i = 0; i < 4; i++)
			if (DG()->psngr[i]) dQ0[CABIN] += dQ_crew;
	}

	// ---- heat conduction between compartments (linear, symmetric) ---------
	// same conductance terms as HeatConduction(), assembled as -G_ij couplings
	{
		double G;
		// helper macro: add coupling between two compartments
#define ADD_CONDUCTION(ii, jj, g) \
		Amat[(ii)*13+(ii)] += (g); Amat[(jj)*13+(jj)] += (g); \
		Amat[(ii)*13+(jj)] -= (g); Amat[(jj)*13+(ii)] -= (g)

		// left/right wing surface <-> wing tank
		G = k_upper * Ay_wing;
		ADD_CONDUCTION (SURFUPPERLEFTWING, PROPELLANT_LEFTWING, G);
		G = k_lower * Ay_wing;
		ADD_CONDUCTION (SURFLOWERLEFTWING, PROPELLANT_LEFTWING, G);
		G = k_upper * Ay_wing;
		ADD_CONDUCTION (SURFUPPERRIGHTWING, PROPELLANT_RIGHTWING, G);
		G = k_lower * Ay_wing;
		ADD_CONDUCTION (SURFLOWERRIGHTWING, PROPELLANT_RIGHTWING, G);
		// fuselage surface <-> fuselage interior
		G = k_upper * (Ax_fuselage*2.0 + Ay_fuselage + Az_fuselage*2.0);
		ADD_CONDUCTION (SURFUPPERFUSELAGE, INTERIORFUSELAGE, G);
		G = k_lower * Ay_fuselage;
		ADD_CONDUCTION (SURFLOWERFUSELAGE, INTERIORFUSELAGE, G);
		// fuselage <-> wings
		G = k_upper * 3.0;
		ADD_CONDUCTION (SURFUPPERFUSELAGE, SURFUPPERLEFTWING, G);
		ADD_CONDUCTION (SURFUPPERFUSELAGE, SURFUPPERRIGHTWING, G);
		G = k_lower * 3.0;
		ADD_CONDUCTION (SURFLOWERFUSELAGE, SURFLOWERLEFTWING, G);
		ADD_CONDUCTION (SURFLOWERFUSELAGE, SURFLOWERRIGHTWING, G);
		// fuselage interior <-> interior tank
		G = k_upper * A_maintank;
		ADD_CONDUCTION (INTERIORFUSELAGE, PROPELLANT_MAIN, G);
		// fuselage interior <-> cabin
		G = k_cabin * A_cabin * cprm[CABIN].mass_air;
		ADD_CONDUCTION (INTERIORFUSELAGE, CABIN, G);
		// fuselage interior <-> avionics
		G = k_upper * A_avionics;
		ADD_CONDUCTION (INTERIORFUSELAGE, AVIONICS, G);
		// cabin <-> avionics
		G = k_cabin * A_avionics * cprm[CABIN].mass_air;
		ADD_CONDUCTION (CABIN, AVIONICS, G);
		// radiator <-> fuselage exterior (stowed only)
		if (RadiatorState().State() == 0.0) {
			G = 0.02 * A_radiatorpanel2;
			ADD_CONDUCTION (RADIATOR, SURFUPPERFUSELAGE, G);
		}
#undef ADD_CONDUCTION
	}

	// ---- atmospheric convection (linear, to a fixed ambient temperature) ---
	if (atm_p && atm_T) {
		double g;
		g = k_convect * atm_p * Ay_fuselage;
		Amat[SURFLOWERFUSELAGE*13+SURFLOWERFUSELAGE] += g;
		bvec[SURFLOWERFUSELAGE] += g * atm_T;
		g = k_convect * atm_p * (Ay_fuselage + 2.0*Ax_fuselage + 2.0*Az_fuselage);
		Amat[SURFUPPERFUSELAGE*13+SURFUPPERFUSELAGE] += g;
		bvec[SURFUPPERFUSELAGE] += g * atm_T;
		g = k_convect * atm_p * Ay_wing;
		Amat[SURFUPPERLEFTWING*13+SURFUPPERLEFTWING] += g;
		Amat[SURFLOWERLEFTWING*13+SURFLOWERLEFTWING] += g;
		Amat[SURFUPPERRIGHTWING*13+SURFUPPERRIGHTWING] += g;
		Amat[SURFLOWERRIGHTWING*13+SURFLOWERRIGHTWING] += g;
		bvec[SURFUPPERLEFTWING] += g * atm_T;
		bvec[SURFLOWERLEFTWING] += g * atm_T;
		bvec[SURFUPPERRIGHTWING] += g * atm_T;
		bvec[SURFLOWERRIGHTWING] += g * atm_T;
		double rstate = RadiatorState().State();
		if (rstate > 0.0) {
			double A_rad = A_radiatorpanel2 + 4.0*A_radiatorpanel1;
			g = k_convect * atm_p * A_rad;
			Amat[RADIATOR*13+RADIATOR] += g;
			bvec[RADIATOR] += g * atm_T;
		}
		// cabin convection through open hatch: handled as a linear exchange to
		// the ambient temperature (same gate as AtmosphericConvection)
		const PressureSubsystem *pssys = DG()->SubsysPressure();
		if (pssys->HatchState().IsOpen() || (pssys->OLockState().IsOpen() && pssys->ILockState().IsOpen())) {
			g = k_convect * atm_p * cprm[CABIN].mass_air;
			Amat[CABIN*13+CABIN] += g;
			bvec[CABIN] += g * atm_T;
		}
	}

	// ---- black-body radiation (linearised) --------------------------------
	for (i = 0; i < 13; i++) {
		double A_rad = 0.0, eps_rad = 0.0;
		switch (i) {
		case SURFUPPERFUSELAGE:
			A_rad = Ax_fuselage*2.0 + Ay_fuselage + Az_fuselage*2.0; eps_rad = eps; break;
		case SURFLOWERFUSELAGE:
			A_rad = Ay_fuselage; eps_rad = eps; break;
		case SURFUPPERLEFTWING: case SURFLOWERLEFTWING:
		case SURFUPPERRIGHTWING: case SURFLOWERRIGHTWING:
			A_rad = Ay_wing; eps_rad = eps; break;
		case RADIATOR:
			if (RadiatorState().State() > 0.0) {
				A_rad = (A_radiatorpanel2 + 2.0*A_radiatorpanel1*1.4) * RadiatorState().State();
				eps_rad = eps_radiator;
			}
			break;
		default:
			break;
		}
		if (A_rad > 0.0) {
			double Tn = max (cprm[i].T, T_CMB);
			double T4 = Tn*Tn*Tn*Tn;
			double dT4 = 4.0*Tn*Tn*Tn;
			double k_rad = eps_rad * sigma * A_rad;
			// -k*T^4 ~= -k*(T4 + dT4*(T - Tn)) = -k*dT4*T + k*(dT4*Tn - T4)
			Amat[i*13+i] += k_rad * dT4;
			bvec[i] += k_rad * (dT4*Tn - T4);
		}
	}

	// ---- coolant loop: equivalent conductance to the loop temperature ------
	{
		double g_eff[13], T_in[13];
		coolantloop->GetEffectiveConductance (g_eff, T_in);
		for (i = 0; i < 13; i++) {
			if (g_eff[i] > 0.0) {
				Amat[i*13+i] += g_eff[i];
				bvec[i] += g_eff[i] * T_in[i];
			}
		}
	}

	// ---- external heat fluxes (irradiance, internal generation) ------------
	for (i = 0; i < 13; i++) bvec[i] += dQ0[i];

	// ---- heat capacity term: C_i*(T_i - T_i^n)/dt -------------------------
	for (i = 0; i < 13; i++) {
		double C = cprm[i].mass * cprm[i].cp;
		if (C < 1.0) C = 1.0;   // guard (all compartments have a finite mass)
		double g = C / dt;
		Amat[i*13+i] += g;
		bvec[i] += g * cprm[i].T;
	}
}

// --------------------------------------------------------------

// Solve the 13x13 linear system A*x = b (Gaussian elimination with partial
// pivoting). A is stored row-major (13x13).
bool ThermalSubsystem::SolveLinear13 (const double *Amat, const double *bvec, double *x)
{
	double A[13][13], b[13];
	int i, j, k;
	for (i = 0; i < 13; i++) {
		b[i] = bvec[i];
		for (j = 0; j < 13; j++) A[i][j] = Amat[i*13+j];
	}

	for (k = 0; k < 13; k++) {
		// pivot
		int p = k;
		double pmax = fabs (A[k][k]);
		for (i = k+1; i < 13; i++) {
			double v = fabs (A[i][k]);
			if (v > pmax) { pmax = v; p = i; }
		}
		if (pmax < 1e-30) return false;
		if (p != k) {
			for (j = 0; j < 13; j++) { double t = A[k][j]; A[k][j] = A[p][j]; A[p][j] = t; }
			double t = b[k]; b[k] = b[p]; b[p] = t;
		}
		double d = A[k][k];
		for (i = k+1; i < 13; i++) {
			double f = A[i][k] / d;
			if (f == 0.0) continue;
			for (j = k; j < 13; j++) A[i][j] -= f * A[k][j];
			b[i] -= f * b[k];
		}
	}
	for (i = 12; i >= 0; i--) {
		double s = b[i];
		for (j = i+1; j < 13; j++) s -= A[i][j] * x[j];
		x[i] = s / A[i][i];
	}
	for (i = 0; i < 13; i++)
		if (!std::isfinite (x[i])) return false;
	return true;
}

// --------------------------------------------------------------

// One half-implicit (backward Euler) step of length dt. The system is
// linearised twice (the second pass uses the updated temperatures as the
// linearisation point for the T^4 term).
bool ThermalSubsystem::SolveImplicitStep (double dt)
{
	double Amat[13*13], bvec[13], x[13];
	double Told[13];
	int i;

	for (i = 0; i < 13; i++) Told[i] = cprm[i].T;

	for (int pass = 0; pass < 2; pass++) {
		AssembleImplicitSystem (dt, Amat, bvec);
		if (!SolveLinear13 (Amat, bvec, x)) {
			for (i = 0; i < 13; i++) cprm[i].T = Told[i];
			return false;
		}
		for (i = 0; i < 13; i++)
			cprm[i].T = max (T_CMB, min (T_MAX_SANE, x[i]));
	}
	return true;
}

// --------------------------------------------------------------

// Detect non-finite or out-of-range compartment temperatures. Repairs the
// temperatures (so that a corrupted value cannot be stored in a scenario) and
// reports a numerical failure to the wreck/destruction system (once per vessel).
bool ThermalSubsystem::CheckTemperatureSanity ()
{
	bool bad = false;
	double Tmax = -1e30;
	int cmax = 0;

	for (int i = 0; i < 13; i++) {
		double T = cprm[i].T;
		if (!std::isfinite (T) || T < 0.0 || T > T_MAX_SANE) {
			bad = true;
			if (!std::isfinite (T)) T = T_CMB;
			cprm[i].T = max (T_CMB, min (T_MAX_SANE, T));
		}
		if (cprm[i].T > Tmax) { Tmax = cprm[i].T; cmax = i; }
	}
	if (bad && !bNumericFailureReported)
		ReportNumericFailure (Tmax, cmax);
	return !bad;
}

// --------------------------------------------------------------

void ThermalSubsystem::ReportNumericFailure (double Tmax, int compartment)
{
	if (bNumericFailureReported) return;
	bNumericFailureReported = true;
	oapiWreckReportNumericFailure (DG()->GetHandle (), Tmax, compartment);
	oapiWriteLogV ("THERMAL: numerical failure reported (max T=%.1f K, compartment %d)", Tmax, compartment);
}

// ==============================================================
// Coolant loop extensions
// ==============================================================

void CoolantLoop::ComputeSplits ()
{
	// set the wing tank bypass rate
	double Ta = node[MERGER_WINGBYPASS].upstream[0]->T1;
	double Tb = node[MERGER_WINGBYPASS].upstream[1]->T1;
	node[SPLITTER_WINGBYPASS].split = ((Ta != Tb) ? min (1.0, max (0.0, (Tref_tgt-Tb)/(Ta-Tb))) : 1.0);

	// set the heatsink bypass rate
	Ta = node[MERGER_HEATSINKBYPASS].upstream[0]->T1;
	Tb = node[MERGER_HEATSINKBYPASS].upstream[1]->T1;
	node[SPLITTER_HEATSINKBYPASS].split = ((Ta != Tb) ? min(1.0, max(0.0, (Tref_tgt-Tb)/(Ta-Tb))) : 1.0);
}

// --------------------------------------------------------------

void CoolantLoop::UpdateExplicit (double dt)
{
	node[EXCHANGER_CABIN].k = node[EXCHANGER_CABIN].cprm->mass_air*3.7;
	ComputeSplits ();
	// update all nodes except the pump first, then the pump (so that the pump
	// sees the same-frame cold-plate exit temperature)
	for (int i = 1; i < nnode; i++)
		node[i].Update (dt);
	node[PUMP].Update (dt);
}

// --------------------------------------------------------------

void CoolantLoop::UpdateQuasiStatic ()
{
	node[EXCHANGER_CABIN].k = node[EXCHANGER_CABIN].cprm->mass_air*3.7;
	ComputeSplits ();
	// dt=0: node temperatures and exchanger heat flows are updated, but the
	// reservoir temperatures are unchanged
	for (int i = 1; i < nnode; i++)
		node[i].Update (0.0);
	node[PUMP].Update (0.0);
}

// --------------------------------------------------------------

void CoolantLoop::AddHeatFlux (double *compartmentQ) const
{
	// each exchanger node holds the heat flow from its reservoir into the
	// coolant [W]; add it to the net heat flow of the connected compartment
	for (int i = 0; i < nnode; i++) {
		if (node[i].nodetype != NodeParam::EXCHANGER || !node[i].cprm) continue;
		for (int c = 0; c < 13; c++) {
			if (node[i].cprm == &ssys_th->cprm[c]) {
				// Q is positive when heat flows from the reservoir into the
				// coolant, so the net heat gain of the reservoir is -Q
				compartmentQ[c] -= node[i].Q;
				break;
			}
		}
	}
}

// --------------------------------------------------------------

// Effective linear conductance between each compartment and the coolant loop
// (used by the half-implicit solver). The values follow from the NTU exchanger
// model evaluated with the current flow rates and splits:
//     g = (1-exp(-k/(m_dot*cp))) * m_dot*cp     [W/K]
// and the temperature of the coolant entering the exchanger is taken from the
// quasi-static loop solution.
void CoolantLoop::GetEffectiveConductance (double *g_eff, double *T_in)
{
	int c;
	for (c = 0; c < 13; c++) { g_eff[c] = 0.0; T_in[c] = 293.0; }

	UpdateQuasiStatic ();

	for (int i = 0; i < nnode; i++) {
		if (node[i].nodetype != NodeParam::EXCHANGER || !node[i].cprm) continue;
		double flowrate = node[i].Flowrate (&node[i]);
		double g = 0.0;
		if (flowrate > 0.0) {
			double efficiency = flowrate * cp;
			double dTrel = exp (-node[i].k / efficiency);
			g = (1.0 - dTrel) * efficiency;
		}
		for (c = 0; c < 13; c++) {
			if (node[i].cprm == &ssys_th->cprm[c]) {
				g_eff[c] += g;
				T_in[c] = (g > 0.0 ? node[i].T0 : ssys_th->cprm[c].T);
				break;
			}
		}
	}
}

// --------------------------------------------------------------

// Scenario save/load helpers for the coolant loop node temperatures. The node
// temperatures are written on a separate line for backward compatibility:
// older scenarios simply keep the default node temperatures (293 K).

void CoolantLoop::SaveNodeTemperatures (FILEHANDLE scn)
{
	char nbuf[2048];
	int n = 0;
	for (int i = 0; i < nnode && n < 1900; i++)
		n += sprintf (nbuf+n, "%s%0.3lf %0.3lf", (i ? " " : ""), node[i].T0, node[i].T1);
	oapiWriteScenario_string (scn, (char*)"COOLANT_NODES", nbuf);
}

// --------------------------------------------------------------

void CoolantLoop::ParseNodeTemperatures (const char *line)
{
	double v[24];
	int got = sscanf (line+13,
		"%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
		v+0,  v+1,  v+2,  v+3,  v+4,  v+5,  v+6,  v+7,
		v+8,  v+9,  v+10, v+11, v+12, v+13, v+14, v+15,
		v+16, v+17, v+18, v+19, v+20, v+21, v+22, v+23);
	for (int i = 0; i < nnode; i++) {
		if (2*i+1 < got) {
			node[i].T0 = max (1.0, min (10000.0, v[2*i]));
			node[i].T1 = max (1.0, min (10000.0, v[2*i+1]));
		}
	}
}

// ==============================================================
