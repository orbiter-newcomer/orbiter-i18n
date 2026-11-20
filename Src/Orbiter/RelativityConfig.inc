// -----------------------------------------------------------------------------
// Relativity configuration parameters: load/write and Config::GetParam support.
//
// Implementation moved out of the official Config.cpp (included at the end of
// that file). Declarations live in Config.h.
// -----------------------------------------------------------------------------

// The official Config::GetString() matches configuration keys by prefix, so a
// query for "Relativity" also matches "RelativityDoppler", "RelativityLensing",
// ... depending on the line order in the file. The relativity parameters are
// therefore loaded with an exact key comparison.
static bool RelativityGetStringExact (std::istream &ifs, const char *key, char *val, size_t valsz)
{
	char line[512];
	int i, last;
	val[0] = 0;
	ifs.clear();
	ifs.seekg (0, std::ios::beg);
	while (ifs.getline (line, sizeof(line))) {
		for (i = 0; line[i] && line[i] != ';'; i++);	// cut comment
		line[i] = 0;
		for (i = 0; line[i] && line[i] != '='; i++);	// split key/value
		if (!line[i]) continue;
		line[i] = 0;
		i++;											// value start
		for (last = (int)strlen(line)-1; last >= 0 && (line[last]==' '||line[last]=='\t'); last--);
		line[last+1] = 0;
		if (_stricmp (line, key)) continue;
		while (line[i] == ' ' || line[i] == '\t') i++;
		snprintf (val, valsz, "%s", line+i);
		return true;
	}
	ifs.clear();
	return false;
}

static bool RelativityGetBoolExact (std::istream &ifs, const char *key, bool &val)
{
	char buf[64];
	if (!RelativityGetStringExact (ifs, key, buf, sizeof(buf))) return false;
	if (!_strnicmp (buf, "true", 4)) { val = true;  return true; }
	if (!_strnicmp (buf, "false", 5)) { val = false; return true; }
	return false;
}

void Config::RelativityLoad (std::istream &ifs)
{
	RelativityGetBoolExact (ifs, "Relativity", CfgPhysicsPrm.bRelativity);
	GetInt (ifs, "RelativityGravityMethod", CfgPhysicsPrm.RelativityGravityMethod);
	GetInt (ifs, "RelativityFTLMode", CfgPhysicsPrm.RelativityFTLMode);
	RelativityGetBoolExact (ifs, "RelativityCharge", CfgPhysicsPrm.bRelativisticCharge);
	double d;
	// relativistic visual effects
	RelativityGetBoolExact (ifs, "RelativityDoppler", CfgVisualPrm.bRelDoppler);
	RelativityGetBoolExact (ifs, "RelativityAberration", CfgVisualPrm.bRelAberration);
	RelativityGetBoolExact (ifs, "RelativityLensing", CfgVisualPrm.bRelLensing);
	RelativityGetBoolExact (ifs, "RelativityLensWireframe", CfgVisualPrm.bRelLensWireframe);
	RelativityGetBoolExact (ifs, "RelativityGravRedshift", CfgVisualPrm.bRelGravRedshift);
	RelativityGetBoolExact (ifs, "RelativityBodyMarkers", CfgVisualPrm.bRelBodyMarkers);
	if (GetReal (ifs, "RelativityChargeRatio", d))
		CfgPhysicsPrm.RelChargeRatio = max (0.0, min (1.0, d));
	// Wreck system parameters (collision impact threshold, gas-giant crushing
	// factor) are loaded by Config::WreckLoad (WreckConfig.inc, Src/Wreck).
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
		if (CfgPhysicsPrm.bRelativisticCharge != CfgPhysicsPrm_default.bRelativisticCharge || bEchoAll)
		ofs << "RelativityCharge = " << BoolStr (CfgPhysicsPrm.bRelativisticCharge) << '\n';
	if (CfgPhysicsPrm.RelChargeRatio != CfgPhysicsPrm_default.RelChargeRatio || bEchoAll)
		ofs << "RelativityChargeRatio = " << CfgPhysicsPrm.RelChargeRatio << '\n';
	// Wreck system parameters are written by Config::WreckWritePhysics (WreckConfig.inc).
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
	case CFGPRM_RELCHARGE:
		return (void*)&CfgPhysicsPrm.bRelativisticCharge;
	case CFGPRM_RELCHARGERATIO:
		return (void*)&CfgPhysicsPrm.RelChargeRatio;
	case CFGPRM_RELGRAVMETHOD:
		return (void*)&CfgPhysicsPrm.RelativityGravityMethod;
	// Wreck system parameters are handled by Config::GetWreckParam (WreckConfig.inc).
	}
	return 0;
}
