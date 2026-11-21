// ==============================================================
// ThermalModelMembers_CoolantLoop.inc - project extension member declarations for class CoolantLoop.
// Included inside the class body in the official header: the official
// header only grows by the include line.
// ==============================================================

public:
	// Coolant loop support (project extension, implemented in ThermalSolver.inc).
	// ComputeSplits: control of the reference temperature
	// UpdateExplicit: update the node temperatures and reservoir temperatures
	// UpdateQuasiStatic: update node temperatures and exchanger heat flows only
	// AddHeatFlux: add the exchanger heat flows to the compartment heat flux array
	// GetEffectiveConductance: NTU-based linear coupling for the implicit solver
	void ComputeSplits ();
	void UpdateExplicit (double dt);
	void UpdateQuasiStatic ();
	void AddHeatFlux (double *compartmentQ) const;
	void GetEffectiveConductance (double *g_eff, double *T_in);
	// scenario save/load of the node temperatures
	void SaveNodeTemperatures (FILEHANDLE scn);
	void ParseNodeTemperatures (const char *line);

private:

		double Q;           // exchanger heat flow from the reservoir into the coolant [W]
		                    // (computed by Update; used by the quasi-static path)

