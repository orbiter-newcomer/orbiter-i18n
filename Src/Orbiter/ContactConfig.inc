// ==============================================================
// ContactConfig.inc
// Contact model configuration: load/write and Config::GetParam support.
//
// Included at the end of the official Config.cpp. Kept in a separate file so
// that the changes to the official Orbiter sources are limited to a single
// include line and three call sites.
//
// Stiffness and friction always use the original per-point values. Only the
// damping can be overridden, as a damping ratio zeta which is converted to a
// damping coefficient at runtime: c = 2*zeta*sqrt(k*m). A value of 0 keeps the
// original damping of the contact point.
//
// Two independent damping ratios are available: one for the landing gear
// contact points (the first three touchdown vertices) and one for the hull /
// body contact points (all remaining vertices).
// ==============================================================

#include "ContactAPI.h"

void Config::ContactLoad (std::istream &ifs)
{
	int i;
	double d;
	if (GetInt (ifs, "ContactFrictionModel", i))
		CfgPhysicsPrm.ContactFrictionModel = (i != 0 ? 1 : 0);
	if (GetInt (ifs, "ContactSolver", i))
		CfgPhysicsPrm.ContactSolver = (i < 0 ? 0 : (i > 2 ? 2 : i));

	if (GetReal (ifs, "ContactGearDampingRatio", d))
		CfgPhysicsPrm.ContactGearDampingRatio = (d < 0.0 ? 0.0 : (d > 20.0 ? 20.0 : d));
	if (GetReal (ifs, "ContactHullDampingRatio", d))
		CfgPhysicsPrm.ContactHullDampingRatio = (d < 0.0 ? 0.0 : (d > 20.0 ? 20.0 : d));
}

void Config::ContactWritePhysics (std::ostream &ofs, bool bEchoAll) const
{
	if (CfgPhysicsPrm.ContactFrictionModel != CfgPhysicsPrm_default.ContactFrictionModel || bEchoAll)
		ofs << "ContactFrictionModel = " << CfgPhysicsPrm.ContactFrictionModel << '\n';
	if (CfgPhysicsPrm.ContactSolver != CfgPhysicsPrm_default.ContactSolver || bEchoAll)
		ofs << "ContactSolver = " << CfgPhysicsPrm.ContactSolver << '\n';
	if (CfgPhysicsPrm.ContactGearDampingRatio != CfgPhysicsPrm_default.ContactGearDampingRatio || bEchoAll)
		ofs << "ContactGearDampingRatio = " << CfgPhysicsPrm.ContactGearDampingRatio << '\n';
	if (CfgPhysicsPrm.ContactHullDampingRatio != CfgPhysicsPrm_default.ContactHullDampingRatio || bEchoAll)
		ofs << "ContactHullDampingRatio = " << CfgPhysicsPrm.ContactHullDampingRatio << '\n';
}

const void *Config::GetContactParam (DWORD paramtype) const
{
	switch (paramtype) {
	case CFGPRM_CONTACTFRICTION: return (void*)&CfgPhysicsPrm.ContactFrictionModel;
	case CFGPRM_CONTACTSOLVER:   return (void*)&CfgPhysicsPrm.ContactSolver;
	}
	return 0;
}
