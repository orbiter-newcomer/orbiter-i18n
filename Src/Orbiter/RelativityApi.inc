// ==============================================================
// RelativityApi.inc
// Relativity extension API function implementations.
// Included by OrbiterAPI.cpp (after the extern declarations, so that it can
// use g_pOrbiter and the Orbiter class). Keeps the official API file clean.
// ==============================================================

DLLEXPORT double oapiGetRelativityTimeDilation ()
{
	if (!g_pOrbiter) return 1.0;
	double vel  = g_pOrbiter->RelWarpScale();
	double grav = g_pOrbiter->RelGravTimeFactor();
	return 1.0 / (vel * grav);
}

DLLEXPORT double oapiGetRelativityGravTimeFactor ()
{
	if (!g_pOrbiter) return 1.0;
	return g_pOrbiter->RelGravTimeFactor();
}

DLLEXPORT double oapiGetRelativityVelTimeFactor ()
{
	if (!g_pOrbiter) return 1.0;
	return g_pOrbiter->RelWarpScale();
}