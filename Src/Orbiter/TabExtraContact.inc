// ==============================================================
// TabExtraContact.inc
// Launchpad "Extra" page: contact model configuration (implementation).
//
// Included at the end of TabExtra.cpp. Exposes two settings of the physics
// engine contact model:
//   ContactFrictionModel: 0 = traditional, 1 = experimental
//   ContactSolver:        0 = official explicit, 1 = implicit damping,
//                         2 = fully implicit
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
	static const char *friction[2] = { "Traditional", "Experimental" };
	static const char *solver[3] = { "Official (explicit)", "Implicit damping", "Fully implicit" };
	int i;
	for (i = 0; i < 2; i++)
		orbiter::ComboAddStringUtf8 (GetDlgItem (hWnd, IDC_CONTACT_FRICTION), orbiter::Tr(friction[i]));
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
	int f = prm.ContactFrictionModel;
	if (f < 0) f = 0;
	if (f > 1) f = 1;
	int s = prm.ContactSolver;
	if (s < 0) s = 0;
	if (s > 2) s = 2;
	SendDlgItemMessageW (hWnd, IDC_CONTACT_FRICTION, CB_SETCURSEL, f, 0);
	SendDlgItemMessageW (hWnd, IDC_CONTACT_SOLVER, CB_SETCURSEL, s, 0);
}

bool ExtraContact::StoreParams (HWND hWnd)
{
	int f = (int)SendDlgItemMessage (hWnd, IDC_CONTACT_FRICTION, CB_GETCURSEL, 0, 0);
	int s = (int)SendDlgItemMessage (hWnd, IDC_CONTACT_SOLVER, CB_GETCURSEL, 0, 0);
	if (f == CB_ERR || s == CB_ERR) {
		Error ("Invalid contact model selection.");
		return false;
	}
	Config *cfg = pTab->Cfg();
	cfg->CfgPhysicsPrm.ContactFrictionModel = (f != 0 ? 1 : 0);
	cfg->CfgPhysicsPrm.ContactSolver = (s < 0 ? 0 : (s > 2 ? 2 : s));
	return true;
}

INT_PTR CALLBACK ExtraContact::DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		((ExtraContact*)lParam)->InitDialog (hWnd);
		orbiter::LocalizeWindowTree(hWnd);
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
