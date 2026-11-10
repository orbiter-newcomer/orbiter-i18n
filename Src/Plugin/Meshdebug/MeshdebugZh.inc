// Chinese localization for the mesh debugger dialog (moved out of Meshdebug.cpp)
// UTF-8 -> UTF-16 helper
static std::wstring Utf8ToWideS(const char *s)
{
	if (!s) return L"";
	int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
	if (n <= 0) return L"";
	std::wstring w(n - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], n);
	return w;
}

// Chinese UI localization for the Mesh debugger dialog (rc stays English)
static const struct { const wchar_t *en; const wchar_t *zh; } meshMap[] = {
    { L"Mesh Debugger", L"网格调试器" },
    { L"Visual:", L"视觉体:" },
    { L"Mesh:", L"网格:" },
    { L"Group:", L"组:" }, { L"[None]", L"[无]" }
};
static BOOL CALLBACK ZhMeshLocCb(HWND hWnd, LPARAM)
{
    wchar_t txt[256];
    if (GetWindowTextW(hWnd, txt, 256) > 0) {
        for (size_t i = 0; i < sizeof(meshMap)/sizeof(meshMap[0]); i++) {
            if (!wcscmp(txt, meshMap[i].en)) { SetWindowTextW(hWnd, meshMap[i].zh); break; }
        }
    }
    return TRUE;
}
static void ZhLocalizeMeshDlg(HWND hDlg)
{
    if (strcmp(oapiTr("Build"), "Build") == 0) return;
    wchar_t title[128];
    if (GetWindowTextW(hDlg, title, 128) > 0 && !wcscmp(title, L"Mesh Debugger"))
        SetWindowTextW(hDlg, L"网格调试器");
    EnumChildWindows(hDlg, ZhMeshLocCb, 0);
}
