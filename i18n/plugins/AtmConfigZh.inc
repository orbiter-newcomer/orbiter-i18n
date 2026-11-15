// Chinese localization for the atmosphere configuration dialog (moved out of AtmConfig.cpp).
// Translations are registered in the core table (i18n/core/PluginZh.inc); the
// English dialog texts are used as keys via oapiTr().
static BOOL CALLBACK ZhLocalizeChild(HWND hCtrl, LPARAM lp)
{
	wchar_t buf[512];
	if (!GetWindowTextW(hCtrl, buf, 512) || !buf[0]) return TRUE;
	char utf8[1536];
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
