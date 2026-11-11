// =============================================================================
// DeltaGliderToolsZh.inc - DeltaGlider 飞船「游戏外」汉化数据
//
// 两段（宏区分，两个 DLL 各自包含对应段）：
//   DG_ZH_EDITOR : 场景编辑器 → 载具编辑页（动画/乘客/损伤页），DeltaGlider.cpp
//   DG_ZH_CONFIG : 启动器 → 载具配置 → DG Configuration，DGConfigurator.cpp
// =============================================================================
#if defined(DG_ZH_EDITOR)
// =============================================================================
// DeltaGliderZh.inc - DeltaGlider 飞船主模块的汉化数据
//
// 内容：DeltaGlider.cpp 的编辑器页面中文映射 + HUD 中文辅助（AIRBRK→减速板）。
// 启动器载具配置对话框（DG Configuration）的数据属于"面板窗口数据"类，
// 单独放在 DGConfigurator 目录的 DGConfiguratorZh.inc。
// =============================================================================
// Chinese localization for the Delta-glider editor pages (moved out of DeltaGlider.cpp)
// --------------------------------------------------------------
// Chinese UI localization for the Delta-glider editor pages.
// The .rc resources stay English (used as-is in English mode); the
// controls are re-labelled when a page dialog is initialised.
// --------------------------------------------------------------
static const struct { const wchar_t *en; const wchar_t *zh; } dgPageMap[] = {
    // page 1: animation settings
    { L"Delta-glider animation settings", L"Delta-glider 动画设置" },
    { L"Landing gear", L"起落架" },
    { L"Up", L"上" },
    { L"Down", L"下" },
    { L"Retro doors", L"反推门" },
    { L"Closed", L"关闭" },
    { L"Open", L"打开" },
    { L"Outer airlock", L"外气闸" },
    { L"Inner airlock", L"内气闸" },
    { L"Nose cone", L"机头锥" },
    { L"Escape ladder", L"逃生梯" },
    { L"Stowed", L"收起" },
    { L"Extended", L"展开" },
    { L"Radiator", L"散热器" },
    { L"Top hatch", L"顶部舱门" },
    // page 2: passengers
    { L"Delta-glider passengers", L"Delta-glider 乘客" },
    { L"Front left", L"左前" },
    { L"Rear left", L"左后" },
    { L"Front right", L"右前" },
    { L"Rear right", L"右后" },
    { L"Total vessel mass:", L"载具总质量：" },
    // page 3: damage
    { L"Damage", L"损伤" },
    { L"Airframe", L"机体" },
    { L"left wing", L"左翼" },
    { L"right wing", L"右翼" },
    { L"fail", L"失效" },
    { L"operate", L"正常" },
    { L"Repair all", L"全部修复" },
    { L"Refresh", L"刷新" },
    { L"<< Done", L"<< 完成" }
};

static BOOL CALLBACK DgPageLocCb (HWND hWnd, LPARAM)
{
    wchar_t txt[256];
    if (GetWindowTextW (hWnd, txt, 256) > 0) {
        for (size_t i = 0; i < sizeof(dgPageMap)/sizeof(dgPageMap[0]); i++) {
            if (!wcscmp (txt, dgPageMap[i].en)) { SetWindowTextW (hWnd, dgPageMap[i].zh); break; }
        }
    }
    return TRUE;
}

static void ZhLocalizeDgPage (HWND hDlg)
{
    if (!oapiTr("Build") || strcmp (oapiTr("Build"), "Build") == 0) return;
    EnumChildWindows (hDlg, DgPageLocCb, 0);
}
#endif // DG_ZH_EDITOR

// =============================================================================
// 启动器「载具配置 → DG Configuration」对话框（DGConfigurator.dll）
// =============================================================================
#if defined(DG_ZH_CONFIG)
// =============================================================================
// DGConfiguratorZh.inc - 面板窗口数据：启动器「载具配置 → DG Configuration」对话框
//
// 该对话框由 DGConfigurator.dll 注册为 Launchpad 列表项（oapiRegisterLaunchpadItem），
// 只在启动器（Launchpad）中打开，与场景编辑器无关。
// 本文件只放对话框界面中文数据（英文 .rc 资源保持原样，作为英文模式母本）。
// =============================================================================
// Chinese localization for the Delta-glider configurator dialog (moved out of DGConfigurator.cpp)
// ---- Chinese localization for this dialog (English resources remain the master copy) ----
static const wchar_t *ZhUiText(const wchar_t *en)
{
	static const struct { const wchar_t *en; const wchar_t *zh; } map[] = {
		{ L"DG Configuration", L"三角翼配置" },
		{ L"OK", L"确定" },
		{ L"Cancel", L"取消" },
		{ L"Apply", L"应用" },
		{ L"Close", L"关闭" },
		{ L"Help", L"帮助" },
		{ L"Texture resolution", L"纹理分辨率" },
		{ L"High (1024x1024)", L"高（1024x1024）" },
		{ L"Low (512x512)", L"低（512x512）" },

	};
	for (auto &e : map) if (!wcscmp(e.en, en)) return e.zh;
	return nullptr;
}

static BOOL CALLBACK ZhLocalizeChild(HWND hCtrl, LPARAM lp)
{
	wchar_t buf[512];
	if (!GetWindowTextW(hCtrl, buf, 512) || !buf[0]) return TRUE;
	const wchar_t *zh = ZhUiText(buf);
	if (zh) SetWindowTextW(hCtrl, zh);
	return TRUE;
}

static void ZhLocalizeDialog(HWND hDlg)
{
	if (strcmp(oapiTr("Build"), "Build") == 0) return;   // english mode: keep resources as-is
	SetWindowTextW(hDlg, ZhUiText(L"DG Configuration"));
	EnumChildWindows(hDlg, ZhLocalizeChild, 0);
}
#endif // DG_ZH_CONFIG