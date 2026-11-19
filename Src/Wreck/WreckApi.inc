// ==============================================================
// WreckApi.inc
// Wreck/debris system API function implementations.
// Included by OrbiterAPI.cpp (after the extern declarations).
// Kept separate from RelativityApi.inc: the wreck system is independent of
// the relativity extension.
// ==============================================================

#include "WreckDebris.h"

// (the old 2D visual debris API oapiGetWreckDebrisCount/oapiGetWreckDebris was
// removed together with the screen-space debris drawing)

DLLEXPORT bool oapiGetWreckMessageEx (char *buf, int n, double *age_ms, int *isFocus, char *ship, int shipN)
{
	return WreckGetMessageEx (buf, n, age_ms, isFocus, ship, shipN);
}

DLLEXPORT OBJHANDLE oapiGetWreckAnchorVessel () { return WreckGetVessel (); }

DLLEXPORT void oapiWreckReportNumericFailure (OBJHANDLE hVessel, double maxTemp, int compartment)
{
	WreckReportNumericFailure ((Vessel*)hVessel, maxTemp, compartment);
}
