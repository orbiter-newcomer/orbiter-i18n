// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ==============================================================
//                 ORBITER MODULE: Meshdebug
//                  Part of the ORBITER SDK
//
// Meshdebug.cpp
//
// A simple tool for marking mesh groups during a simulation.
// May be useful for debugging meshes.
// ==============================================================

#define STRICT
#define ORBITER_MODULE
#include <windows.h>
#include <commctrl.h>
#include "orbitersdk.h"
#include "resource.h"
#include <cstdio>
#include <string>

using std::min;
using std::max;

// ==============================================================
// Global variables
// ==============================================================

HINSTANCE g_hInst;    // module instance handle
HWND g_hDlg = 0;      // dialog window handle
DWORD g_dwCmd;        // custom function identifier
VESSEL *g_vessel;     // focus vessel interface
VISHANDLE *g_visptr;  // pointer to visual of current focus object
MESHHANDLE g_mesh;    // current mesh handle
MATERIAL matOrig;
DWORD g_nmesh, g_ngrp;
int   g_imesh, g_igrp;
DWORD g_flag;
DWORD g_origmat;
int g_tmpmat;
float g_opac = 0.5f;
double g_t;
bool blink;

// ==============================================================
// Local prototypes
// ==============================================================

void ChangeMesh (int idx);
void ChangeGroup (int idx);
void OpenDlgClbk (void *context);
void SetMaterialOpacity (MESHHANDLE hMesh, float opac);
// UTF-8 -> UTF-16 helper
static std::wstring Utf8ToWideS(const char *s)
{
	if (!s) return L"";
	int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
	if (n <= 0) return L"";
	std::wstring w(n - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], n);
	return w;
}

// Chinese UI localization for the Mesh debugger dialog (rc stays English)
static const struct { const wchar_t *en; const wchar_t *zh; } meshMap[] = {
    { L"Mesh Debugger", L"网格调试器" },
    { L"Visual:", L"视觉体:" },
    { L"Mesh:", L"网格:" },
    { L"Group:", L"组:" }, { L"[None]", L"[无]" }
};
static BOOL CALLBACK ZhMeshLocCb(HWND hWnd, LPARAM)
{
    wchar_t txt[256];
    if (GetWindowTextW(hWnd, txt, 256) > 0) {
        for (size_t i = 0; i < sizeof(meshMap)/sizeof(meshMap[0]); i++) {
            if (!wcscmp(txt, meshMap[i].en)) { SetWindowTextW(hWnd, meshMap[i].zh); break; }
        }
    }
    return TRUE;
}
static void ZhLocalizeMeshDlg(HWND hDlg)
{
    if (strcmp(oapiTr("Build"), "Build") == 0) return;
    wchar_t title[128];
    if (GetWindowTextW(hDlg, title, 128) > 0 && !wcscmp(title, L"Mesh Debugger"))
        SetWindowTextW(hDlg, L"网格调试器");
    EnumChildWindows(hDlg, ZhMeshLocCb, 0);
}

INT_PTR CALLBACK MsgProc (HWND, UINT, WPARAM, LPARAM);

// ==============================================================
// API interface
// ==============================================================

// ==============================================================
// This function is called when Orbiter starts or when the module
// is activated.

DLLCLBK void InitModule (HINSTANCE hModule)
{
	g_hInst = hModule;
	g_dwCmd = oapiRegisterCustomCmd ((char*)"Mesh debugger",
		(char*)"Mark individual mesh groups in a vessel mesh",
		OpenDlgClbk, NULL);
}


// ==============================================================
// This function is called when Orbiter shuts down or when the
// module is deactivated

DLLCLBK void ExitModule (HINSTANCE hModule)
{
	oapiUnregisterCustomCmd (g_dwCmd);
	// Unregister the custom function in Orbiter
}


// ==============================================================
// frame step

DLLCLBK void opcPreStep (double simt, double simdt, double mjd)
{
	if (!g_hDlg) return;
	double d, t = oapiGetSysTime();
	bool bshow = (modf (t-g_t, &d) > 0.5);
	if (bshow != blink) {
		blink = bshow;
		if (g_igrp >= 0) {
			MESHGROUP *grp = oapiMeshGroup (g_mesh, g_igrp);
			grp->UsrFlag = (blink ? g_flag:3);
		}
	}
}

// ==============================================================
// Open the dialog window

void OpenDlgClbk (void *context)
{
	g_hDlg = oapiOpenDialog (g_hInst, IDD_DBGDIALOG, MsgProc);
}


// ==============================================================
// Close the dialog

void CloseDlg (HWND hDlg)
{
	ChangeMesh (-1);
	oapiCloseDialog (hDlg);
	g_hDlg = 0;
}


// ==============================================================

void GetMeshParams ()
{
	g_vessel = oapiGetFocusInterface();
	g_visptr = oapiObjectVisualPtr (oapiGetFocusObject());
	g_mesh = 0;
	g_nmesh = g_ngrp =  0;
	g_imesh = g_igrp = -1;
	g_tmpmat = -1;
	if (*g_visptr) {
		for (; g_vessel->GetMesh (*g_visptr, g_nmesh); g_nmesh++);
		ChangeMesh (0);
	}
}

void ChangeMesh (int idx)
{
	if (*g_visptr && idx != g_imesh) {
		if (g_mesh) {
			ChangeGroup (-1);
			SetMaterialOpacity (g_mesh, 1.0f/g_opac);
			oapiDeleteMaterial (g_mesh, g_tmpmat);
			oapiSetMeshProperty (g_mesh, MESHPROPERTY_MODULATEMATALPHA, 0);
		}
		g_imesh = idx;
		g_mesh = (idx >= 0 ? g_vessel->GetMesh (*g_visptr, g_imesh) : 0);
		if (g_mesh) {
			g_ngrp = oapiMeshGroupCount (g_mesh);
			SetMaterialOpacity (g_mesh, g_opac);
			oapiSetMeshProperty (g_mesh, MESHPROPERTY_MODULATEMATALPHA, 1);
			MATERIAL mat;
			g_tmpmat = oapiAddMaterial (g_mesh, &mat);
			ChangeGroup (0);
		}
	}
}

void ChangeGroup (int idx)
{
	if (*g_visptr) {
		MESHGROUP *grp;
		g_t = oapiGetSysTime();
		blink = true;
		if (g_igrp >= 0) {
			grp = oapiMeshGroup (g_mesh, g_igrp);
			grp->UsrFlag = g_flag;
			grp->MtrlIdx = g_origmat;
		}
		g_igrp = idx;
		if (g_igrp >= 0) {
			grp = oapiMeshGroup (g_mesh, g_igrp);
			g_flag = grp->UsrFlag; // save original flag
			grp->UsrFlag = 3;      // hide group
			g_origmat = grp->MtrlIdx;
			if (g_origmat >= oapiMeshMaterialCount (g_mesh)) g_origmat = 0;
			MATERIAL *mat1 = oapiMeshMaterial (g_mesh, g_tmpmat);
			MATERIAL *mat2 = oapiMeshMaterial (g_mesh, g_origmat);
			memcpy (mat1, mat2, sizeof(MATERIAL));
			mat1->diffuse.a /= g_opac;
			mat1->ambient.a /= g_opac;
			mat1->specular.a /= g_opac;
			mat1->emissive.a /= g_opac;
			grp->MtrlIdx = g_tmpmat;
		}
	}
}

void SetMaterialOpacity (MESHHANDLE hMesh, float opac)
{
	DWORD i, nmat = oapiMeshMaterialCount (hMesh);
	for (i = 0; i < nmat; i++) {
		MATERIAL *mat = oapiMeshMaterial (hMesh, i);
		mat->diffuse.a  *= opac;
		mat->ambient.a  *= opac;
		mat->specular.a *= opac;
		mat->emissive.a *= opac;
	}
}

// ==============================================================

void RefreshDialog (HWND hDlg)
{
	VISHANDLE vis = *g_visptr;
	char cbuf[256];
	if (vis) {
		SendMessage (GetDlgItem (hDlg, IDC_MESHSPIN), UDM_SETRANGE, 0, MAKELONG (g_nmesh-1, 0));
		//SendMessage (GetDlgItem (hDlg, IDC_MESHSPIN), UDM_SETPOS, 0, MAKELONG (g_imesh, 0));
		SendMessage (GetDlgItem (hDlg, IDC_GROUPSPIN), UDM_SETRANGE, 0, MAKELONG (g_ngrp-1, 0));
		SendMessage (GetDlgItem (hDlg, IDC_GROUPSPIN), UDM_SETPOS, 0, MAKELONG (g_igrp, 0));
	}
	sprintf (cbuf, oapiTr("(0 to %d)"), g_nmesh-1);
	SetWindowTextW (GetDlgItem (hDlg, IDC_NMESH), Utf8ToWideS(cbuf).c_str());
	sprintf (cbuf, oapiTr("(0 to %d)"), g_ngrp-1);
	SetWindowTextW (GetDlgItem (hDlg, IDC_NGRP), Utf8ToWideS(cbuf).c_str());
}

// ==============================================================
// Windows message handler for the dialog box

INT_PTR CALLBACK MsgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:          // initialise dialog
		ZhLocalizeMeshDlg(hDlg);
		GetMeshParams();
		RefreshDialog (hDlg);
		return TRUE;
	case WM_NOTIFY:
		switch (wParam) {
		case IDC_MESHSPIN:
			if (((LPNMHDR)lParam)->code == UDN_DELTAPOS) {
				int m = ((LPNMUPDOWN)lParam)->iPos + ((LPNMUPDOWN)lParam)->iDelta;
				m = min ((int)g_nmesh-1, max (0, m));
				if (m != g_imesh) ChangeMesh (m);
				RefreshDialog (hDlg);
			}
			return 0;
		case IDC_GROUPSPIN:
			if (((LPNMHDR)lParam)->code == UDN_DELTAPOS) {
				int g = ((LPNMUPDOWN)lParam)->iPos + ((LPNMUPDOWN)lParam)->iDelta;
				g = min ((int)g_ngrp-1, max (0, g));
				if (g != g_igrp) ChangeGroup (g);
			}
			return 0;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD (wParam)) {

		case IDCANCEL: // dialog closed by user
			CloseDlg (hDlg);
			return TRUE;
		}
		break;
	}
	return oapiDefDialogProc (hDlg, uMsg, wParam, lParam);
}
