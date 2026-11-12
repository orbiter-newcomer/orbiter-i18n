// Chinese UI localization for the Scenario Editor dialogs.
// Moved out of Editor.cpp to keep the upstream source diff minimal.
// Included by Editor.cpp (English mode: all functions return immediately).

// Chinese UI localization for the scenario editor dialogs.
// The .rc resources stay English (used as-is in English mode); in Chinese
// mode the dialog controls are re-labelled when the dialog is initialised.
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

// Add a combo box item with Chinese UI support (English mode: original text)
static void AddListEntry(HWND hDlg, int id, const char *text)
{
	if (strcmp(oapiTr("Build"), "Build") == 0) {
		SendDlgItemMessageA(hDlg, id, CB_ADDSTRING, 0, (LPARAM)text);
	} else {
		SendDlgItemMessageW(hDlg, id, CB_ADDSTRING, 0, (LPARAM)Utf8ToWideS(oapiTr(text)).c_str());
	}
}

static const struct { const wchar_t *en; const wchar_t *zh; } scnMap[] = {
    { L"Scenario Editor", L"场景编辑器" },
    { L"Create >>", L"创建 >>" },
    { L"Vessel name:", L"载具名称：" },
    { L"<< Cancel", L"<< 取消" },
    { L"Vessel type:", L"载具类型：" },
    { L"Set input focus", L"设置输入焦点" },
    { L"Set camera", L"设置相机" },
    { L"<< Done", L"<< 退出" },
    { L"Vessel", L"载具" },
    { L"Class", L"类型" },
    { L"Status", L"状态" },
    { L"Free flight", L"自由飞行" },
    { L"Orbital elements", L"轨道根数" },
    { L"State vectors", L"状态向量" },
    { L"Orientation", L"姿态" },
    { L"Angular velocity", L"角速度" },
    { L"On the ground", L"在地面" },
    { L"Location", L"位置" },
    { L"Misc", L"杂项" },
    { L"Propellant", L"推进剂" },
    { L"Docking", L"对接" },
    { L"Animations", L"动画" },
    { L"Passengers", L"乘客" },
    { L"More", L"更多" },
    { L"Extra 1", L"额外 1" },
    { L"Extra 2", L"额外 2" },
    { L"Extra 3", L"额外 3" },
    { L"Extra 4", L"额外 4" },
    { L"Extra 5", L"额外 5" },
    { L"Extra 6", L"额外 6" },
    { L"Help", L"帮助" },
    { L"Surface position", L"表面位置" },
    { L"Longitude", L"经度" },
    { L"Latitude", L"纬度" },
    { L"Celestial body", L"天体" },
    { L"Surface base", L"地面基地" },
    { L"Reference", L"参考" },
    { L"deg", L"度" },
    { L"Heading", L"航向" },
    { L"Landing pad", L"着陆平台" },
    { L"Apply", L"应用" },
    { L"Refresh", L"刷新" },
    { L"Copy state from", L"复制状态来自" },
    { L"Orbit reference", L"轨道参考" },
    { L"Frame", L"坐标系" },
    { L"Coordinates", L"坐标" },
    { L"m", L"m" },
    { L"m/s", L"m/s" },
    { L"New ...", L"新建 ..." },
    { L"Edit ...", L"编辑 ..." },
    { L"Delete", L"删除" },
    { L"Vessels in current scenario:", L"当前场景中的载具：" },
    { L"Close", L"关闭" },
    { L"Save ...", L"保存 ..." },
    { L"Track", L"跟踪" },
    { L"Date ...", L"日期 ..." },
    { L"Cancel", L"取消" },
    { L"Save", L"保存" },
    { L"Scenario file name", L"场景文件名" },
    { L"Description", L"描述" },
    { L"Title", L"标题" },
    { L"Text", L"文本" },
    { L"Osculating elements", L"密切根数" },
    { L"Semi-major axis [SMa]", L"半长轴 [SMa]" },
    { L"Eccentricity [Ecc]", L"偏心率 [Ecc]" },
    { L"Inclination [Inc]", L"倾角 [Inc]" },
    { L"Longitude of ascending node [LAN]", L"升交点经度 [LAN]" },
    { L"Longitude of periapsis [LPe]", L"近点经度 [LPe]" },
    { L"Mean longitude at epoch [eps]", L"历元平经度 [eps]" },
    { L"Epoch", L"历元" },
    { L"Secondary parameters", L"次要参数" },
    { L"Orbit period:", L"轨道周期：" },
    { L"Periapsis:", L"近点距离：" },
    { L"Apoapsis:", L"远点距离：" },
    { L"T. periapsis:", L"近点时刻：" },
    { L"T. apoapsis:", L"远点时刻：" },
    { L"Mn anomaly:", L"平近点角：" },
    { L"Tr anomaly:", L"真近点角：" },
    { L"Mn longitude:", L"平经度：" },
    { L"Tr longitude:", L"真经度：" },
    { L"Orientation: Euler angles", L"姿态：欧拉角" },
    { L"Rotate", L"旋转" },
    { L"Pitch", L"俯仰" },
    { L"Up", L"上" },
    { L"Down", L"下" },
    { L"Left", L"左" },
    { L"Right", L"右" },
    { L"Yaw", L"偏航" },
    { L"Bank", L"滚转" },
    { L"deg/s", L"度/秒" },
    { L"pitch (omega x)", L"俯仰 (ωx)" },
    { L"yaw (omega y)", L"偏航 (ωy)" },
    { L"bank (omega z)", L"滚转 (ωz)" },
    { L"Kill", L"消除" },
    { L"Propellant levels", L"推进剂液位" },
    { L"Tank no.", L"储箱号" },
    { L"Full", L"满" },
    { L"of 1", L"/ 1" },
    { L"Fill level (0-1)", L"填充水平 (0-1)" },
    { L"Mass", L"质量" },
    { L"Empty", L"空" },
    { L"Empty all", L"全部排空" },
    { L"Fill all", L"全部加注" },
    { L"Total propellant mass:", L"推进剂总质量：" },
    { L"Total vessel mass:", L"载具总质量：" },
    { L"Undock", L"解除对接" },
    { L"Docking status", L"对接状态" },
    { L"Dock no.", L"接口号" },
    { L"Currently docked to", L"当前对接于" },
    { L"Establish docking connection with", L"建立对接连接对象" },
    { L"IDS frequency", L"IDS 频率" },
    { L"Dock", L"对接" },
    { L"using dock", L"使用接口" },
    { L"Bring target to my current position", L"将目标移动到我的当前位置" },
    { L"Move me to current target position", L"将我移动到目标当前位置" },
    { L"Date and time", L"日期与时间" },
    { L"Modified Julian Date (MJD)", L"简化儒略日 (MJD)" },
    { L"Julian Date (JD)", L"儒略日 (JD)" },
    { L"Julian Century (JC2000)", L"儒略世纪 (JC2000)" },
    { L"Vessel state propagation", L"载具状态传播" },
    { L"Orbital", L"轨道" },
    { L"Suborbital", L"亚轨道" },
    { L"Now", L"现在" },
    { L"alpha", L"α" },
    { L"beta", L"β" },
    { L"gamma", L"γ" },
    { L"x", L"x" },
    { L"y", L"y" },
    { L"z", L"z" },
    { L"dx / dt", L"dx / dt" },
    { L"dy / dt", L"dy / dt" },
    { L"dz / dt", L"dz / dt" },
};

static BOOL CALLBACK ZhScnLocCb(HWND hWnd, LPARAM)
{
    wchar_t txt[512];
    if (GetWindowTextW(hWnd, txt, 512) > 0) {
        for (size_t i = 0; i < sizeof(scnMap)/sizeof(scnMap[0]); i++) {
            if (!wcscmp(txt, scnMap[i].en)) { SetWindowTextW(hWnd, scnMap[i].zh); break; }
        }
    }
    return TRUE;
}

static void ZhLocalizeScnDlg(HWND hDlg)
{
    if (strcmp(oapiTr("Build"), "Build") == 0) return;  // English mode: keep original
    wchar_t title[256];
    if (GetWindowTextW(hDlg, title, 256) > 0 && !wcscmp(title, L"Scenario Editor"))
        SetWindowTextW(hDlg, L"场景编辑器");
    // Secondary-parameter labels: after translation, measure the widest Chinese
    // label with the dialog font and build a uniform "label：value" two-column
    // layout: label column at x=220, value column right after the widest label.
    // Labels are collected BEFORE translation (matched by original English text).
    struct LB { const wchar_t *en; int valId; } labels[] = {
        { L"Orbit period:", IDC_PERIOD }, { L"Periapsis:", IDC_PERIAPSIS },
        { L"Apoapsis:", IDC_APOAPSIS }, { L"T. periapsis:", IDC_PET },
        { L"T. apoapsis:", IDC_APT }, { L"Mn anomaly:", IDC_MNANM },
        { L"Tr anomaly:", IDC_TRANM }, { L"Mn longitude:", IDC_MNLNG },
        { L"Tr longitude:", IDC_TRLNG }
    };
    const int NLBL = (int)(sizeof(labels)/sizeof(labels[0]));
    HWND hLabel[NLBL] = {0};
    int  vLabel[NLBL] = {0};
    int  nLabel = 0;
    HWND hChild = GetWindow(hDlg, GW_CHILD);
    while (hChild) {
        wchar_t txt[256];
        if (GetWindowTextW(hChild, txt, 256) > 0) {
            for (int i = 0; i < NLBL; i++) {
                if (!wcscmp(txt, labels[i].en) && nLabel < NLBL) {
                    hLabel[nLabel] = hChild;
                    vLabel[nLabel] = labels[i].valId;
                    nLabel++;
                    break;
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
        struct OLB { const wchar_t *en; const wchar_t *zh; } olbl[] = {
            { L"Pitch", L"俯仰" }, { L"Yaw", L"偏航" }, { L"Bank", L"滚转" }
        };
        const int NOLB = (int)(sizeof(olbl)/sizeof(olbl[0]));
        HWND hFound[NOLB] = {0};
        int  iFound[NOLB] = {0};
        int  nFound = 0;
        int  minLeft = 0x7fffffff;
        HWND hO = GetWindow(hDlg, GW_CHILD);
        while (hO) {
            wchar_t txt[256];
            if (GetWindowTextW(hO, txt, 256) > 0) {
                for (int i = 0; i < NOLB; i++) {
                    if (!wcscmp(txt, olbl[i].en) && nFound < NOLB) {
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
            hO = GetWindow(hO, GW_HWNDNEXT);
        }
        if (nFound > 0) {
            HDC hdcO = GetDC(hDlg);
            for (int k = 0; k < nFound; k++) {
                RECT r; GetWindowRect(hFound[k], &r);
                MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
                HFONT hf = (HFONT)SendMessageW(hFound[k], WM_GETFONT, 0, 0);
                HGDIOBJ old = hf ? SelectObject(hdcO, hf) : NULL;
                SIZE sz = {0, 0};
                GetTextExtentPoint32W(hdcO, olbl[iFound[k]].zh, (int)wcslen(olbl[iFound[k]].zh), &sz);
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
        struct PB { int id; const wchar_t *zh; } pbtn[] = {
            { IDC_EMPTY, L"空" }, { IDC_FULL, L"满" }
        };
        HDC hdcP = GetDC(hDlg);
        for (int i = 0; i < (int)(sizeof(pbtn)/sizeof(pbtn[0])); i++) {
            HWND hB = GetDlgItem(hDlg, pbtn[i].id);
            if (!hB) continue;
            RECT r; GetWindowRect(hB, &r);
            MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
            HFONT hf = (HFONT)SendMessageW(hB, WM_GETFONT, 0, 0);
            HGDIOBJ old = hf ? SelectObject(hdcP, hf) : NULL;
            SIZE sz = {0, 0};
            GetTextExtentPoint32W(hdcP, pbtn[i].zh, (int)wcslen(pbtn[i].zh), &sz);
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
