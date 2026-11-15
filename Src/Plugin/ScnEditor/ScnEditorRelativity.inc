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

// Relativity: reject superluminal velocities entered in the state-vector tab.
// Returns false if the input must be rejected (velocity magnitude > c); the
// user is informed with a message box and the input is not applied.
static bool ScnEditorCheckSuperluminal (VECTOR3 vel)
{
	const double C_LIGHT = 299792458.0;
	double vl = sqrt (vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);
	if (vl <= C_LIGHT) return true;
	const bool zh = (oapiZhActive () != 0);
	const char *txt = zh
		? "相对论模式：输入的速度超过光速，输入已被截留。"
		: "Relativistic mode: the entered velocity exceeds the speed of light. The input has been rejected.";
	const wchar_t *title = zh ? L"相对论模式" : L"Relativistic mode";
	MessageBoxW (NULL, oapiUtf8ToWString(txt).c_str(), title, MB_OK | MB_ICONWARNING);
	return false;
}
