// -----------------------------------------------------------------------------
// Relativity configuration parameters: load/write and Config::GetParam support.
//
// Implementation moved out of the official Config.cpp (included at the end of
// that file). Declarations live in Config.h.
// -----------------------------------------------------------------------------

void Config::RelativityLoad (std::istream &ifs)
{
	GetBool (ifs, "Relativity", CfgPhysicsPrm.bRelativity);
	GetInt (ifs, "RelativityGravityMethod", CfgPhysicsPrm.RelativityGravityMethod);
	GetInt (ifs, "RelativityFTLMode", CfgPhysicsPrm.RelativityFTLMode);
	double d;
	// relativistic visual effects
	GetBool (ifs, "RelativityDoppler", CfgVisualPrm.bRelDoppler);
	GetBool (ifs, "RelativityAberration", CfgVisualPrm.bRelAberration);
	GetBool (ifs, "RelativityLensing", CfgVisualPrm.bRelLensing);
	GetBool (ifs, "RelativityLensWireframe", CfgVisualPrm.bRelLensWireframe);
	GetBool (ifs, "RelativityGravRedshift", CfgVisualPrm.bRelGravRedshift);
	GetBool (ifs, "RelativityBodyMarkers", CfgVisualPrm.bRelBodyMarkers);
	if (GetReal (ifs, "RelativityExposure", d))
		CfgVisualPrm.RelExposureWhite = max (0.0, min (8.0, d));
}

void Config::RelativityWriteVisual (std::ostream &ofs, bool bEchoAll) const
{
		if (CfgVisualPrm.bRelDoppler != CfgVisualPrm_default.bRelDoppler || bEchoAll)
			ofs << "RelativityDoppler = " << BoolStr (CfgVisualPrm.bRelDoppler) << '\n';
		if (CfgVisualPrm.bRelAberration != CfgVisualPrm_default.bRelAberration || bEchoAll)
			ofs << "RelativityAberration = " << BoolStr (CfgVisualPrm.bRelAberration) << '\n';
		if (CfgVisualPrm.bRelLensing != CfgVisualPrm_default.bRelLensing || bEchoAll)
			ofs << "RelativityLensing = " << BoolStr (CfgVisualPrm.bRelLensing) << '\n';
		if (CfgVisualPrm.bRelLensWireframe != CfgVisualPrm_default.bRelLensWireframe || bEchoAll)
			ofs << "RelativityLensWireframe = " << BoolStr (CfgVisualPrm.bRelLensWireframe) << '\n';		if (CfgVisualPrm.bRelGravRedshift != CfgVisualPrm_default.bRelGravRedshift || bEchoAll)
			ofs << "RelativityGravRedshift = " << BoolStr (CfgVisualPrm.bRelGravRedshift) << '\n';
		if (CfgVisualPrm.bRelBodyMarkers != CfgVisualPrm_default.bRelBodyMarkers || bEchoAll)
			ofs << "RelativityBodyMarkers = " << BoolStr (CfgVisualPrm.bRelBodyMarkers) << '\n';
		if (CfgVisualPrm.RelExposureWhite != CfgVisualPrm_default.RelExposureWhite || bEchoAll)
			ofs << "RelativityExposure = " << CfgVisualPrm.RelExposureWhite << '\n';
}

void Config::RelativityWritePhysics (std::ostream &ofs, bool bEchoAll) const
{
		if (CfgPhysicsPrm.bRelativity != CfgPhysicsPrm_default.bRelativity || bEchoAll)
			ofs << "Relativity = " << BoolStr (CfgPhysicsPrm.bRelativity) << '\n';
		if (CfgPhysicsPrm.RelativityGravityMethod != CfgPhysicsPrm_default.RelativityGravityMethod || bEchoAll)
			ofs << "RelativityGravityMethod = " << CfgPhysicsPrm.RelativityGravityMethod << '\n';
		if (CfgPhysicsPrm.RelativityFTLMode != CfgPhysicsPrm_default.RelativityFTLMode || bEchoAll)
			ofs << "RelativityFTLMode = " << CfgPhysicsPrm.RelativityFTLMode << '\n';
}

const void *Config::GetRelativityParam (DWORD paramtype) const
{
	switch (paramtype) {
	case CFGPRM_RELDOPPLER:
		return (void*)&CfgVisualPrm.bRelDoppler;
	case CFGPRM_RELABERRATION:
		return (void*)&CfgVisualPrm.bRelAberration;
	case CFGPRM_RELLENSING:
		return (void*)&CfgVisualPrm.bRelLensing;
	case CFGPRM_RELLENSWIREFRAME:
		return (void*)&CfgVisualPrm.bRelLensWireframe;	case CFGPRM_RELGRAVREDSHIFT:
		return (void*)&CfgVisualPrm.bRelGravRedshift;
	case CFGPRM_RELBODYMARKERS:
		return (void*)&CfgVisualPrm.bRelBodyMarkers;
	case CFGPRM_RELEXPOSURE:
		return (void*)&CfgVisualPrm.RelExposureWhite;
	case CFGPRM_PHYSRELATIVITY:
		return (void*)&CfgPhysicsPrm.bRelativity;
	case CFGPRM_RELGRAVMETHOD:
		return (void*)&CfgPhysicsPrm.RelativityGravityMethod;
	}
	return 0;
}
