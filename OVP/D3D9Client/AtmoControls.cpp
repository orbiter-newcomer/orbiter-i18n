// ==============================================================
// Atmospheric controls implementation
// Part of the ORBITER VISUALISATION PROJECT (OVP)
// Dual licensed under GPL v3 and LGPL v3
// Copyright (C) 2014-2026 Jarmo Nikkanen
// ==============================================================

#include "D3D9Client.h"
#include "resource.h"
#include "D3D9Config.h"
#include "AtmoControls.h"
#include "Commctrl.h"
#include "vObject.h"
#include "vPlanet.h"
#include "Mesh.h"
#include "Scene.h"
#include <stdio.h>

using namespace oapi;

// UTF-8 -> UTF-16 for the W (Unicode) Win32 API calls (Chinese UI text)
static std::wstring Utf8ToWide(const char *s)
{
	if (!s) return L"";
	int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
	if (n <= 0) return L"";
	std::wstring w(n - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], n);
	return w;
}

extern HINSTANCE g_hInst;
extern D3D9Client *g_client;

// ==============================================================

// Defaut c'tor to init members
ScatterParams::ScatterParams() :
	red      ( 0.650 ),  // 0.400 ... 0.700
	green    ( 0.560 ),  // 0.400 ... 0.700
	blue     ( 0.480 ),  // 0.400 ... 0.700
	rpow	 ( 4.0 ),    // -8.0 ... 8.0
	mpow	 ( 1.0 ),    // -2.0 ... 2.0
	rheight  ( 8.0 ),    // 4.0 ... 40.0 [km]
	mheight  ( 1.0 ),    // 0.0 ... 1.5
	// ----------------------------------------
	trb      ( 1.0 ),    // 0.2 ... 3.0
	tr3D	 ( 1.0 ),
	// ----------------------------------------
	rayrat   ( 1.0 ),    // 0.0 ... 3.0
	ray      ( 1.0 ),	 // 0.0 ... 4.0
	tw_bld	 ( 0.0 ),	 // 0.0 ... 3.5
	// ----------------------------------------
	mie      ( 0.0869 ), // 0.0 ... 8.0
	mphase   ( 0.9831 ), // 0.85 ... 0.999
	// ----------------------------------------
	mierat	 ( 1.0 ),    // 0.0 ... 2.0
	aux2	 ( 1.0 ),    // 0.0 ... 2.0
	aux3	 ( 2.0 ),
	tgamma	 ( 1.0 ),
	mphaseb  ( 1.0 ),
	hazei	 ( 1.0 ),
	tw_bri   ( 0.0 ),
	tw_dst	 ( 0.0 ),
	// ----------------------------------------
	orbalt	 ( 250e3 ),
	visalt	 ( 70e3 ),
	wtrans	 ( 0.1 ),
	wspec	 ( 0.8 ),
	wnrml	 ( 1.0 ),
	wboost	 ( 0.0 ),
	zcolor	 (1.0f, 1.0f, 0.9f),
	hcolor	 (1.0f, 0.7f, 0.0f),
	acolor	 (0.9f, 0.9f, 1.0f),
	suni	 (1.0)
{
}

// ==============================================================

namespace AtmoControls {



ScatterParams defs;
ScatterParams *param = NULL;

std::vector<sSlider> Slider;
std::vector<sValue> Values;

DWORD atmpage = 0;
DWORD atmmode = 0;
DWORD dwCmd = NULL;
HWND hDlg = NULL;
vPlanet *vObj = NULL;


// ==============================================================

void SetToolTip(int vid, PTSTR pszText)
{
	if (!pszText) return;
	for (auto& x : Values) if (x.sprm == vid) {
		x.tooltip = string(pszText);   // store original English; translated at display time
		break;
	}
}

// ==============================================================

void InitToolTips()
{
	if (!hDlg) return;

	for (auto& s : Slider)
	{
		if (!s.hWnd || !s.hwndTip) return;

		if (s.val && s.val->tooltip.size() > 2)
		{
			TOOLINFO toolInfo = { 0 };
			toolInfo.cbSize = sizeof(toolInfo);
			toolInfo.hwnd = hDlg;
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
			toolInfo.uId = (UINT_PTR)s.hWnd;
			toolInfo.lpszText = LPSTR(s.val->tooltip.c_str());
			SendMessage(s.hwndTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&toolInfo);
			SendMessage(s.hwndTip, TTM_ACTIVATE, TRUE, 0);
		}
		else {
			SendMessage(s.hwndTip, TTM_ACTIVATE, FALSE, 0);
		}
	}
}

// ==============================================================

void Create()
{
	vObj = NULL;
	hDlg = NULL;

	dwCmd = oapiRegisterCustomCmd((char*)"D3D9 Atmospheric Controls", (char*)"This dialog allows to control various atmospheric parameters and effects", OpenDlgClbk, NULL);

	Slider.resize(ATM_SLIDER_COUNT);

	for (int i = 0; i < ATM_SLIDER_COUNT; i++) {
		Slider[i].res = IDC_ATM_S1 + i;
		Slider[i].dsp = IDC_ATD_S1 + i;
		Slider[i].lbl = IDC_ATL_S1 + i;
	}

	// Style flags
	// 0x1 = unit in [km] 
	// 0x2 = same for orbital and surface setup
	// 0x8 = x^2 "linearization"
	// 0x10 = x^0.5 "linearization"
	// 0x20 = x^4 "linearization"
	// 0x40 = no 'lerp', special handling 

	// Slider ID, Value ID, Page ID, Label, min, max, flags

	// PAGE 0 ------------------------------------------------
	ConfigValue(1,  0,  0, "Distance", 0.0, 0.2);
	ConfigValue(2,  2,  0, "Terrain", 0.01, 3.0, 0x8);
	ConfigValue(3,  6,  0, "Building", 0.01, 1.0, 0x8);
	// -------------------------------------------------------
	ConfigValue(4,  1,  0, "Green", 0.46, 0.65);
	ConfigValue(5,  3,  0, "R-Pow", -8.0, 12.0);
	ConfigValue(6,  12, 0, "M-Pow", -8.0, 12.0);
	ConfigValue(7,  9,  0, "R-Height", 6.0, 600.0, 0x1 | 0x8);
	ConfigValue(8,  11, 0, "M-Height", 0.5, 10.0, 0x1 | 0x8);
	// -------------------------------------------------------
	ConfigValue(9,  13, 0, "Brightness", 0.1, 8.0, 0x8);
	ConfigValue(10, 16, 0, "Gamma", 0.2, 1.5);
	ConfigValue(11, 19, 0, "Light/Shad", 0.0, 5.0, 32);
	// -------------------------------------------------------
	ConfigValue(12, 5,  0, "Density", 0.0, 10.0, 8);
	ConfigValue(13, 4,  0, "Ratio", 0.2, 5.0, 0x20);
	// -------------------------------------------------------
	ConfigValue(14, 7,  0, "Density", 0.01, 10.0, 0x8);
	ConfigValue(15, 8,  0, "Phase-A", 0.02, 0.999, 0x8);
	ConfigValue(16, 17, 0, "Phase-B", 0.0, 8.0);
	ConfigValue(17, 14, 0, "Ratio", 0.2, 5.0, 0x20);
	// -------------------------------------------------------
	ConfigValue(18, 10, 0, "CloudAlt", 0.2, 10.0, 0x1 | 0x40);	// Clouds altitude (km)
	ConfigValue(19, 15, 0, "HDR", 0.1, 4.0, 0x8);	// HDR
	ConfigValue(20, 18, 0, "Intensity", 0.0, 5.0, 0x8 | 0x40);	// Clouds intensity

	// PAGE 1 ------------------------------------------------
	ConfigValue(14,  20, 1, "Normals", 0.2, 2.0, 0x8);
	ConfigValue(15,  21, 1, "Spec",  0.5, 1.5);
	ConfigValue(16,  22, 1, "Color", 0.01, 0.4);
	ConfigValue(17,  23, 1, " ", 0.01, 1.0);


	SetToolTip(0, (char*)"Light travel distance behind terminator");
	SetToolTip(1, (char*)"Green wave lenght. (Green balance)");
	SetToolTip(2, (char*)"Terrain brightness during twilight");
	SetToolTip(3, (char*)"Main control for atmospheric rayleigh color composition (4.0 for the Earth)");
	SetToolTip(12, (char*)"Main control for atmospheric mie color composition");
	SetToolTip(9, (char*)"Atmosphere Ray scale height (7km - 9km for the Earth)");
	SetToolTip(11, (char*)"Atmosphere Mie scale height (0.6km - 2km for the Earth)");
	// -------------------------------------------------------
	SetToolTip(13, (char*)"Terrain/Ocean brightness control (default 1.0)");
	SetToolTip(16, (char*)"Terrain/Ocean gamma control value (default 1.0)");
	SetToolTip(19, (char*)"Terrain light and shadow boost");
	// -------------------------------------------------------
	SetToolTip(5, (char*)"Overall control for rayleigh scattering (i.e. Haze stickness, atmosphere transparency, optical depth");
	SetToolTip(4, (char*)"Rayleigh in-scatter out-scatter ratio (1.0 nominal)");
	SetToolTip(6, (char*)"Building lighting level");
	// -------------------------------------------------------
	SetToolTip(7, (char*)"Overall scale factor for mie scattering. (Mie-particle density)");
	SetToolTip(8, (char*)"Directional strength of Henyey-Greenstein phase function");
	SetToolTip(14, (char*)"Mie in-scatter out-scatter ratio (1.0 nominal)");
	// -------------------------------------------------------
	SetToolTip(10, (char*)"Altitude for cloud lighting calculations");
	SetToolTip(15, (char*)"'HDR' Exposure factor");
	SetToolTip(17, (char*)"Omnidirectional mie scattering scale factor");
	SetToolTip(18, (char*)"[Dual purpose] Clouds intensity [on surface]. Multiscatter light level [on orbit]");
	// -------------------------------------------------------
	SetToolTip(20, (char*)"Water normal map strength");
	SetToolTip(21, (char*)"Water reflectivity");
	SetToolTip(22, (char*)"Water color / transparency");
	SetToolTip(23, (char*)"Unused");
}

// ==============================================================

bool IsActive()
{
	return (hDlg!=NULL);
}

// ==============================================================

bool Visualize()
{
	if (!hDlg) return false;
	return SendDlgItemMessage(hDlg, IDC_ATM_DISPLAY, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

// ==============================================================

void Release()
{
	if (dwCmd) oapiUnregisterCustomCmd(dwCmd);
	dwCmd = NULL;
}

// ==============================================================

void OpenDlgClbk(void *context)
{
	HWND l_hDlg = oapiOpenDialog(g_hInst, IDD_D3D9SCATTER, WndProc);

	if (l_hDlg) hDlg = l_hDlg; // otherwise open already
	else return;

	Scene *scene = g_client->GetScene();
	
	if (scene) vObj = scene->GetCameraNearVisual();

	if (vObj) param = vObj->GetAtmoParams(atmmode);
	else      param = &defs;

	SendDlgItemMessageW(hDlg, IDC_ATM_MODE, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessageW(hDlg, IDC_ATM_MODE, CB_ADDSTRING, 0, (LPARAM)Utf8ToWide(oapiTr("Auto")).c_str());
	SendDlgItemMessageW(hDlg, IDC_ATM_MODE, CB_ADDSTRING, 0, (LPARAM)Utf8ToWide(oapiTr("Surface")).c_str());
	SendDlgItemMessageW(hDlg, IDC_ATM_MODE, CB_ADDSTRING, 0, (LPARAM)Utf8ToWide(oapiTr("Low Orbit")).c_str());
	SendDlgItemMessageW(hDlg, IDC_ATM_MODE, CB_ADDSTRING, 0, (LPARAM)Utf8ToWide(oapiTr("High Orbit")).c_str());
	SendDlgItemMessageW(hDlg, IDC_ATM_MODE, CB_SETCURSEL, atmmode, 0);

	SendDlgItemMessageW(hDlg, IDC_ATM_PAGE, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessageW(hDlg, IDC_ATM_PAGE, CB_ADDSTRING, 0, (LPARAM)Utf8ToWide(oapiTr("Atmospheric Controls")).c_str());
	SendDlgItemMessageW(hDlg, IDC_ATM_PAGE, CB_ADDSTRING, 0, (LPARAM)Utf8ToWide(oapiTr("Water and Sun-glare")).c_str());
	SendDlgItemMessageW(hDlg, IDC_ATM_PAGE, CB_SETCURSEL, atmpage, 0);

	for (auto& s : Slider)
	{
		s.hWnd = GetDlgItem(hDlg, s.res);
		s.hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hDlg, NULL, g_hInst, NULL);
		TOOLINFO toolInfo = { 0 };
		toolInfo.cbSize = sizeof(toolInfo);
		toolInfo.hwnd = hDlg;
		toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		toolInfo.uId = (UINT_PTR)s.hWnd;	
		toolInfo.lpszText = "ToolTip";
		SendMessage(s.hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
	}

	SetTimer(hDlg, 0, 1000, NULL);

	atmpage = 0;
	InitPage(atmpage);
}

// ==============================================================

void InitPage(int p)
{
	for (auto& s : Slider) s.val = nullptr;

	for (auto& v : Values) if (v.page == p) {
		assert(v.slider >= 0 && v.slider < ATM_SLIDER_COUNT);
		Slider[v.slider].val = &v;
	}

	for (auto& s : Slider) {
		if (s.val) SetWindowTextW(GetDlgItem(hDlg, s.lbl), Utf8ToWide(oapiTr(s.val->lbl.c_str())).c_str());
		else SetWindowTextW(GetDlgItem(hDlg, s.lbl), L" ");
	}

	if (p == 0) {
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG1), Utf8ToWide(oapiTr("Twilight and Ambient settings")).c_str());
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG2), Utf8ToWide(oapiTr("Wavelenght and Scale height settings")).c_str());
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG3), Utf8ToWide(oapiTr("Terrain")).c_str());
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG4), Utf8ToWide(oapiTr("Rayleigh settings")).c_str());
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG5), Utf8ToWide(oapiTr("Mie settings")).c_str());
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG6), Utf8ToWide(oapiTr("Custom settings")).c_str());
	}

	if (p == 1) {
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG1), L" ");
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG2), L" ");
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG3), L" ");
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG4), L" ");
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG5), Utf8ToWide(oapiTr("Water Rendering")).c_str());
		SetWindowTextW(GetDlgItem(hDlg, IDC_ATG6), L" ");
	}

	InitToolTips();
	UpdateSliders();
}

// ==============================================================

double GetValue(int id)
{
	for (auto& s : Values) if (s.id == id) {
		assert(s.sprm >= 0 && s.sprm < ATM_DATA_COUNT);
		return param->data[s.sprm];
	}
	LogErr("Invalid Slider ID in AtmoControls");
	return 0.0;
}

// ==============================================================

void ConfigValue(int sid, int vid, int pid, string lbl, double min, double max, int style)
{
	// Slider ID, Value ID, Page ID, Label, min, max, flags
	sValue v;
	v.max = max;
	v.min = min;
	v.style = style;
	v.page = pid;
	v.sprm = vid;
	v.slider = sid - 1;
	v.lbl = lbl;   // store original English; translated at display time so runtime language switches apply
	Values.push_back(v);
}

// ==============================================================

void SetSlider(sSlider& s)
{
	if (!vObj) return;
	if (!s.val) return;

	auto pos = SendDlgItemMessage(hDlg, s.res, TBM_GETPOS, 0, 0);
	auto v = s.val;
	double x = (1000.0-double(pos))/1000.0;

	assert(v->sprm >= 0 && v->sprm < ATM_DATA_COUNT);

	if (v->style & 8) x = x * x;
	if (v->style & 16) x = sqrt(x);
	if (v->style & 32) x = pow(x, 4.0);

	double val = v->min * (1.0 - x) + v->max * x;
		
	if (v->style&2) {
		vObj->GetAtmoParams(1)->data[v->sprm] = val;
		vObj->GetAtmoParams(2)->data[v->sprm] = val;
		vObj->GetAtmoParams(3)->data[v->sprm] = val;
	}
	else {
		param->data[v->sprm] = val;
	}

	UpdateSlider(s, false); // Update value display
	return;
}

// ==============================================================

void UpdateSliders()
{
	for (auto& s : Slider) UpdateSlider(s, true);
}

// ==============================================================

void UpdateSlider(sSlider& q, bool bSetPos)
{
	char buf[128];
	if (!param) return;

	if (q.val)
	{
		auto v = q.val;
		assert(v->sprm >= 0 && v->sprm < ATM_DATA_COUNT);
		double val = param->data[v->sprm];

		if (bSetPos) {

			SendDlgItemMessage(hDlg, q.res, TBM_SETRANGEMAX, 1, 1000);
			SendDlgItemMessage(hDlg, q.res, TBM_SETRANGEMIN, 1, 0);
			SendDlgItemMessage(hDlg, q.res, TBM_SETTICFREQ, 1, 0);

			double x = (val - v->min) / (v->max - v->min);
			if (v->style & 8) x = sqrt(x);
			if (v->style & 16) x = x * x;
			if (v->style & 32) x = pow(x, 0.25);
			DWORD dpos = 1000 - DWORD(x * 1000.0);

			SendDlgItemMessage(hDlg, q.res, TBM_SETPOS, 1, dpos);
		}

		if (v->style & 1) sprintf_s(buf, 128, "%.1lf k", val);
		else sprintf_s(buf, 128, "%.3lf", val);
	}
	else {
		strcpy_s(buf, 32, " ");
		SendDlgItemMessage(hDlg, q.res, TBM_SETPOS, 1, 0);
	}
	
	SetWindowTextA(GetDlgItem(hDlg, q.dsp), buf);
	return;
}

// ==============================================================

vPlanet * GetVisual()
{
	return vObj;
}

// ==============================================================

void SetVisual(vObject *vo)
{
	if (!vo) {
		vObj = NULL;
		param = &defs;
		return;
	}

	if (!hDlg || !dwCmd) return;
	
	OBJHANDLE hObj = vo->GetObjectA();

	if (oapiGetObjectType(hObj)!=OBJTP_PLANET) {
		LogErr("Invalid Object Type in AtmoControls");
		vObj = NULL;
		param = &defs;
		return;
	}

	vObj = static_cast<vPlanet *>(vo);

	if (vObj) param = vObj->GetAtmoParams(0);
	else	  param = &defs;

	UpdateSliders();
}


#include "AtmoControlsZh.inc"

static BOOL CALLBACK ZhAtmLocCb(HWND hWnd, LPARAM)
{
    wchar_t txt[256];
    if (GetWindowTextW(hWnd, txt, 256) > 0) {
        for (size_t i = 0; i < sizeof(atmMap)/sizeof(atmMap[0]); i++) {
            if (!wcscmp(txt, atmMap[i].en)) { SetWindowTextW(hWnd, atmMap[i].zh); break; }
        }
    }
    return TRUE;
}

static void ZhLocalizeAtmDialog(HWND hDlg)
{
    if (strcmp(oapiTr("Build"), "Build") == 0) return;  // English mode: keep original
    EnumChildWindows(hDlg, ZhAtmLocCb, 0);
}
INT_PTR CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool bOrbOld = false;

	switch (uMsg) {

	case WM_INITDIALOG:
		ZhLocalizeAtmDialog(hWnd);
	{
		vObject *vPl = g_client->GetScene()->GetCameraNearVisual();
		SetVisual(vPl);
		return true;
	}

	case WM_TIMER:
	{
		if (vObj) {	
			char title[256]; string file = "??";
			sprintf_s(title, 256, "%s [%s]", oapiTr("Atmospheric Controls"), vObj->GetName());

			auto cfg = Config->AtmoCfg.find(vObj->GetName());
			if (cfg != Config->AtmoCfg.end()) file = cfg->second;

			param = vObj->GetAtmoParams(atmmode);

			double vd = 0.5f;
			if (atmmode == 0) vd = param->cfg_alt < 0.9999f ? param->cfg_alt : 1.0 - param->cfg_halt;

			if (vObj->GetAtmoMode() == 1) sprintf_s(title, 256, "%s [%s][%s] [%s] (%3.1f%%)", oapiTr("Atmospheric Controls"), file.c_str(), vObj->GetName(), oapiTr("Surface"), (1.0 - vd) * 100.0);
			if (vObj->GetAtmoMode() == 2) sprintf_s(title, 256, "%s [%s][%s] [%s] (%3.1f%%)", oapiTr("Atmospheric Controls"), file.c_str(), vObj->GetName(), oapiTr("Low Orbit"), vd * 100.0);
			if (vObj->GetAtmoMode() == 3) sprintf_s(title, 256, "%s [%s][%s] [%s] (%3.1f%%)", oapiTr("Atmospheric Controls"), file.c_str(), vObj->GetName(), oapiTr("High Orbit"), (1.0 - vd) * 100.0);
		
			SetWindowTextW(hDlg, Utf8ToWide(title).c_str());
			UpdateSliders();
		}
		break;	
	}

	case WM_VSCROLL:
	{
		if (LOWORD(wParam) == TB_THUMBTRACK || LOWORD(wParam) == TB_ENDTRACK || LOWORD(wParam) == TB_THUMBPOSITION) {
			for (auto& s : Slider) if (s.hWnd == HWND(lParam)) {
				SetSlider(s);
				return false;
			}
		}
		return false;
	}

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
			case IDCANCEL:  
			case IDOK:
				oapiCloseDialog(hWnd);
				hDlg = NULL;
				return TRUE;

			case IDC_ATM_LOAD:
				if (vObj) {
					vObj->LoadAtmoConfig();
					UpdateSliders();
				}
				break;

			case IDC_ATM_SAVE:
				if (vObj) vObj->SaveAtmoConfig();
				break;

			case IDC_ATM_COPYTO:
				if (vObj) {
					if (vObj->GetAtmoParams(1) != param) memcpy(vObj->GetAtmoParams(1), param, sizeof(ScatterParams));			
				}
				break;

			case IDC_ATM_COPYLOW:
				if (vObj) {
					if (vObj->GetAtmoParams(2) != param) memcpy(vObj->GetAtmoParams(2), param, sizeof(ScatterParams));
				}
				break;

			case IDC_ATM_COPYHIGH:
				if (vObj) {
					if (vObj->GetAtmoParams(3) != param) memcpy(vObj->GetAtmoParams(3), param, sizeof(ScatterParams));
				}
				break;

			case IDC_ATM_MODE:
				if (HIWORD(wParam)==CBN_SELCHANGE) {
					atmmode = DWORD(SendDlgItemMessage(hWnd, IDC_ATM_MODE, CB_GETCURSEL, 0, 0));
				}
				break;

			case IDC_ATM_PAGE:
				if (HIWORD(wParam) == CBN_SELCHANGE) {
					atmpage = DWORD(SendDlgItemMessage(hWnd, IDC_ATM_PAGE, CB_GETCURSEL, 0, 0));
					InitPage(atmpage);
				}
				break;

			default: 
				//LogErr("LOWORD(%hu), HIWORD(0x%hX)",LOWORD(wParam),HIWORD(wParam));
				break;
		}
		break;
	}

	return oapiDefDialogProc(hWnd, uMsg, wParam, lParam);;
}

} //namespace



