// =============================================================================
// ShuttleAToolsZh.inc - ShuttleA 飞船「游戏外」汉化逻辑
//
// 迁移说明：英文为键、中文为值的数据已并入核心翻译表（.po 文件）；
// 本文件只保留场景编辑器载具编辑页的本地化逻辑：
//   - 控件文本翻译（oapiTr）
//   - 中文布局适配（原版布局 + 测量/偏置，见 oapiZhFitControl*）
// 偏置数值来自 i18n/layout/zh_CN/layout.ini -> [dialog.shuttle_a_anim]。
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

// 中文模式控件适配（原版布局 + 偏置）：文字加宽 / 同排按钮重排
static void ZhFitSaControls (HWND hDlg)
{
    const int padX = oapiGetLayoutInt ("dialog.shuttle_a_anim", "buttonPadX", 20);
    const int gap  = oapiGetLayoutInt ("dialog.shuttle_a_anim", "buttonGap", 4);
    const int rowX = oapiGetLayoutInt ("dialog.shuttle_a_anim", "auxRowX", 15);

    // 起落架 / 对接口 / 气闸：每个按钮按中文宽度加宽（成对垂直排列，不重排）
    static const int btnIds[] = {
        IDC_GEAR_UP, IDC_GEAR_DOWN,
        IDC_DPORT_CLOSE, IDC_DPORT_OPEN,
        IDC_OLOCK_CLOSE, IDC_OLOCK_OPEN
    };
    for (int i = 0; i < (int)(sizeof(btnIds)/sizeof(btnIds[0])); i++)
        oapiZhFitControlText (hDlg, btnIds[i], padX);

    // 辅助吊舱：Retro / Hover / Fwd 三个按钮按中文宽度从左重排
    static const int auxRow[] = { IDC_AUX_RETRO, IDC_AUX_HOVER, IDC_AUX_FWD };
    oapiZhFitControlRow (hDlg, auxRow, 3, padX, gap, rowX);

    // 同步（Sync）按钮：按中文宽度加宽
    oapiZhFitControlText (hDlg, IDC_AUX_SYNC, padX);
}

static void ZhLocalizeSaPage (HWND hDlg)
{
    if (oapiZhActive() == 0) return;
    EnumChildWindows (hDlg, SaPageLocCb, 0);
    ZhFitSaControls (hDlg);
}
