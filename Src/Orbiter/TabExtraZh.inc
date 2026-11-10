// Chinese localization for the orbit stabilisation page (moved out of TabExtra.cpp)
// ---- Chinese localisation for the orbit stabilisation page (english resources keep the master copy) ----
static void ZhLocalizeStabilisation(HWND hWnd)
{
	if (strcmp(oapiTr("Build"), "Build") == 0) return;   // english mode: leave resources untouched
	SetWindowTextW(hWnd, L"Orbiter：轨道稳定化参数");
	SetDlgItemTextW(hWnd, IDC_STAB_ENABLE, L"启用轨道稳定化");
	SetDlgItemTextW(hWnd, IDC_STATIC2, L"轨道步长限制");
	SetDlgItemTextW(hWnd, IDC_STATIC1, L"引力场摄动限制");
	SetDlgItemTextW(hWnd, IDC_STATIC3, L"摄动积分子采样");
	SetDlgItemTextW(hWnd, IDC_STATIC11, L"摄动模型");
	SetDlgItemTextW(hWnd, IDC_STATIC6, L"当轨道步长大于整条轨道的百分之");
	SetDlgItemTextW(hWnd, IDC_STATIC7, L"时开启稳定化");
	SetDlgItemTextW(hWnd, IDC_STATIC4, L"当引力摄动大于总场强的百分之");
	SetDlgItemTextW(hWnd, IDC_STATIC5, L"时禁用稳定化");
	SetDlgItemTextW(hWnd, IDC_STATIC8, L"目标轨道步长为整条轨道的百分之");
	SetDlgItemTextW(hWnd, IDC_STATIC9, L"");
	SetDlgItemTextW(hWnd, IDC_STATIC10, L"最大步数");
	SetDlgItemTextW(hWnd, IDC_STATIC12, L"当轨道步长大于整条轨道的百分之");
	SetDlgItemTextW(hWnd, IDC_STATIC13, L"时抑制摄动");
	SetDlgItemTextW(hWnd, IDOK, L"确定");
	SetDlgItemTextW(hWnd, IDCANCEL, L"取消");
	SetDlgItemTextW(hWnd, IDC_BUTTON1, L"重置");
	SetDlgItemTextW(hWnd, IDC_BUTTON2, L"帮助");
}
