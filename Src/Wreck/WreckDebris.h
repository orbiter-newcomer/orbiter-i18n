// ==============================================================
// WreckDebris.h
// Wreck/debris system: declarations.
//
// Wrecks are real Orbiter vessels ("Wreck" class, generated meshes) created by
// the main program when a vessel is destroyed; the graphics client renders them
// through the standard vessel path. (The old 2D screen-space debris system was
// removed.)
// ==============================================================

#ifndef __WRECKDEBRIS_H
#define __WRECKDEBRIS_H

class Vector;
class Vessel;
class StateVectors;

// Collision impact check: if |deltaV| exceeds the configured threshold the
// vessel is destroyed and replaced by debris.
void WreckCollisionImpact (Vessel *vessel, const Vector &deltaV, const char *otherName, bool otherDestroyed = false);
// Surface impact check (called from the force integration when ground contact
// is detected): if the normal approach speed exceeds the threshold, the vessel
// is destroyed and replaced by debris.
void WreckSurfaceImpact (Vessel *vessel, const StateVectors &state);
// Update the wreck camera/vessel state (called once per frame from Orbiter::UpdateWorld).
void WreckDebrisUpdate ();


// Wreck reason codes (message shown to the player when a vessel is destroyed)
enum {
	REL_WRECK_COLLISION = 0,	// hit another vessel
	REL_WRECK_SURFACE,			// hit the surface of a body
	REL_WRECK_STRONGFIELD,		// torn apart in a strong gravity field
	REL_WRECK_FTL,				// hit the light-speed wall
	REL_WRECK_NUMERIC,			// numerical failure ("hand of God")
	REL_WRECK_STAR,				// fell into a star
	REL_WRECK_GASGIANT,			// crushed in a gas giant
	REL_WRECK_BURNUP			// dynamic pressure exceeded: became a meteor
};

// Damage event classes (reworked wreck/damage system).
//
// The destruction policy depends on the failure class:
enum WreckDamageType {
	// FTL / numerical / strong-field failure: the simulation state has failed,
	// so the vessel must leave the update loop IMMEDIATELY. No debris.
	WRECK_DAMAGE_FAILURE  = 1,
	// Flight accident (vessel collision, surface crash, atmospheric burn-up):
	// the vessel is removed IMMEDIATELY and replaced by wreck debris.
	WRECK_DAMAGE_IMPACT   = 2,
};

// Unified damage entry point used by all damage sources.
void WreckDamageVessel (class Vessel *vessel, int damageType, int reason, const char *otherName = 0);

// Sets the on-screen wreck message (bodyName/otherName may be null).
void WreckMessage (int reason, const char *bodyName, const char *otherName, bool otherDestroyed = false);
// Retrieves the current wreck message (UTF-8) and its age in milliseconds.
bool WreckGetMessage (char *buf, int n, double *age_ms);
void WreckMessageVessel (int reason, Vessel *vessel, const char *bodyName, const char *otherName, bool otherDestroyed);
bool WreckGetMessageEx (char *buf, int n, double *age_ms, int *isFocus, char *ship, int shipN);
OBJHANDLE WreckGetVessel ();
// Restore the cockpit view when the player switches to another vessel
void WreckOnFocusChanged (class Vessel *newFocus);
void WreckProcessPendingSafe ();
bool WreckIsVessel (class Vessel *vessel);
bool WreckIsObserver (class Vessel *vessel);
// Atmospheric dynamic-pressure check (vessel burns up as a meteor).
void WreckAtmosphericBurnup (class Vessel *vessel);

// Numerical-failure report from vessel modules (e.g. the thermal system): the
// request is queued (REL_WRECK_NUMERIC) and left to the vessel destruction code
// in the safe phase. Currently no destruction is performed (stub).
void WreckReportNumericFailure (class Vessel *vessel, double maxTemp, int compartment);
// Query/consume the queued numerical-failure request (for the destruction code).
bool WreckGetPendingNumericFailure (char *vesselName, int nameN, double *maxTemp, int *compartment);

#endif // __WRECKDEBRIS_H
