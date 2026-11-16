// =============================================================================
// DeltaGliderToolsZh.inc - DeltaGlider 飞船「游戏外」汉化逻辑
//
// 两段（宏区分，两个 DLL 各自包含对应段）：
//   DG_ZH_EDITOR : 场景编辑器 → 载具编辑页（动画/乘客/损伤页），DeltaGlider.cpp
//   DG_ZH_CONFIG : 启动器 → 载具配置 → DG Configuration，DGConfigurator.cpp
//
// 迁移说明：英文为键、中文为值的数据已并入核心翻译表（i18n/core/PluginZh.inc）；
// 本文件只保留本地化逻辑（文本通过 oapiTr() 查询）。
// =============================================================================
#if defined(DG_ZH_EDITOR)
// Chinese localization for the Delta-glider editor pages (moved out of DeltaGlider.cpp).
// The .rc resources stay English (used as-is in English mode); the controls are
// re-labelled when a page dialog is initialised.
static BOOL CALLBACK DgPageLocCb (HWND hWnd, LPARAM)
{
    wchar_t txt[256];
    if (GetWindowTextW (hWnd, txt, 256) > 0) {
        char utf8[1024];
        if (oapiWideToUtf8 (txt, utf8, sizeof(utf8)) > 0) {
            const char *zh = oapiTr (utf8);
            if (strcmp (zh, utf8) != 0) SetWindowTextW (hWnd, oapiUtf8ToWString (zh).c_str());
        }
    }
    return TRUE;
}

static void ZhLocalizeDgPage (HWND hDlg)
{
    if (oapiZhActive() == 0) return;
    EnumChildWindows (hDlg, DgPageLocCb, 0);
    // 原版布局 + 偏置：中文按钮按实测文字宽高适配（含同列按钮重排），
    // 偏置数值来自 i18n/layout/zh_CN/layout.ini -> [dialog.vessel_pages]
    oapiZhFitDialogButtons (hDlg,
        oapiGetLayoutInt ("dialog.vessel_pages", "buttonPadX", 10),
        oapiGetLayoutInt ("dialog.vessel_pages", "buttonPadY", 6));
}
#endif // DG_ZH_EDITOR

// =============================================================================
// 启动器「载具配置 → DG Configuration」对话框（DGConfigurator.dll）
// =============================================================================
#if defined(DG_ZH_CONFIG)
// Chinese localization for the Delta-glider configurator dialog (moved out of DGConfigurator.cpp).
// English resources remain the master copy; the Chinese texts come from the core
// translation table via oapiTr().
static BOOL CALLBACK ZhLocalizeChild(HWND hCtrl, LPARAM lp)
{
	wchar_t buf[512];
	if (!GetWindowTextW(hCtrl, buf, 512) || !buf[0]) return TRUE;
	char utf8[2048];
	if (oapiWideToUtf8(buf, utf8, sizeof(utf8)) > 0) {
		const char *zh = oapiTr(utf8);
		if (strcmp(zh, utf8) != 0) SetWindowTextW(hCtrl, oapiUtf8ToWString(zh).c_str());
	}
	return TRUE;
}

static void ZhLocalizeDialog(HWND hDlg)
{
	if (oapiZhActive() == 0) return;   // english mode: keep resources as-is
	wchar_t title[256];
	if (GetWindowTextW(hDlg, title, 256) > 0) {
		char utf8[1024];
		if (oapiWideToUtf8(title, utf8, sizeof(utf8)) > 0) {
			const char *zh = oapiTr(utf8);
			if (strcmp(zh, utf8) != 0) SetWindowTextW(hDlg, oapiUtf8ToWString(zh).c_str());
		}
	}
	EnumChildWindows(hDlg, ZhLocalizeChild, 0);
}
#endif // DG_ZH_CONFIG
