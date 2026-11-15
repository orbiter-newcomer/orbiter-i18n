#include "WreckDebris.h"

// ==============================================================
// RelativityApi.inc
// Relativity extension API function implementations.
// Included by OrbiterAPI.cpp (after the extern declarations, so that it can
// use g_pOrbiter and the Orbiter class). Keeps the official API file clean.
// ==============================================================

// Relativistic time dilation of the focus vessel (used by the Lua interface).
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

DLLEXPORT int oapiGetWreckDebrisCount () { 	return WreckDebrisCount (); }  DLLEXPORT bool oapiGetWreckDebris (int idx, VECTOR3 *pos, VECTOR3 *axis, double *angle, double *size, int *shape) { 	if (!pos || !axis || !angle || !size || !shape) return false; 	return WreckDebrisGet (idx, *pos, *axis, *angle, *size, *shape); }

DLLEXPORT bool oapiGetWreckMessageEx (char *buf, int n, double *age_ms, int *isFocus, char *ship, int shipN)
{
	return WreckGetMessageEx (buf, n, age_ms, isFocus, ship, shipN);
}

DLLEXPORT OBJHANDLE oapiGetWreckAnchorVessel () { 	return WreckGetVessel (); }
