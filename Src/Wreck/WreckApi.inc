// ==============================================================
// WreckApi.inc
// Wreck/debris system API function implementations.
// Included by OrbiterAPI.cpp (after the extern declarations).
// Kept separate from RelativityApi.inc: the wreck system is independent of
// the relativity extension.
// ==============================================================

#include "WreckDebris.h"

DLLEXPORT int oapiGetWreckDebrisCount () { return WreckDebrisCount (); }

DLLEXPORT bool oapiGetWreckDebris (int idx, VECTOR3 *pos, VECTOR3 *axis, double *angle, double *size, int *shape)
{
	if (!pos || !axis || !angle || !size || !shape) return false;
	return WreckDebrisGet (idx, *pos, *axis, *angle, *size, *shape);
}

DLLEXPORT bool oapiGetWreckMessageEx (char *buf, int n, double *age_ms, int *isFocus, char *ship, int shipN)
{
	return WreckGetMessageEx (buf, n, age_ms, isFocus, ship, shipN);
}

DLLEXPORT OBJHANDLE oapiGetWreckAnchorVessel () { return WreckGetVessel (); }
