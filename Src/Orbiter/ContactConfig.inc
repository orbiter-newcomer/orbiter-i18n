// ==============================================================
// ContactConfig.inc
// Contact model configuration: load/write and Config::GetParam support.
//
// Included at the end of the official Config.cpp. Kept in a separate file so
// that the changes to the official Orbiter sources are limited to a single
// include line and three call sites.
//
// Two independent parameter sets are available: one for the landing gear
// contact points (the first three touchdown vertices) and one for the hull /
// body contact points (all remaining vertices). A value of 0 means "use the
// original value of the contact point".
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

	// landing gear contact parameters (absolute values; 0 = original)
	if (GetReal (ifs, "ContactGearStiffness", d))
		CfgPhysicsPrm.ContactGearStiffness = (d < 1e3 ? 0.0 : (d > 1e9 ? 1e9 : d));
	if (GetReal (ifs, "ContactGearDamping", d))
		CfgPhysicsPrm.ContactGearDamping = (d < 0.0 ? 0.0 : (d > 1e10 ? 1e10 : d));
	if (GetReal (ifs, "ContactGearFriction", d))
		CfgPhysicsPrm.ContactGearFriction = (d < 0.0 ? 0.0 : (d > 1e6 ? 1e6 : d));

	// hull/body contact parameters (absolute values; 0 = original)
	if (GetReal (ifs, "ContactHullStiffness", d))
		CfgPhysicsPrm.ContactHullStiffness = (d < 1e3 ? 0.0 : (d > 1e9 ? 1e9 : d));
	if (GetReal (ifs, "ContactHullDamping", d))
		CfgPhysicsPrm.ContactHullDamping = (d < 0.0 ? 0.0 : (d > 1e10 ? 1e10 : d));
	if (GetReal (ifs, "ContactHullFriction", d))
		CfgPhysicsPrm.ContactHullFriction = (d < 0.0 ? 0.0 : (d > 1e6 ? 1e6 : d));
}

void Config::ContactWritePhysics (std::ostream &ofs, bool bEchoAll) const
{
	if (CfgPhysicsPrm.ContactFrictionModel != CfgPhysicsPrm_default.ContactFrictionModel || bEchoAll)
		ofs << "ContactFrictionModel = " << CfgPhysicsPrm.ContactFrictionModel << '\n';
	if (CfgPhysicsPrm.ContactSolver != CfgPhysicsPrm_default.ContactSolver || bEchoAll)
		ofs << "ContactSolver = " << CfgPhysicsPrm.ContactSolver << '\n';
	if (CfgPhysicsPrm.ContactGearStiffness != CfgPhysicsPrm_default.ContactGearStiffness || bEchoAll)
		ofs << "ContactGearStiffness = " << CfgPhysicsPrm.ContactGearStiffness << '\n';
	if (CfgPhysicsPrm.ContactGearDamping != CfgPhysicsPrm_default.ContactGearDamping || bEchoAll)
		ofs << "ContactGearDamping = " << CfgPhysicsPrm.ContactGearDamping << '\n';
	if (CfgPhysicsPrm.ContactGearFriction != CfgPhysicsPrm_default.ContactGearFriction || bEchoAll)
		ofs << "ContactGearFriction = " << CfgPhysicsPrm.ContactGearFriction << '\n';
	if (CfgPhysicsPrm.ContactHullStiffness != CfgPhysicsPrm_default.ContactHullStiffness || bEchoAll)
		ofs << "ContactHullStiffness = " << CfgPhysicsPrm.ContactHullStiffness << '\n';
	if (CfgPhysicsPrm.ContactHullDamping != CfgPhysicsPrm_default.ContactHullDamping || bEchoAll)
		ofs << "ContactHullDamping = " << CfgPhysicsPrm.ContactHullDamping << '\n';
	if (CfgPhysicsPrm.ContactHullFriction != CfgPhysicsPrm_default.ContactHullFriction || bEchoAll)
		ofs << "ContactHullFriction = " << CfgPhysicsPrm.ContactHullFriction << '\n';
}

const void *Config::GetContactParam (DWORD paramtype) const
{
	switch (paramtype) {
	case CFGPRM_CONTACTFRICTION: return (void*)&CfgPhysicsPrm.ContactFrictionModel;
	case CFGPRM_CONTACTSOLVER:   return (void*)&CfgPhysicsPrm.ContactSolver;
	}
	return 0;
}
