// -----------------------------------------------------------------------------
// WreckConfig.inc
// Wreck/debris system configuration parameters: load/write and
// Config::GetParam support.
//
// Kept separate from RelativityConfig.inc: the wreck system is independent of
// the relativity extension. Included at the end of the official Config.cpp.
// -----------------------------------------------------------------------------

#include "WreckAPI.h"	// CFGPRM_* (usually already available through OrbiterAPI.h)

void Config::WreckLoad (std::istream &ifs)
{
	GetBool (ifs, "Wrecks", CfgPhysicsPrm.bWrecks);
	double d;
	// historical key names are kept for compatibility (the wreck system used to
	// be part of the relativity extension)
	if (GetReal (ifs, "RelativityCollisionDVThreshold", d))
		CfgPhysicsPrm.RelCollisionDVThreshold = max (1.0, min (1e6, d));
	if (GetReal (ifs, "RelativityGasGiantFactor", d))
		CfgPhysicsPrm.RelGasGiantFactor = max (0.1, min (1.0, d));
}

void Config::WreckWritePhysics (std::ostream &ofs, bool bEchoAll) const
{
	if (CfgPhysicsPrm.bWrecks != CfgPhysicsPrm_default.bWrecks || bEchoAll)
		ofs << "Wrecks = " << BoolStr (CfgPhysicsPrm.bWrecks) << '\n';
	if (CfgPhysicsPrm.RelCollisionDVThreshold != CfgPhysicsPrm_default.RelCollisionDVThreshold || bEchoAll)
		ofs << "RelativityCollisionDVThreshold = " << CfgPhysicsPrm.RelCollisionDVThreshold << '\n';
	if (CfgPhysicsPrm.RelGasGiantFactor != CfgPhysicsPrm_default.RelGasGiantFactor || bEchoAll)
		ofs << "RelativityGasGiantFactor = " << CfgPhysicsPrm.RelGasGiantFactor << '\n';
}

const void *Config::GetWreckParam (DWORD paramtype) const
{
	switch (paramtype) {
	case CFGPRM_RELCOLLISIONDV: return (void*)&CfgPhysicsPrm.RelCollisionDVThreshold;
	case CFGPRM_RELGASGIANT:    return (void*)&CfgPhysicsPrm.RelGasGiantFactor;
	case CFGPRM_WRECKS:         return (void*)&CfgPhysicsPrm.bWrecks;
	}
	return 0;
}
