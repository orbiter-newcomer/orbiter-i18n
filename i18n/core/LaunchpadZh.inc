// =============================================================================
// LaunchpadZh.inc - 面板窗口数据：启动器（Launchpad）全部窗口文本
//
// 段 A（LAUNCHPAD_ZH_TABLE）: 启动器主窗口/Scenario/Modules/Video/Extra/About
//   各选项卡、Options 页、加载画面等 Win32 对话框文本；由 Localization.cpp
//   include 并在 BuildTables() 中合并，供 LocalizeWindowTree 自动汉化使用。
// 段 B（LAUNCHPAD_ZH_EXTRA）: Extra 选项卡中需要直接设置控件的补充文本
//   （轨道稳定化参数对话框），由 TabExtra.cpp include 使用。
// =============================================================================
#ifndef LAUNCHPADZH_INC
#define LAUNCHPADZH_INC

// ---- 段 A：启动器窗口文本（翻译表条目）----

// ---- 段 B：Extra 选项卡补充文本（Win32 控件直接设置）----
#if defined(LAUNCHPAD_ZH_EXTRA)
// Chinese localization for the orbit stabilisation page (moved out of TabExtra.cpp)
// ---- Chinese localisation for the orbit stabilisation page (english resources keep the master copy) ----
static void ZhLocalizeStabilisation(HWND hWnd)
{
	if (oapiZhActive() == 0) return;   // english mode: leave resources untouched
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

#endif // LAUNCHPAD_ZH_EXTRA

#endif // LAUNCHPADZH_INC
