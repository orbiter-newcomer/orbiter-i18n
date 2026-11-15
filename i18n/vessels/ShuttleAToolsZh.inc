// =============================================================================
// ShuttleAToolsZh.inc - ShuttleA 飞船「游戏外」汉化逻辑
//
// 迁移说明：英文为键、中文为值的数据已并入核心翻译表（i18n/core/PluginZh.inc）；
// 本文件只保留场景编辑器载具编辑页的本地化逻辑（文本通过 oapiTr() 查询）。
// 英文模式下不做任何事（.rc 资源保持原样）。
// =============================================================================
static BOOL CALLBACK SaPageLocCb (HWND hWnd, LPARAM)
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

static void ZhLocalizeSaPage (HWND hDlg)
{
    if (oapiZhActive() == 0) return;
    EnumChildWindows (hDlg, SaPageLocCb, 0);
}
