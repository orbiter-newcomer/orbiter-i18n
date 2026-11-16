// =============================================================================
// ShuttleAToolsZh.inc - ShuttleA 飞船「游戏外」汉化逻辑
//
// 迁移说明：英文为键、中文为值的数据已并入核心翻译表（.po 文件）；
// 本文件只保留场景编辑器载具编辑页的本地化逻辑：
//   - 控件文本翻译（oapiTr）
//   - 按钮文字适配（原版布局为基准，按中文文字加宽/加高，不移动按钮）
//   - 辅助吊舱三个按钮的移动：布局 API + layout.ini 偏置（只移动这一行）
// 偏置数值来自 i18n/layout/zh_CN/layout.ini -> [dialog.shuttle_a_anim]；
// 代码中不写死任何像素坐标或尺寸。
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

    // 文字适配（保留）：官方布局为基准，按钮只按中文文字加宽/加高，不移动
    oapiZhFitDialogButtons (hDlg,
        oapiGetLayoutInt ("dialog.vessel_pages", "buttonPadX", 10),
        oapiGetLayoutInt ("dialog.vessel_pages", "buttonPadY", 6));

    // 只把辅助吊舱那一行的三个按钮下移（布局 API + layout.ini 偏置）
    static const int auxBtn[] = { IDC_AUX_RETRO, IDC_AUX_HOVER, IDC_AUX_FWD };
    oapiZhOffsetControls (hDlg, auxBtn, 3, 0,
        oapiGetLayoutInt ("dialog.shuttle_a_anim", "auxButtonYOffset", 16));
}
