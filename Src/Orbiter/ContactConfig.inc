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
// body contact points (all remaining vertices). Each set has three parameters:
//
//   stiffness    absolute contact stiffness [N/m]  (0 = original value)
//   damping      damping RATIO zeta                (0 = original damping)
//   friction     absolute friction coefficient     (0 = original value)
//
// The damping ratio is converted to a damping coefficient at runtime:
//     c = 2*zeta*sqrt(k*m)
// where k is the effective stiffness of the contact point and m the vessel mass.
// ==============================================================

#include "ContactAPI.h"

void Config::ContactLoad (std::istream &ifs)
{
	int i;
	double d;
	if (GetInt (ifs, "ContactSolver", i))
		CfgPhysicsPrm.ContactSolver = (i < 0 ? 0 : (i > 2 ? 2 : i));

	// landing gear contact parameters
	if (GetReal (ifs, "ContactGearStiffness", d))
		CfgPhysicsPrm.ContactGearStiffness = (d < 1e3 ? 0.0 : (d > 1e9 ? 1e9 : d));
	if (GetReal (ifs, "ContactGearDampingRatio", d))
		CfgPhysicsPrm.ContactGearDampingRatio = (d < 0.0 ? 0.0 : (d > 20.0 ? 20.0 : d));
	if (GetReal (ifs, "ContactGearFriction", d))
		CfgPhysicsPrm.ContactGearFriction = (d < 0.0 ? 0.0 : (d > 1e6 ? 1e6 : d));

	// hull/body contact parameters
	if (GetReal (ifs, "ContactHullStiffness", d))
		CfgPhysicsPrm.ContactHullStiffness = (d < 1e3 ? 0.0 : (d > 1e9 ? 1e9 : d));
	if (GetReal (ifs, "ContactHullDampingRatio", d))
		CfgPhysicsPrm.ContactHullDampingRatio = (d < 0.0 ? 0.0 : (d > 20.0 ? 20.0 : d));
	if (GetReal (ifs, "ContactHullFriction", d))
		CfgPhysicsPrm.ContactHullFriction = (d < 0.0 ? 0.0 : (d > 1e6 ? 1e6 : d));
}

void Config::ContactWritePhysics (std::ostream &ofs, bool bEchoAll) const
{
	if (CfgPhysicsPrm.ContactSolver != CfgPhysicsPrm_default.ContactSolver || bEchoAll)
		ofs << "ContactSolver = " << CfgPhysicsPrm.ContactSolver << '\n';
	if (CfgPhysicsPrm.ContactGearStiffness != CfgPhysicsPrm_default.ContactGearStiffness || bEchoAll)
		ofs << "ContactGearStiffness = " << CfgPhysicsPrm.ContactGearStiffness << '\n';
	if (CfgPhysicsPrm.ContactGearDampingRatio != CfgPhysicsPrm_default.ContactGearDampingRatio || bEchoAll)
		ofs << "ContactGearDampingRatio = " << CfgPhysicsPrm.ContactGearDampingRatio << '\n';
	if (CfgPhysicsPrm.ContactGearFriction != CfgPhysicsPrm_default.ContactGearFriction || bEchoAll)
		ofs << "ContactGearFriction = " << CfgPhysicsPrm.ContactGearFriction << '\n';
	if (CfgPhysicsPrm.ContactHullStiffness != CfgPhysicsPrm_default.ContactHullStiffness || bEchoAll)
		ofs << "ContactHullStiffness = " << CfgPhysicsPrm.ContactHullStiffness << '\n';
	if (CfgPhysicsPrm.ContactHullDampingRatio != CfgPhysicsPrm_default.ContactHullDampingRatio || bEchoAll)
		ofs << "ContactHullDampingRatio = " << CfgPhysicsPrm.ContactHullDampingRatio << '\n';
	if (CfgPhysicsPrm.ContactHullFriction != CfgPhysicsPrm_default.ContactHullFriction || bEchoAll)
		ofs << "ContactHullFriction = " << CfgPhysicsPrm.ContactHullFriction << '\n';
}

const void *Config::GetContactParam (DWORD paramtype) const
{
	switch (paramtype) {
	case CFGPRM_CONTACTSOLVER:   return (void*)&CfgPhysicsPrm.ContactSolver;
	}
	return 0;
}
