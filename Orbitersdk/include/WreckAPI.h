// ==============================================================
// WreckAPI.h
// Wreck/debris system: public API and configuration parameter types.
//
// Kept separate from RelativityAPI.h so that the wreck system is fully
// independent of the relativity extension.
// ==============================================================

#ifndef __WRECKAPI_H
#define __WRECKAPI_H

// --- Configuration parameter types (used by oapiGetConfigParam / Config) ---

/**
 * Collision impact delta-v tolerance [m/s]: impacts above this value
 * destroy the vessel and replace it with debris (wreck system).
 * \par Parameter type:
 *   double
 */
#define CFGPRM_RELCOLLISIONDV 0x0032

/** Gas-giant crushing radius factor (wreck system, double). */
#define CFGPRM_RELGASGIANT 0x0033

/**
 * Wreck/debris system enable flag.
 * \par Parameter type:
 *   bool
 */
#define CFGPRM_WRECKS 0x0034

// --- API functions -----------------------------------------------------


/** Retrieve the current wreck message (UTF-8) and its age in milliseconds. */
OAPIFUNC bool oapiGetWreckMessageEx (char *buf, int n, double *age_ms, int *isFocus, char *ship, int shipN);

/** Handle of the invisible wreck anchor vessel (0 if none). */
OAPIFUNC OBJHANDLE oapiGetWreckAnchorVessel ();

/**
 * \brief Report a numerical failure (non-finite or out-of-range state) of a
 *   vessel to the wreck/destruction system.
 * \param hVessel vessel handle
 * \param maxTemp maximum temperature of the affected compartment [K] (0 if the
 *   failure is not a temperature failure)
 * \param compartment affected compartment index (-1 if unknown)
 * \note The request is only queued by the wreck system (REL_WRECK_NUMERIC).
 *   The actual destruction policy is decided by the vessel destruction code in
 *   the safe phase (currently stubbed); this API exists so that vessel modules
 *   (e.g. the thermal system) can hand over numerical failures without creating
 *   or deleting vessels during the physics update.
 */
OAPIFUNC void oapiWreckReportNumericFailure (OBJHANDLE hVessel, double maxTemp, int compartment);

#endif // !__WRECKAPI_H
