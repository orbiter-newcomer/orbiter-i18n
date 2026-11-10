// Chinese localization for the Shuttle-A editor page (moved out of ShuttleA.cpp)
// --------------------------------------------------------------
// Chinese UI localization for the Shuttle-A editor page.
// The .rc resource stays English (used as-is in English mode); the
// controls are re-labelled when the page dialog is initialised.
// --------------------------------------------------------------
static const struct { const wchar_t *en; const wchar_t *zh; } saPageMap[] = {
    { L"Shuttle-A animation settings", L"Shuttle-A 动画设置" },
    { L"Landing gear", L"起落架" },
    { L"Up", L"上" },
    { L"Down", L"下" },
    { L"Docking port", L"对接口" },
    { L"Closed", L"关闭" },
    { L"Open", L"打开" },
    { L"Airlock", L"气闸" },
    { L"Auxiliary pods", L"辅助吊舱" },
    { L"Left", L"左" },
    { L"Right", L"右" },
    { L"Retro", L"后向" },
    { L"Fwd", L"前向" },
    { L"Hover", L"悬停" },
    { L"Sync", L"同步" },
    { L"<< Done", L"<< 完成" }
};

static BOOL CALLBACK SaPageLocCb (HWND hWnd, LPARAM)
{
    wchar_t txt[256];
    if (GetWindowTextW (hWnd, txt, 256) > 0) {
        for (size_t i = 0; i < sizeof(saPageMap)/sizeof(saPageMap[0]); i++) {
            if (!wcscmp (txt, saPageMap[i].en)) { SetWindowTextW (hWnd, saPageMap[i].zh); break; }
        }
    }
    return TRUE;
}

static void ZhLocalizeSaPage (HWND hDlg)
{
    if (!oapiTr("Build") || strcmp (oapiTr("Build"), "Build") == 0) return;
    EnumChildWindows (hDlg, SaPageLocCb, 0);
}
