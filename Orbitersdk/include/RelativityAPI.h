// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ======================================================================
//                     ORBITER SOFTWARE DEVELOPMENT KIT
// RelativityAPI.h
// Relativity extension API: graphics-client configuration parameters and
// oapi functions. Kept in a separate header so the official Orbiter headers
// only need a single include line. This header must be included after the
// OAPIFUNC / OBJHANDLE / VECTOR3 definitions (both official headers below
// satisfy this).
// ======================================================================

#ifndef __RELATIVITYAPI_H
#define __RELATIVITYAPI_H

// --- Graphics client configuration parameters -------------------------

/**
 * Relativistic Doppler shift visual effect (graphics client extension)
 * \par Parameter type:
 *   bool
 */
#define CFGPRM_RELDOPPLER 0x0027

/**
 * Relativistic aberration visual effect (graphics client extension)
 * \par Parameter type:
 *   bool
 */
#define CFGPRM_RELABERRATION 0x0028

/**
 * Relativistic Doppler observer exposure adaptation white point.
 * 0 = exposure adaptation disabled (graphics client extension)
 * \par Parameter type:
 *   double
 */
#define CFGPRM_RELEXPOSURE 0x0029

/**
 * Relativistic gravitational lensing visual effect (graphics client extension)
 * \par Parameter type:
 *   bool
 */
#define CFGPRM_RELLENSING 0x002A

/**
 * Main physics engine relativity switch. When disabled, all relativistic
 * visual effects are disabled as well (graphics client extension).
 * \par Parameter type:
 *   bool
 */
#define CFGPRM_PHYSRELATIVITY 0x002B

/**
 * Relativity gravity solver: 0 = 1PN weak field, 1 = Kerr exact
 * (graphics client extension)
 * \par Parameter type:
 *   int
 */
#define CFGPRM_RELGRAVMETHOD 0x002C
/**
 * \brief Debug option: render the gravitational lensing source field (star
 *   field cubemap) as a tile wireframe.
 * \note Graphics client extension. Mainly used to inspect the lensing mapping.
 * \par Parameter type:
 *   bool
 */
#define CFGPRM_RELLENSWIREFRAME 0x002D
/** Gravitational redshift of the observed sky (bool). */
#define CFGPRM_RELGRAVREDSHIFT 0x002E
/** Debug: draw markers at the relativity-shifted apparent positions of the bodies (bool). */
#define CFGPRM_RELBODYMARKERS 0x002F

/** Charge-induced gravity (Reissner-Nordstrom) for compact relativistic bodies
 *  (black holes / neutron stars). bool. */
#define CFGPRM_RELCHARGE 0x0030

/** Charge ratio q = r_Q/r_s for the compact bodies (double, 0..1). */
#define CFGPRM_RELCHARGERATIO 0x0031

// --- API functions -----------------------------------------------------

/**
 * \brief Returns the relativistic spin parameters of a celestial body.
 * \param hObj object handle (must be a star or planet)
 * \param aM pointer receiving the dimensionless Kerr spin parameter a/M (may be NULL)
 * \param axis pointer receiving the normalised spin axis direction (may be NULL)
 * \return true if the object is a celestial body with a valid mass, false otherwise.
 * \note The spin is derived from the body rotation and inertia the same way the
 *   physics engine does it: J = I * omega * axis, a/M = J c / (G M^2).
 *   Used by the graphics clients for the Kerr spin correction of the
 *   relativistic visual effects.
 */
OAPIFUNC bool oapiGetRelativitySpin (OBJHANDLE hObj, double *aM, VECTOR3 *axis);

/**
 * \brief Relativistic time dilation at the focus vessel: proper time rate
 *   d(tau)/dt = 1 / (velGamma * gravFactor) (1.0 = no time dilation).
 */
OAPIFUNC double oapiGetRelativityTimeDilation ();

/**
 * \brief Gravitational time dilation factor 1/sqrt(1-2*Phi/c^2) at the
 *   focus vessel (1.0 when relativistic physics is disabled).
 */
OAPIFUNC double oapiGetRelativityGravTimeFactor ();

/**
 * \brief Velocity Lorentz factor gamma of the focus vessel
 *   (1.0 when relativistic physics is disabled).
 */
OAPIFUNC double oapiGetRelativityVelTimeFactor ();

#endif // !__RELATIVITYAPI_H
