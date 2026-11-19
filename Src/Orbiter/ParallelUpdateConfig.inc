// ============================================================================
// ParallelUpdateConfig.inc (Orbiter core, performance extension)
//
// Configuration file I/O for the multi-core vessel update feature (implementation
// in ParallelUpdate.inc). Kept in a separate file so that the official Orbiter
// sources only need two call lines and one include line.
//
// Keys in the [Physics] section of Orbiter.cfg:
//   VesselParallelUpdate = 0/1   enable the multi-core vessel state update
//   VesselParallelRange  = <m>   vessels closer than this to any other vessel
//                                are updated in sequence (possible interaction)
// ============================================================================

void Config::ParallelUpdateLoad (std::istream &ifs)
{
	bool b;
	double d;
	if (GetBool (ifs, "VesselParallelUpdate", b))
		CfgPhysicsPrm.bParallelVesselUpdate = b;
	if (GetReal (ifs, "VesselParallelRange", d))
		CfgPhysicsPrm.ParallelVesselRange = (d < 1e3 ? 1e3 : (d > 1e7 ? 1e7 : d));
}

void Config::ParallelUpdateWritePhysics (std::ostream &ofs, bool bEchoAll) const
{
	if (CfgPhysicsPrm.bParallelVesselUpdate != CfgPhysicsPrm_default.bParallelVesselUpdate || bEchoAll)
		ofs << "VesselParallelUpdate = " << BoolStr (CfgPhysicsPrm.bParallelVesselUpdate) << '\n';
	if (CfgPhysicsPrm.ParallelVesselRange != CfgPhysicsPrm_default.ParallelVesselRange || bEchoAll)
		ofs << "VesselParallelRange = " << CfgPhysicsPrm.ParallelVesselRange << '\n';
}
