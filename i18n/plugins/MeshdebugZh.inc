// Chinese localization for the mesh debugger dialog (moved out of Meshdebug.cpp).
// Translations are registered in the core table (i18n/core/PluginZh.inc); the
// English control texts are used as keys via oapiTr().
static BOOL CALLBACK ZhMeshLocCb(HWND hWnd, LPARAM)
{
    wchar_t txt[256];
    if (GetWindowTextW(hWnd, txt, 256) > 0) {
        char utf8[1024];
        if (oapiWideToUtf8(txt, utf8, sizeof(utf8)) > 0) {
            const char *zh = oapiTr(utf8);
            if (strcmp(zh, utf8) != 0) SetWindowTextW(hWnd, oapiUtf8ToWString(zh).c_str());
        }
    }
    return TRUE;
}
static void ZhLocalizeMeshDlg(HWND hDlg)
{
    if (oapiZhActive() == 0) return;
    wchar_t title[128];
    if (GetWindowTextW(hDlg, title, 128) > 0) {
        char utf8[512];
        if (oapiWideToUtf8(title, utf8, sizeof(utf8)) > 0) {
            const char *zh = oapiTr(utf8);
            if (strcmp(zh, utf8) != 0) SetWindowTextW(hDlg, oapiUtf8ToWString(zh).c_str());
        }
    }
    EnumChildWindows(hDlg, ZhMeshLocCb, 0);
}
