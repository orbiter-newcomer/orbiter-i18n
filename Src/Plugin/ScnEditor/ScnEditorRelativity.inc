// ==============================================================
// ScnEditorRelativity.inc
// Relativity-related helpers for the scenario editor (included by Editor.cpp).
// ==============================================================

// Returns the radius (m) at which a newly created vessel is placed around the
// reference body. Ordinary bodies use the traditional 1.1 x body radius
// altitude. Strong-field bodies (black holes, whose Schwarzschild radius is
// comparable to or larger than the visual size) get a safe circular orbit at
// 20 rs instead, otherwise the new vessel would appear inside the strong-field
// protection zone and be removed immediately.
static double ScnEditorSafeOrbitRadius (OBJHANDLE hBody)
{
	if (!hBody) hBody = oapiGetGbodyByIndex (0);
	double mass = oapiGetMass (hBody);
	double rad = 1.1 * oapiGetSize (hBody);
	const double C_LIGHT = 299792458.0;
	double rs = 2.0 * GGRAV * mass / (C_LIGHT * C_LIGHT);
	if (rs >= 0.5 * rad) rad = 20.0 * rs;
	return rad;
}