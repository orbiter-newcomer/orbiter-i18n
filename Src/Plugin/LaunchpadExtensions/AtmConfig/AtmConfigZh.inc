// Chinese localization for the atmosphere configuration dialog (moved out of AtmConfig.cpp)
// ---- Chinese localization for this dialog (English resources remain the master copy) ----
static const wchar_t *ZhUiText(const wchar_t *en)
{
	static const struct { const wchar_t *en; const wchar_t *zh; } map[] = {
		{ L"Atmospheric parameter configuration", L"大气参数配置" },
		{ L"OK", L"确定" },
		{ L"Cancel", L"取消" },
		{ L"Apply", L"应用" },
		{ L"Close", L"关闭" },
		{ L"Help", L"帮助" },
		{ L"Atmosphere model", L"大气模型" },
		{ L"Celestial body", L"天体" },

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
	SetWindowTextW(hDlg, ZhUiText(L"Atmospheric parameter configuration"));
	EnumChildWindows(hDlg, ZhLocalizeChild, 0);
}
