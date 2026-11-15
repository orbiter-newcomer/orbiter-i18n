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

/** Number of active wreck debris particles (rendered by the graphics client). */
OAPIFUNC int oapiGetWreckDebrisCount ();

/**
 * \brief Retrieve a wreck debris particle for rendering.
 * \param idx debris index (0 .. oapiGetWreckDebrisCount()-1)
 * \param pos global position, axis/angle orientation, size radius, shape index
 * \return true if the entry is active.
 */
OAPIFUNC bool oapiGetWreckDebris (int idx, VECTOR3 *pos, VECTOR3 *axis, double *angle, double *size, int *shape);

/** Retrieve the current wreck message (UTF-8) and its age in milliseconds. */
OAPIFUNC bool oapiGetWreckMessageEx (char *buf, int n, double *age_ms, int *isFocus, char *ship, int shipN);

/** Handle of the invisible wreck anchor vessel (0 if none). */
OAPIFUNC OBJHANDLE oapiGetWreckAnchorVessel ();

#endif // !__WRECKAPI_H
