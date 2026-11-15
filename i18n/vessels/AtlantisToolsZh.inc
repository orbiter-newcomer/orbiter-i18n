// =============================================================================
// AtlantisToolsZh.inc - Atlantis 飞船「游戏外」汉化逻辑
//
// 迁移说明：英文为键、中文为值的数据已并入核心翻译表（i18n/core/PluginZh.inc）；
// 本文件只保留载具配置对话框的本地化逻辑（文本通过 oapiTr() 查询）。
// 英文模式下不做任何事（.rc 资源保持原样）。
// =============================================================================
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
