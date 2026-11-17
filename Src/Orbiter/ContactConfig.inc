// ==============================================================
// ContactConfig.inc
// Contact model configuration: load/write and Config::GetParam support.
//
// Included at the end of the official Config.cpp. Kept in a separate file so
// that the changes to the official Orbiter sources are limited to a single
// include line and three call sites.
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
	if (GetReal (ifs, "ContactStiffnessScale", d))
		CfgPhysicsPrm.ContactStiffnessScale = max (0.01, min (100.0, d));
	if (GetReal (ifs, "ContactDampingRatio", d))
		CfgPhysicsPrm.ContactDampingRatio = max (0.0, min (20.0, d));
	if (GetReal (ifs, "ContactFrictionScale", d))
		CfgPhysicsPrm.ContactFrictionScale = max (0.0, min (100.0, d));
}

void Config::ContactWritePhysics (std::ostream &ofs, bool bEchoAll) const
{
	if (CfgPhysicsPrm.ContactFrictionModel != CfgPhysicsPrm_default.ContactFrictionModel || bEchoAll)
		ofs << "ContactFrictionModel = " << CfgPhysicsPrm.ContactFrictionModel << '\n';
	if (CfgPhysicsPrm.ContactSolver != CfgPhysicsPrm_default.ContactSolver || bEchoAll)
		ofs << "ContactSolver = " << CfgPhysicsPrm.ContactSolver << '\n';
	if (CfgPhysicsPrm.ContactStiffnessScale != CfgPhysicsPrm_default.ContactStiffnessScale || bEchoAll)
		ofs << "ContactStiffnessScale = " << CfgPhysicsPrm.ContactStiffnessScale << '\n';
	if (CfgPhysicsPrm.ContactDampingRatio != CfgPhysicsPrm_default.ContactDampingRatio || bEchoAll)
		ofs << "ContactDampingRatio = " << CfgPhysicsPrm.ContactDampingRatio << '\n';
	if (CfgPhysicsPrm.ContactFrictionScale != CfgPhysicsPrm_default.ContactFrictionScale || bEchoAll)
		ofs << "ContactFrictionScale = " << CfgPhysicsPrm.ContactFrictionScale << '\n';
}

const void *Config::GetContactParam (DWORD paramtype) const
{
	switch (paramtype) {
	case CFGPRM_CONTACTFRICTION: return (void*)&CfgPhysicsPrm.ContactFrictionModel;
	case CFGPRM_CONTACTSOLVER:   return (void*)&CfgPhysicsPrm.ContactSolver;
	}
	return 0;
}
