// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ============================================================================
// Relativity extension: UI implementation.
//
// Implementation moved out of the official options/launchpad source files.
// Declarations: RelativityUi.h.
// ============================================================================

#include <windows.h>
#include <commctrl.h>
#include "Config.h"
#include "OptionsPages.h"
#include "resource.h"
#include "Localization.h"
#include "RelativityUi.h"

// Update the "K=x.x / off" readout of the relativistic Doppler exposure slider.
// The rightmost tick (8) means "exposure adaptation disabled".
void UpdateRelExposureLabel (HWND hPage)
{
	int pos = (int)SendDlgItemMessage (hPage, IDC_OPT_VIS_RELEXPOSURE, TBM_GETPOS, 0, 0);
	char buf[32];
	if (pos >= 8) strcpy_s (buf, 32, orbiter::IsChinese() ? "关" : "off");
	else sprintf_s (buf, 32, "K=%.1f", 1.0 + (pos - 1) * 0.5);
	orbiter::SetWindowTextUtf8 (GetDlgItem (hPage, IDC_OPT_VIS_RELEXP_DSP), buf);   // UTF-8 -> wide (中文"关"防乱码)
}

BOOL OptionsPage_Visual::OnHScroll (HWND hPage, WPARAM wParam, LPARAM lParam)
{
	if (HWND(lParam) == GetDlgItem (hPage, IDC_OPT_VIS_RELEXPOSURE)) {
		UpdateRelExposureLabel (hPage);
	}
	return FALSE;
}

void RelativityVisualUpdateControls (HWND hPage, Config *cfg)
{
	// relativistic visual effects
	SendDlgItemMessage(hPage, IDC_OPT_VIS_RELDOPPLER, BM_SETCHECK,
		cfg->CfgVisualPrm.bRelDoppler ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hPage, IDC_OPT_VIS_RELABERRATION, BM_SETCHECK,
		cfg->CfgVisualPrm.bRelAberration ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hPage, IDC_OPT_VIS_RELLENSING, BM_SETCHECK,
		cfg->CfgVisualPrm.bRelLensing ? BST_CHECKED : BST_UNCHECKED, 0);	SendDlgItemMessage(hPage, IDC_OPT_VIS_RELGRAVREDSHIFT, BM_SETCHECK,
		cfg->CfgVisualPrm.bRelGravRedshift ? BST_CHECKED : BST_UNCHECKED, 0);
	{
		double k = cfg->CfgVisualPrm.RelExposureWhite;
		int pos = (k <= 0.0) ? 8 : (int)((k - 1.0) / 0.5 + 0.5) + 1;
		if (pos < 1) pos = 1;
		if (pos > 8) pos = 8;
		SendDlgItemMessage(hPage, IDC_OPT_VIS_RELEXPOSURE, TBM_SETPOS, 1, pos);
	}
	UpdateRelExposureLabel(hPage);
}

void RelativityVisualUpdateConfig (HWND hPage, Config *cfg)
{
	// relativistic visual effects
	cfg->CfgVisualPrm.bRelDoppler = (SendDlgItemMessage(hPage, IDC_OPT_VIS_RELDOPPLER, BM_GETCHECK, 0, 0) == BST_CHECKED);
	cfg->CfgVisualPrm.bRelAberration = (SendDlgItemMessage(hPage, IDC_OPT_VIS_RELABERRATION, BM_GETCHECK, 0, 0) == BST_CHECKED);
	cfg->CfgVisualPrm.bRelLensing = (SendDlgItemMessage(hPage, IDC_OPT_VIS_RELLENSING, BM_GETCHECK, 0, 0) == BST_CHECKED);	cfg->CfgVisualPrm.bRelGravRedshift = (SendDlgItemMessage(hPage, IDC_OPT_VIS_RELGRAVREDSHIFT, BM_GETCHECK, 0, 0) == BST_CHECKED);
	{
		int pos = (int)SendDlgItemMessage(hPage, IDC_OPT_VIS_RELEXPOSURE, TBM_GETPOS, 0, 0);
		cfg->CfgVisualPrm.RelExposureWhite = (pos >= 8) ? 0.0 : (1.0 + (pos - 1) * 0.5);
	}
}

void RelativityVisualInitSlider (HWND hPage)
{
	// relativistic Doppler exposure adaptation slider range: tick 1..7 => K=1.0..4.0,
	// rightmost tick (8) => exposure adaptation disabled
	SendDlgItemMessage(hPage, IDC_OPT_VIS_RELEXPOSURE, TBM_SETRANGEMAX, 1, 8);
	SendDlgItemMessage(hPage, IDC_OPT_VIS_RELEXPOSURE, TBM_SETRANGEMIN, 1, 1);
	SendDlgItemMessage(hPage, IDC_OPT_VIS_RELEXPOSURE, TBM_SETTICFREQ, 1, 0);
}

void RelativityPhysicsUpdateControls (HWND hPage, Config *cfg)
{
	SendDlgItemMessage(hPage, IDC_OPT_PHYS_RELATIVITY, BM_SETCHECK,
		cfg->CfgPhysicsPrm.bRelativity ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage (hPage, IDC_OPT_PHYS_RELFTL, CB_SETCURSEL, cfg->CfgPhysicsPrm.RelativityFTLMode, 0);
}

void RelativityPhysicsUpdateConfig (HWND hPage, Config *cfg)
{
	cfg->CfgPhysicsPrm.bRelativity = (SendDlgItemMessage(hPage, IDC_OPT_PHYS_RELATIVITY, BM_GETCHECK, 0, 0) == BST_CHECKED);
	int iFTL = (int)SendDlgItemMessage (hPage, IDC_OPT_PHYS_RELFTL, CB_GETCURSEL, 0, 0);
	if (iFTL >= 0 && iFTL <= 1) cfg->CfgPhysicsPrm.RelativityFTLMode = iFTL;
}

void RelativityPhysicsToggle (HWND hPage, Config *cfg)
{
	bool check = (SendDlgItemMessage(hPage, IDC_OPT_PHYS_RELATIVITY, BM_GETCHECK, 0, 0) == BST_CHECKED);
	cfg->CfgPhysicsPrm.bRelativity = check;
}

// ---- Launchpad Extra page: relativistic gravity field solver ---------------

// Physics engine: Relativistic gravity field solver - implementation.
//
// Implementation moved out of the official TabExtra.cpp (included at the
// original location). See TabExtraRelativity.h for the class declaration.
// -----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Physics engine: Relativistic gravity field solver
//-----------------------------------------------------------------------------

char *ExtraRelativity::Name ()
{
	return (char*)orbiter::Tr ("Relativity effects");
}

char *ExtraRelativity::Description ()
{
	return (char*)orbiter::Tr ("Advanced configuration of relativistic effects, such as the gravity model.");
}

bool ExtraRelativity::clbkOpen (HWND hParent)
{
	OpenDialog (hParent, IDD_EXTRA_RELATIVITY, DlgProc);
	return true;
}

void ExtraRelativity::InitDialog (HWND hWnd)
{
	SetDialog (hWnd);
}

void ExtraRelativity::SetDialog (HWND hWnd)
{
	int method = pTab->Cfg()->CfgPhysicsPrm.RelativityGravityMethod;
	int id = (method == 2) ? IDC_REL_METHOD_NEWTON : (method == 1 ? IDC_REL_METHOD_KERR : IDC_REL_METHOD_1PN);
	SendDlgItemMessage (hWnd, id, BM_SETCHECK, BST_CHECKED, 0);
	SendDlgItemMessage (hWnd, IDC_REL_CHARGE, BM_SETCHECK,
		pTab->Cfg()->CfgPhysicsPrm.bRelativisticCharge ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool ExtraRelativity::StoreParams (HWND hWnd)
{
	int method = 0;
	if (SendDlgItemMessage (hWnd, IDC_REL_METHOD_NEWTON, BM_GETCHECK, 0, 0) == BST_CHECKED) method = 2;
	else if (SendDlgItemMessage (hWnd, IDC_REL_METHOD_KERR, BM_GETCHECK, 0, 0) == BST_CHECKED) method = 1;
	pTab->Cfg()->CfgPhysicsPrm.RelativityGravityMethod = method;
	pTab->Cfg()->CfgPhysicsPrm.bRelativisticCharge =
		(SendDlgItemMessage (hWnd, IDC_REL_CHARGE, BM_GETCHECK, 0, 0) == BST_CHECKED);
	return true;
}

INT_PTR CALLBACK ExtraRelativity::DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		((ExtraRelativity*)lParam)->InitDialog (hWnd);
		orbiter::LocalizeWindowTree (hWnd);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (((ExtraRelativity*)GetWindowLongPtr (hWnd, DWLP_USER))->StoreParams (hWnd))
				EndDialog (hWnd, 0);
			break;
		case IDCANCEL:
			EndDialog (hWnd, 0);
			break;
		}
		break;
	}
	return BuiltinLaunchpadItem::DlgProc (hWnd, uMsg, wParam, lParam);
}
