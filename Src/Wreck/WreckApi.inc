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

// ---- vessel shadow rendering mode API (project extension) ----
DLLEXPORT void oapiSetVesselShadowMode (OBJHANDLE hVessel, int mode)
{
	Body *body = (Body*)hVessel;
	if (body && body->Type() == OBJTP_VESSEL) ((Vessel*)body)->SetShadowMode (mode);
}

DLLEXPORT int oapiGetVesselShadowMode (OBJHANDLE hVessel)
{
	Body *body = (Body*)hVessel;
	if (body && body->Type() == OBJTP_VESSEL) return ((Vessel*)body)->GetShadowMode ();
	return SHADOWMODE_DEFAULT;
}
