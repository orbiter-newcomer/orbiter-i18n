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

	if (GetReal (ifs, "ContactGearStiffnessScale", d))
		CfgPhysicsPrm.ContactGearStiffnessScale = (d < 0.01 ? 0.01 : (d > 100.0 ? 100.0 : d));
	if (GetReal (ifs, "ContactGearDampingRatio", d))
		CfgPhysicsPrm.ContactGearDampingRatio = (d < 0.0 ? 0.0 : (d > 20.0 ? 20.0 : d));
	if (GetReal (ifs, "ContactGearFrictionScale", d))
		CfgPhysicsPrm.ContactGearFrictionScale = (d < 0.0 ? 0.0 : (d > 100.0 ? 100.0 : d));
	if (GetReal (ifs, "ContactHullStiffnessScale", d))
		CfgPhysicsPrm.ContactHullStiffnessScale = (d < 0.01 ? 0.01 : (d > 100.0 ? 100.0 : d));
	if (GetReal (ifs, "ContactHullDampingRatio", d))
		CfgPhysicsPrm.ContactHullDampingRatio = (d < 0.0 ? 0.0 : (d > 20.0 ? 20.0 : d));
	if (GetReal (ifs, "ContactHullFrictionScale", d))
		CfgPhysicsPrm.ContactHullFrictionScale = (d < 0.0 ? 0.0 : (d > 100.0 ? 100.0 : d));

	// backwards compatibility: the earlier single parameter set applied to both
	// groups, so use it as a default for the gear group if the new keys are absent
	if (GetReal (ifs, "ContactStiffnessScale", d) &&
		CfgPhysicsPrm.ContactGearStiffnessScale == CfgPhysicsPrm_default.ContactGearStiffnessScale)
		CfgPhysicsPrm.ContactGearStiffnessScale = (d < 0.01 ? 0.01 : (d > 100.0 ? 100.0 : d));
	if (GetReal (ifs, "ContactDampingRatio", d) &&
		CfgPhysicsPrm.ContactGearDampingRatio == CfgPhysicsPrm_default.ContactGearDampingRatio)
		CfgPhysicsPrm.ContactGearDampingRatio = (d < 0.0 ? 0.0 : (d > 20.0 ? 20.0 : d));
	if (GetReal (ifs, "ContactFrictionScale", d) &&
		CfgPhysicsPrm.ContactGearFrictionScale == CfgPhysicsPrm_default.ContactGearFrictionScale)
		CfgPhysicsPrm.ContactGearFrictionScale = (d < 0.0 ? 0.0 : (d > 100.0 ? 100.0 : d));
}

void Config::ContactWritePhysics (std::ostream &ofs, bool bEchoAll) const
{
	if (CfgPhysicsPrm.ContactFrictionModel != CfgPhysicsPrm_default.ContactFrictionModel || bEchoAll)
		ofs << "ContactFrictionModel = " << CfgPhysicsPrm.ContactFrictionModel << '\n';
	if (CfgPhysicsPrm.ContactSolver != CfgPhysicsPrm_default.ContactSolver || bEchoAll)
		ofs << "ContactSolver = " << CfgPhysicsPrm.ContactSolver << '\n';
	if (CfgPhysicsPrm.ContactGearStiffnessScale != CfgPhysicsPrm_default.ContactGearStiffnessScale || bEchoAll)
		ofs << "ContactGearStiffnessScale = " << CfgPhysicsPrm.ContactGearStiffnessScale << '\n';
	if (CfgPhysicsPrm.ContactGearDampingRatio != CfgPhysicsPrm_default.ContactGearDampingRatio || bEchoAll)
		ofs << "ContactGearDampingRatio = " << CfgPhysicsPrm.ContactGearDampingRatio << '\n';
	if (CfgPhysicsPrm.ContactGearFrictionScale != CfgPhysicsPrm_default.ContactGearFrictionScale || bEchoAll)
		ofs << "ContactGearFrictionScale = " << CfgPhysicsPrm.ContactGearFrictionScale << '\n';
	if (CfgPhysicsPrm.ContactHullStiffnessScale != CfgPhysicsPrm_default.ContactHullStiffnessScale || bEchoAll)
		ofs << "ContactHullStiffnessScale = " << CfgPhysicsPrm.ContactHullStiffnessScale << '\n';
	if (CfgPhysicsPrm.ContactHullDampingRatio != CfgPhysicsPrm_default.ContactHullDampingRatio || bEchoAll)
		ofs << "ContactHullDampingRatio = " << CfgPhysicsPrm.ContactHullDampingRatio << '\n';
	if (CfgPhysicsPrm.ContactHullFrictionScale != CfgPhysicsPrm_default.ContactHullFrictionScale || bEchoAll)
		ofs << "ContactHullFrictionScale = " << CfgPhysicsPrm.ContactHullFrictionScale << '\n';
}

const void *Config::GetContactParam (DWORD paramtype) const
{
	switch (paramtype) {
	case CFGPRM_CONTACTFRICTION: return (void*)&CfgPhysicsPrm.ContactFrictionModel;
	case CFGPRM_CONTACTSOLVER:   return (void*)&CfgPhysicsPrm.ContactSolver;
	}
	return 0;
}
