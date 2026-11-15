// Chinese UI localization for the Scenario Editor dialogs.
// Moved out of Editor.cpp to keep the upstream source diff minimal.
// Included by Editor.cpp (English mode: all functions return immediately).
//
// Migration note: the English->Chinese text data was moved into the core
// translation table (i18n/core/PluginZh.inc). This file only contains the
// localization logic; all texts are queried through oapiTr(). The .rc resources
// stay English (used as-is in English mode); in Chinese mode the dialog controls
// are re-labelled when the dialog is initialised.

// Add a combo box item with Chinese UI support (English mode: original text)
static void AddListEntry(HWND hDlg, int id, const char *text)
{
	if (oapiZhActive() == 0) {
		SendDlgItemMessageA(hDlg, id, CB_ADDSTRING, 0, (LPARAM)text);
	} else {
		SendDlgItemMessageW(hDlg, id, CB_ADDSTRING, 0, (LPARAM)oapiUtf8ToWString(oapiTr(text)).c_str());
	}
}

// Replace the text of a control with the translation of its current English text
static void ZhTranslateControl(HWND hWnd)
{
	wchar_t txt[512];
	if (GetWindowTextW(hWnd, txt, 512) <= 0) return;
	char utf8[2048];
	if (oapiWideToUtf8(txt, utf8, sizeof(utf8)) <= 0) return;
	const char *zh = oapiTr(utf8);
	if (strcmp(zh, utf8) != 0) SetWindowTextW(hWnd, oapiUtf8ToWString(zh).c_str());
}

static BOOL CALLBACK ZhScnLocCb(HWND hWnd, LPARAM)
{
    ZhTranslateControl(hWnd);
    return TRUE;
}

static void ZhLocalizeScnDlg(HWND hDlg)
{
    if (oapiZhActive() == 0) return;  // English mode: keep original
    wchar_t title[256];
    if (GetWindowTextW(hDlg, title, 256) > 0) ZhTranslateControl(hDlg);

    // Secondary-parameter labels: after translation, measure the widest Chinese
    // label with the dialog font and build a uniform "label：value" two-column
    // layout: label column at x=220, value column right after the widest label.
    // Labels are collected BEFORE translation (matched by original English text).
    struct LB { const char *en; int valId; } labels[] = {
        { "Orbit period:", IDC_PERIOD }, { "Periapsis:", IDC_PERIAPSIS },
        { "Apoapsis:", IDC_APOAPSIS }, { "T. periapsis:", IDC_PET },
        { "T. apoapsis:", IDC_APT }, { "Mn anomaly:", IDC_MNANM },
        { "Tr anomaly:", IDC_TRANM }, { "Mn longitude:", IDC_MNLNG },
        { "Tr longitude:", IDC_TRLNG }
    };
    const int NLBL = (int)(sizeof(labels)/sizeof(labels[0]));
    HWND hLabel[NLBL] = {0};
    int  vLabel[NLBL] = {0};
    int  nLabel = 0;
    HWND hChild = GetWindow(hDlg, GW_CHILD);
    while (hChild) {
        wchar_t txt[256];
        if (GetWindowTextW(hChild, txt, 256) > 0) {
            char utf8[1024];
            if (oapiWideToUtf8(txt, utf8, sizeof(utf8)) > 0) {
                for (int i = 0; i < NLBL; i++) {
                    if (!strcmp(utf8, labels[i].en) && nLabel < NLBL) {
                        hLabel[nLabel] = hChild;
                        vLabel[nLabel] = labels[i].valId;
                        nLabel++;
                        break;
                    }
                }
            }
        }
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }

    // Orientation page ("Rotate" group): "Pitch"/"Yaw"/"Bank" labels are a bit too
    // narrow for the two-character Chinese text (e.g. "偏航" was cut to "偏"),
    // and their original x positions differ. Widen to the measured Chinese width
    // and left-align all three to the same column.
    // Must run BEFORE the translation below (labels are matched by English text).
    {
        struct OLB { const char *en; } olbl[] = { { "Pitch" }, { "Yaw" }, { "Bank" } };
        const int NOLB = (int)(sizeof(olbl)/sizeof(olbl[0]));
        HWND hFound[NOLB] = {0};
        int  iFound[NOLB] = {0};
        int  nFound = 0;
        int  minLeft = 0x7fffffff;
        HWND hO = GetWindow(hDlg, GW_CHILD);
        while (hO) {
            wchar_t txt[256];
            if (GetWindowTextW(hO, txt, 256) > 0) {
                char utf8[1024];
                if (oapiWideToUtf8(txt, utf8, sizeof(utf8)) > 0) {
                    for (int i = 0; i < NOLB; i++) {
                        if (!strcmp(utf8, olbl[i].en) && nFound < NOLB) {
                            RECT r; GetWindowRect(hO, &r);
                            MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
                            if (r.left < minLeft) minLeft = r.left;
                            hFound[nFound] = hO;
                            iFound[nFound] = i;
                            nFound++;
                            break;
                        }
                    }
                }
            }
            hO = GetWindow(hO, GW_HWNDNEXT);
        }
        if (nFound > 0) {
            HDC hdcO = GetDC(hDlg);
            for (int k = 0; k < nFound; k++) {
                RECT r; GetWindowRect(hFound[k], &r);
                MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
                HFONT hf = (HFONT)SendMessageW(hFound[k], WM_GETFONT, 0, 0);
                HGDIOBJ old = hf ? SelectObject(hdcO, hf) : NULL;
                std::wstring wzh = oapiUtf8ToWString(oapiTr(olbl[iFound[k]].en));
                SIZE sz = {0, 0};
                GetTextExtentPoint32W(hdcO, wzh.c_str(), (int)wzh.size(), &sz);
                if (old) SelectObject(hdcO, old);
                int wNew = sz.cx + 8;
                int wOld = r.right - r.left;
                if (wNew < wOld) wNew = wOld;
                // left-align all three labels to the same column
                SetWindowPos(hFound[k], NULL, minLeft, r.top, wNew, r.bottom-r.top, SWP_NOZORDER);
            }
            ReleaseDC(hDlg, hdcO);
        }
    }

    EnumChildWindows(hDlg, ZhScnLocCb, 0);   // translate all control texts

    // Propellant page: the "Empty"/"Full" buttons (31x11 dialog units) are too small
    // for the Chinese glyphs ("空"/"满"), which overflow the button rect.
    // Enlarge them around their centre so the text fits.
    {
        struct PB { int id; const char *en; } pbtn[] = {
            { IDC_EMPTY, "Empty" }, { IDC_FULL, "Full" }
        };
        HDC hdcP = GetDC(hDlg);
        for (int i = 0; i < (int)(sizeof(pbtn)/sizeof(pbtn[0])); i++) {
            HWND hB = GetDlgItem(hDlg, pbtn[i].id);
            if (!hB) continue;
            RECT r; GetWindowRect(hB, &r);
            MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
            HFONT hf = (HFONT)SendMessageW(hB, WM_GETFONT, 0, 0);
            HGDIOBJ old = hf ? SelectObject(hdcP, hf) : NULL;
            std::wstring wzh = oapiUtf8ToWString(oapiTr(pbtn[i].en));
            SIZE sz = {0, 0};
            GetTextExtentPoint32W(hdcP, wzh.c_str(), (int)wzh.size(), &sz);
            if (old) SelectObject(hdcP, old);
            int w = r.right - r.left, h = r.bottom - r.top;
            int nw = sz.cx + 16, nh = sz.cy + 8;
            if (nw < w) nw = w;
            if (nh < h) nh = h;
            SetWindowPos(hB, NULL, r.left - (nw-w)/2, r.top - (nh-h)/2, nw, nh, SWP_NOZORDER);
        }
        ReleaseDC(hDlg, hdcP);
    }

    if (nLabel > 0) {
        RECT cr; GetClientRect(hDlg, &cr);   // real pixel size of the client area
        // measure the widest translated label using its own dialog font
        HDC hdc = GetDC(hDlg);
        int maxW = 0;
        for (int i = 0; i < nLabel; i++) {
            wchar_t txt[256];
            GetWindowTextW(hLabel[i], txt, 256);
            HFONT hf = (HFONT)SendMessageW(hLabel[i], WM_GETFONT, 0, 0);
            HGDIOBJ old = hf ? SelectObject(hdc, hf) : NULL;
            SIZE sz = {0, 0};
            GetTextExtentPoint32W(hdc, txt, (int)wcslen(txt), &sz);
            if (old) SelectObject(hdc, old);
            if (sz.cx > maxW) maxW = sz.cx;
        }
        ReleaseDC(hDlg, hdc);
        const int LBL_X = 220;
        const int LBL_W = maxW;            // label column: exactly the widest text
        const int VAL_X = LBL_X + LBL_W - 1;   // value column: 5px left of the old 4px gap
        for (int i = 0; i < nLabel; i++) {
            RECT r; GetWindowRect(hLabel[i], &r);
            MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
            SetWindowPos(hLabel[i], NULL, LBL_X, r.top, LBL_W, r.bottom-r.top, SWP_NOZORDER);
            HWND hVal = GetDlgItem(hDlg, vLabel[i]);
            if (hVal) {
                RECT rv; GetWindowRect(hVal, &rv);
                MapWindowPoints(NULL, hDlg, (POINT*)&rv, 2);
                int valW = cr.right - VAL_X - 4;
                if (valW < 24) valW = 24;
                SetWindowPos(hVal, NULL, VAL_X, rv.top, valW, rv.bottom-rv.top, SWP_NOZORDER);
            }
        }
    }
}
