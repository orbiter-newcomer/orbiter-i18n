// ==============================================================
// ContactAPI.h
// Contact model configuration parameters (project extension).
//
// The contact force solver can be switched at runtime through the Orbiter.cfg
// physics section (see ContactConfig.inc):
//   ContactSolver = 0 (official explicit) / 1 (implicit damping) /
//                   2 (fully implicit spring+damper)
// ==============================================================

#ifndef __CONTACTAPI_H
#define __CONTACTAPI_H

/** Contact force solver. Parameter type: int (0 = explicit, 1 = implicit damping,
 *  2 = fully implicit). */
#define CFGPRM_CONTACTSOLVER 0x0036

#endif // !__CONTACTAPI_H
