// =============================================================================
// LaunchpadZh.inc - 面板窗口的汉化逻辑
//
// 段 A（LAUNCHPAD_ZH_TABLE）: 历史遗留占位（启动器窗口翻译数据已外置到
//   i18n/locales/zh_CN/*.po，由 LocalizeWindowTree 自动使用）。
// 段 B（LAUNCHPAD_ZH_EXTRA）: Extra 选项卡中需要直接设置控件的补充文本
//   （轨道稳定化参数对话框）。文本本身在 .po（英文为键），此处只保留逻辑。
// =============================================================================
#ifndef LAUNCHPADZH_INC
#define LAUNCHPADZH_INC

// ---- 段 B：Extra 选项卡补充文本（Win32 控件直接设置，文本来自 .po）----
#if defined(LAUNCHPAD_ZH_EXTRA)
// Chinese localisation for the orbit stabilisation page (moved out of TabExtra.cpp).
// English resources keep the master copy; translations come from launchpad.po.
static void ZhLocalizeStabilisation(HWND hWnd)
{
	if (!orbiter::IsChinese()) return;   // english mode: leave resources untouched
	struct Item { int id; const char *en; };
	static const Item items[] = {
		{ IDC_STAB_ENABLE, "Enable orbit stabilisation" },
		{ IDC_STATIC2,     "Orbit step limit" },
		{ IDC_STATIC6,     "Enable stabilisation for orbit steps larger than" },
		{ IDC_STATIC7,     "% of full orbit" },
		{ IDC_STATIC1,     "Gravitational field perturbation limit" },
		{ IDC_STATIC4,     "Suppress stabilisation if gravity perturbations are greater than" },
		{ IDC_STATIC5,     "% of total field" },
		{ IDC_STATIC3,     "Subsampling of perturbation integration" },
		{ IDC_STATIC8,     "Target orbit step" },
		{ IDC_STATIC10,    "Max. steps" },
		{ IDC_STATIC9,     "% of full orbit" },
		{ IDC_STATIC11,    "Perturbation model" },
		{ IDC_STATIC12,    "Suppress nonspherical gravity perturbations for orbit steps greater than" },
		{ IDC_STATIC13,    "% of full orbit" },
		{ IDOK,            "OK" },
		{ IDCANCEL,        "Cancel" },
		{ IDC_BUTTON1,     "Reset" },
		{ IDC_BUTTON2,     "Help" }
	};
	orbiter::SetWindowTextUtf8(hWnd, orbiter::Tr("Orbiter: Orbit stabilisation parameters"));
	for (int i = 0; i < (int)(sizeof(items)/sizeof(items[0])); i++) {
		HWND hCtrl = GetDlgItem(hWnd, items[i].id);
		if (hCtrl) orbiter::SetWindowTextUtf8(hCtrl, orbiter::Tr(items[i].en));
	}
}

#endif // LAUNCHPAD_ZH_EXTRA

#endif // LAUNCHPADZH_INC
