// =============================================================================
// AtlantisToolsZh.inc - Atlantis 飞船「游戏外」汉化数据
//
// 内容：启动器 → 载具配置 → Atlantis Configuration 对话框（AtlantisConfig.dll）。
// Atlantis 本体在游戏内没有专用文本（界面文本走主程序通用翻译表），
// 因此本船只有这一个「游戏外」数据文件。
// =============================================================================
// =============================================================================
// AtlantisConfigZh.inc - 面板窗口数据：启动器「载具配置 → Atlantis Configuration」对话框
//
// 该对话框由 AtlantisConfig.dll 注册为 Launchpad 列表项（oapiRegisterLaunchpadItem），
// 只在启动器（Launchpad）中打开，与场景编辑器无关。
// 本文件只放对话框界面中文数据（英文 .rc 资源保持原样，作为英文模式母本）。
// =============================================================================
// Chinese localization for the Atlantis configuration dialog (moved out of AtlantisConfig.cpp)
// ---- Chinese localization for this dialog (English resources remain the master copy) ----
static const wchar_t *ZhUiText(const wchar_t *en)
{
	static const struct { const wchar_t *en; const wchar_t *zh; } map[] = {
		{ L"Atlantis Configuration", L"亚特兰蒂斯配置" },
		{ L"OK", L"确定" },
		{ L"Cancel", L"取消" },
		{ L"Apply", L"应用" },
		{ L"Close", L"关闭" },
		{ L"Help", L"帮助" },
		{ L"Atlantis orbiter: Texture resolution", L"亚特兰蒂斯轨道器：纹理分辨率" },
		{ L"Atlantis orbiter: Mesh resolution", L"亚特兰蒂斯轨道器：网格分辨率" },
		{ L"High", L"高" },
		{ L"Low", L"低" },
		{ L"High resolution textures and meshes provide a better visual appearance, but can reduce the performance of the simulator, in particular on less powerful computers.\r\nIf you experience problems, try switching to the low resolution options.", L"高分辨率纹理和网格能提供更好的视觉效果，但会降低模拟器性能，尤其是在性能较弱的计算机上。\r\n如果遇到问题，请尝试切换到低分辨率选项。" },

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
	SetWindowTextW(hDlg, ZhUiText(L"Atlantis Configuration"));
	EnumChildWindows(hDlg, ZhLocalizeChild, 0);
}
