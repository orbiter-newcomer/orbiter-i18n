#include "ContactResources.h"   // project extension: contact dialog IDs
// ==============================================================
// TabExtraContact.inc
// Launchpad "Extra" page: contact model configuration (implementation).
//
// Included at the end of TabExtra.cpp. Exposes the contact force solver and
// the contact parameters of the physics engine:
//   ContactSolver: 0 = official explicit, 1 = implicit damping, 2 = fully implicit
//   contact parameters: landing gear / hull stiffness, damping ratio and
//                       friction (see Config.h)
// ==============================================================

char *ExtraContact::Name ()
{
	return (char*)orbiter::Tr("Contact model");
}

char *ExtraContact::Description ()
{
	return (char*)orbiter::Tr("Configure the surface contact friction model and the contact force solver used by the physics engine.\r\n\r\nFriction model: traditional (original) or experimental (static/speed-proportional friction with force limiter).\r\n\r\nForce solver: official explicit, implicit damping, or fully implicit contact spring/damper.");
}

bool ExtraContact::clbkOpen (HWND hParent)
{
	OpenDialog (hParent, IDD_EXTRA_CONTACT, DlgProc);
	return true;
}

void ExtraContact::InitDialog (HWND hWnd)
{
	static const char *solver[3] = { "Official (explicit)", "Implicit damping", "Fully implicit" };
	int i;
	for (i = 0; i < 3; i++)
		orbiter::ComboAddStringUtf8 (GetDlgItem (hWnd, IDC_CONTACT_SOLVER), orbiter::Tr(solver[i]));
	SetDialog (hWnd, pTab->Cfg()->CfgPhysicsPrm);
}

void ExtraContact::ResetDialog (HWND hWnd)
{
	extern CFG_PHYSICSPRM CfgPhysicsPrm_default;
	SetDialog (hWnd, CfgPhysicsPrm_default);
}

void ExtraContact::SetDialog (HWND hWnd, const CFG_PHYSICSPRM &prm)
{
	int s = prm.ContactSolver;
	if (s < 0) s = 0;
	if (s > 2) s = 2;
	SendDlgItemMessageW (hWnd, IDC_CONTACT_SOLVER, CB_SETCURSEL, s, 0);
	char buf[64];
	sprintf (buf, "%g", prm.ContactGearStiffness);
	SetWindowText (GetDlgItem (hWnd, IDC_CONTACT_GEAR_STIFF), buf);
	sprintf (buf, "%g", prm.ContactGearDampingRatio);
	SetWindowText (GetDlgItem (hWnd, IDC_CONTACT_GEAR_DAMP), buf);
	sprintf (buf, "%g", prm.ContactGearFriction);
	SetWindowText (GetDlgItem (hWnd, IDC_CONTACT_GEAR_MU), buf);
	sprintf (buf, "%g", prm.ContactHullStiffness);
	SetWindowText (GetDlgItem (hWnd, IDC_CONTACT_HULL_STIFF), buf);
	sprintf (buf, "%g", prm.ContactHullDampingRatio);
	SetWindowText (GetDlgItem (hWnd, IDC_CONTACT_HULL_DAMP), buf);
	sprintf (buf, "%g", prm.ContactHullFriction);
	SetWindowText (GetDlgItem (hWnd, IDC_CONTACT_HULL_MU), buf);
}

// read a floating point value from an edit control
static bool ReadContactValue (HWND hWnd, int id, double &val)
{
	char buf[64];
	GetWindowText (GetDlgItem (hWnd, id), buf, 64);
	char *end = 0;
	val = strtod (buf, &end);
	if (end == buf) return false;
	while (*end == ' ' || *end == '\t') end++;
	return (*end == 0);
}

bool ExtraContact::StoreParams (HWND hWnd)
{
	int s = (int)SendDlgItemMessage (hWnd, IDC_CONTACT_SOLVER, CB_GETCURSEL, 0, 0);
	if (s == CB_ERR) {
		Error ("Invalid contact solver selection.");
		return false;
	}
	double kGear, dGear, muGear, kHull, dHull, muHull;
	if (!ReadContactValue (hWnd, IDC_CONTACT_GEAR_STIFF, kGear) ||
		!ReadContactValue (hWnd, IDC_CONTACT_GEAR_DAMP, dGear) ||
		!ReadContactValue (hWnd, IDC_CONTACT_GEAR_MU, muGear) ||
		!ReadContactValue (hWnd, IDC_CONTACT_HULL_STIFF, kHull) ||
		!ReadContactValue (hWnd, IDC_CONTACT_HULL_DAMP, dHull) ||
		!ReadContactValue (hWnd, IDC_CONTACT_HULL_MU, muHull)) {
		Error ("Invalid contact parameter value.");
		return false;
	}
	Config *cfg = pTab->Cfg();
	cfg->CfgPhysicsPrm.ContactSolver = (s < 0 ? 0 : (s > 2 ? 2 : s));
	// clamp to sensible ranges; 0 keeps the original value
	if (kGear > 0.0) { if (kGear < 1e3) kGear = 1e3; else if (kGear > 1e9) kGear = 1e9; }
	if (dGear < 0.0) dGear = 0.0; else if (dGear > 20.0) dGear = 20.0;
	if (muGear < 0.0) muGear = 0.0; else if (muGear > 1e6) muGear = 1e6;
	if (kHull > 0.0) { if (kHull < 1e3) kHull = 1e3; else if (kHull > 1e9) kHull = 1e9; }
	if (dHull < 0.0) dHull = 0.0; else if (dHull > 20.0) dHull = 20.0;
	if (muHull < 0.0) muHull = 0.0; else if (muHull > 1e6) muHull = 1e6;
	cfg->CfgPhysicsPrm.ContactGearStiffness = kGear;
	cfg->CfgPhysicsPrm.ContactGearDampingRatio = dGear;
	cfg->CfgPhysicsPrm.ContactGearFriction = muGear;
	cfg->CfgPhysicsPrm.ContactHullStiffness = kHull;
	cfg->CfgPhysicsPrm.ContactHullDampingRatio = dHull;
	cfg->CfgPhysicsPrm.ContactHullFriction = muHull;
	return true;
}

INT_PTR CALLBACK ExtraContact::DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		// localize the static texts first, then fill in the control values (the
		// numeric edit fields must not be touched by the localization pass)
		orbiter::LocalizeWindowTree(hWnd);
		((ExtraContact*)lParam)->InitDialog (hWnd);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RESET:
			((ExtraContact*)GetWindowLongPtr (hWnd, DWLP_USER))->ResetDialog (hWnd);
			return 0;
		case IDOK:
			if (((ExtraContact*)GetWindowLongPtr (hWnd, DWLP_USER))->StoreParams (hWnd))
				EndDialog (hWnd, 0);
			break;
		}
		break;
	}
	return BuiltinLaunchpadItem::DlgProc (hWnd, uMsg, wParam, lParam);
}
