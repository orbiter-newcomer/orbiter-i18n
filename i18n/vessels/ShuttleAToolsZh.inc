// =============================================================================
// ShuttleAToolsZh.inc - ShuttleA 飞船「游戏外」汉化逻辑
//
// 迁移说明：英文为键、中文为值的数据已并入核心翻译表（.po 文件）；
// 本文件只保留场景编辑器载具编辑页的本地化逻辑：
//   - 控件文本翻译（oapiTr）
//   - 按钮文字适配（原版布局为基准，按中文文字加宽/加高，同排按钮自动重排）
//   - 辅助吊舱三个按钮的移动：布局 API + layout.ini 偏置
//   - "右"滑块行的下移：布局 API + layout.ini 偏置（左滑块及其他控件不动）
// 偏置数值来自 i18n/layout/zh_CN/layout.ini -> [dialog.shuttle_a_anim]；
// 代码中不写死任何像素坐标或尺寸。英文模式下不做任何事（.rc 资源保持原样）。
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

// Find a child control by its current (English) text. Needed because the two
// slider labels share the same control ID (IDC_STATIC).
static const wchar_t *ZhFindText = 0;
static HWND ZhFoundWnd = 0;
static BOOL CALLBACK ZhFindTextCb (HWND hWnd, LPARAM)
{
    wchar_t buf[128];
    if (GetWindowTextW (hWnd, buf, 128) > 0 && wcscmp (buf, ZhFindText) == 0) {
        ZhFoundWnd = hWnd;
        return FALSE;
    }
    return TRUE;
}
static HWND ZhFindChildByText (HWND hDlg, const wchar_t *text)
{
    ZhFindText = text;
    ZhFoundWnd = 0;
    EnumChildWindows (hDlg, ZhFindTextCb, 0);
    return ZhFoundWnd;
}

static void ZhLocalizeSaPage (HWND hDlg)
{
    if (oapiZhActive() == 0) return;

    // Remember the "Right" slider label before the texts are translated.
    HWND hRightLabel = ZhFindChildByText (hDlg, L"Right");

    EnumChildWindows (hDlg, SaPageLocCb, 0);

    // 文字适配（保留）：官方布局为基准，按钮按中文文字加宽/加高，同排按钮
    // 在加宽后会重叠时自动等距重排（见 oapiZhFitDialogButtons）
    oapiZhFitDialogButtons (hDlg,
        oapiGetLayoutInt ("dialog.vessel_pages", "buttonPadX", 10),
        oapiGetLayoutInt ("dialog.vessel_pages", "buttonPadY", 6));

    // 辅助吊舱按钮行下移（布局 API + layout 偏置）
    static const int auxBtn[] = { IDC_AUX_RETRO, IDC_AUX_HOVER, IDC_AUX_FWD };
    oapiZhOffsetControls (hDlg, auxBtn, 3, 0,
        oapiGetLayoutInt ("dialog.shuttle_a_anim", "auxButtonYOffset", 16));

    // "右"滑块行下移：滑块与标签各自独立的 layout 偏置（可分别调整；
    // 左滑块、同步按钮等其他控件保持不动）
    const int sliderY = oapiGetLayoutInt ("dialog.shuttle_a_anim", "rightSliderYOffset", 6);
    const int labelY  = oapiGetLayoutInt ("dialog.shuttle_a_anim", "rightLabelYOffset", 6);
    if (sliderY != 0) {
        HWND hRSlider = GetDlgItem (hDlg, IDC_RAUX_POS);
        if (hRSlider) oapiZhOffsetWindow (hRSlider, 0, sliderY);
    }
    if (labelY != 0 && hRightLabel)
        oapiZhOffsetWindow (hRightLabel, 0, labelY);
}
