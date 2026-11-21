// ==============================================================
// DlgLayoutApi.inc - dialog layout helper API implementations
// (project extension).
//
// Included by Src/Orbiter/OrbiterAPI.cpp at the original definition site
// so that the official API file only keeps the include hook.
// ==============================================================
DLLEXPORT int oapiGetLayoutInt (const char *section, const char *key, int def)
{
	return orbiter::LayoutInt(section, key, def);
}

DLLEXPORT double oapiGetLayoutFloat (const char *section, const char *key, double def)
{
	return orbiter::LayoutFloat(section, key, def);
}

DLLEXPORT const char *oapiGetLayoutString (const char *section, const char *key, const char *def)
{
	return orbiter::LayoutString(section, key, def);
}

// ---------------------------------------------------------------------------
// Chinese dialog layout helpers ("official layout + offset")
//
// The RC resources keep the official English layout. In Chinese mode the text
// is translated and controls that cannot hold the wider Chinese text are
// enlarged / repacked. English mode never touches the layout.
// ---------------------------------------------------------------------------

// Translate the text of a dialog control in place. Returns true if the text is
// now Chinese (i.e. a translation existed).
static bool ZhTranslateDlgItem (HWND hCtrl)
{
	if (!hCtrl) return false;
	wchar_t wtxt[512];
	if (GetWindowTextW(hCtrl, wtxt, 512) <= 0) return false;
	std::string utf8 = orbiter::WideToUtf8String(wtxt);
	const char *zh = orbiter::Tr(utf8.c_str());
	if (!zh || strcmp(zh, utf8.c_str()) == 0) return false;
	std::wstring wz = orbiter::Utf8ToWideString(zh);
	SetWindowTextW(hCtrl, wz.c_str());
	return true;
}

// Measure the current text of a control in dialog font pixels. When hOut is not
// null it receives the text height as well.
static int ZhMeasureDlgItem (HWND hDlg, HWND hCtrl, int *hOut = 0)
{
	wchar_t wtxt[512];
	if (GetWindowTextW(hCtrl, wtxt, 512) <= 0) return 0;
	HDC hdc = GetDC(hDlg);
	HFONT hf = (HFONT)SendMessageW(hCtrl, WM_GETFONT, 0, 0);
	HGDIOBJ old = hf ? SelectObject(hdc, hf) : NULL;
	SIZE sz = { 0, 0 };
	GetTextExtentPoint32W(hdc, wtxt, (int)wcslen(wtxt), &sz);
	if (old) SelectObject(hdc, old);
	ReleaseDC(hDlg, hdc);
	if (hOut) *hOut = sz.cy;
	return sz.cx;
}

DLLEXPORT int oapiZhFitControlText (HWND hDlg, int ctrlId, int padX)
{
	if (!hDlg || !orbiter::IsChinese()) return 0;
	HWND hCtrl = GetDlgItem(hDlg, ctrlId);
	if (!hCtrl) return 0;
	ZhTranslateDlgItem(hCtrl);
	int textW = ZhMeasureDlgItem(hDlg, hCtrl);
	if (textW <= 0) return 0;
	RECT r;
	GetWindowRect(hCtrl, &r);
	MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
	int w = r.right - r.left, h = r.bottom - r.top;
	int nw = textW + (padX > 0 ? padX : 0);
	if (nw <= w) return 0;
	SetWindowPos(hCtrl, NULL, r.left, r.top, nw, h, SWP_NOZORDER);
	return 1;
}

DLLEXPORT int oapiZhFitControlHeight (HWND hDlg, int ctrlId, int padY)
{
	if (!hDlg || !orbiter::IsChinese()) return 0;
	HWND hCtrl = GetDlgItem(hDlg, ctrlId);
	if (!hCtrl) return 0;
	ZhTranslateDlgItem(hCtrl);
	int textH = 0;
	if (ZhMeasureDlgItem(hDlg, hCtrl, &textH) <= 0) return 0;
	RECT r;
	GetWindowRect(hCtrl, &r);
	MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
	int w = r.right - r.left, h = r.bottom - r.top;
	int nh = textH + (padY > 0 ? padY : 0);
	if (nh <= h) return 0;
	SetWindowPos(hCtrl, NULL, r.left, r.top, w, nh, SWP_NOZORDER);
	return 1;
}

DLLEXPORT int oapiZhOffsetControls (HWND hDlg, const int *ctrlIds, int count, int dx, int dy)
{
	if (!hDlg || !ctrlIds || count <= 0 || !orbiter::IsChinese()) return 0;
	if (dx == 0 && dy == 0) return 0;
	int moved = 0;
	for (int i = 0; i < count; i++) {
		HWND hCtrl = GetDlgItem(hDlg, ctrlIds[i]);
		if (!hCtrl) continue;
		RECT r;
		GetWindowRect(hCtrl, &r);
		MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
		SetWindowPos(hCtrl, NULL, r.left + dx, r.top + dy, r.right - r.left,
			r.bottom - r.top, SWP_NOZORDER);
		moved++;
	}
	return moved;
}

DLLEXPORT int oapiZhOffsetWindow (HWND hCtrl, int dx, int dy)
{
	if (!hCtrl || !orbiter::IsChinese()) return 0;
	if (dx == 0 && dy == 0) return 0;
	RECT r;
	GetWindowRect(hCtrl, &r);
	HWND hParent = GetParent(hCtrl);
	if (hParent) MapWindowPoints(NULL, hParent, (POINT*)&r, 2);
	SetWindowPos(hCtrl, NULL, r.left + dx, r.top + dy, r.right - r.left,
		r.bottom - r.top, SWP_NOZORDER);
	return 1;
}

// Lay out all push buttons of a dialog for the Chinese UI (see OrbiterAPI.h):
// translate, grow vertically if needed, then move the whole row down by yOffset.
struct ZhLayoutDlgCtx {
	HWND hDlg;
	int padY;
	int yOffset;
	int adjusted;
};

static BOOL CALLBACK ZhLayoutDlgBtnCb (HWND hCtrl, LPARAM lp)
{
	ZhLayoutDlgCtx *ctx = (ZhLayoutDlgCtx*)lp;
	LONG style = GetWindowLongA(hCtrl, GWL_STYLE);
	LONG type = style & 0xF;
	if (type != BS_PUSHBUTTON && type != BS_DEFPUSHBUTTON) return TRUE;
	ZhTranslateDlgItem(hCtrl);
	int textH = 0;
	if (ZhMeasureDlgItem(ctx->hDlg, hCtrl, &textH) <= 0) return TRUE;
	RECT r;
	GetWindowRect(hCtrl, &r);
	MapWindowPoints(NULL, ctx->hDlg, (POINT*)&r, 2);
	int w = r.right - r.left, h = r.bottom - r.top;
	int nh = textH + (ctx->padY > 0 ? ctx->padY : 0);
	if (nh < h) nh = h;
	SetWindowPos(hCtrl, NULL, r.left, r.top + ctx->yOffset, w, nh, SWP_NOZORDER);
	ctx->adjusted++;
	return TRUE;
}

DLLEXPORT int oapiZhLayoutDialogButtons (HWND hDlg, int padY, int yOffset)
{
	if (!hDlg || !orbiter::IsChinese()) return 0;
	ZhLayoutDlgCtx ctx;
	ctx.hDlg = hDlg;
	ctx.padY = padY > 0 ? padY : 0;
	ctx.yOffset = yOffset;
	ctx.adjusted = 0;
	EnumChildWindows(hDlg, ZhLayoutDlgBtnCb, (LPARAM)&ctx);
	return ctx.adjusted;
}

DLLEXPORT int oapiZhFitControlRow (HWND hDlg, const int *ctrlIds, int count, int padX, int gap, int startX)
{
	if (!hDlg || !ctrlIds || count <= 0 || !orbiter::IsChinese()) return 0;
	if (gap < 0) gap = 0;
	if (padX < 0) padX = 0;

	std::vector<HWND> ctrl((size_t)count, (HWND)0);
	std::vector<int>  width((size_t)count, 0);
	RECT r0;
	bool haveFirst = false;
	int adjusted = 0;

	for (int i = 0; i < count; i++) {
		ctrl[(size_t)i] = GetDlgItem(hDlg, ctrlIds[i]);
		if (!ctrl[(size_t)i]) return adjusted;
		ZhTranslateDlgItem(ctrl[(size_t)i]);
		width[(size_t)i] = ZhMeasureDlgItem(hDlg, ctrl[(size_t)i]) + padX;
		if (!haveFirst) {
			GetWindowRect(ctrl[0], &r0);
			MapWindowPoints(NULL, hDlg, (POINT*)&r0, 2);
			haveFirst = true;
		}
	}

	int x = (startX >= 0) ? startX : r0.left;
	int y = r0.top, h = r0.bottom - r0.top;
	for (int i = 0; i < count; i++) {
		RECT r;
		GetWindowRect(ctrl[(size_t)i], &r);
		MapWindowPoints(NULL, hDlg, (POINT*)&r, 2);
		int w = r.right - r.left;
		int nw = width[(size_t)i];
		if (nw < w) nw = w;   // never shrink below the official size
		int nh = (r.bottom - r.top);
		// keep the vertical centre of the original control
		int ny = r.top + (nh - h) / 2;
		SetWindowPos(ctrl[(size_t)i], NULL, x, ny, nw, h, SWP_NOZORDER);
		x += nw + gap;
		adjusted++;
	}
	return adjusted;
}

// ---------------------------------------------------------------------------
// oapiZhFitDialogButtons: fit all push buttons of a dialog to their Chinese
// texts. The official layout is the base: every button is translated and grown
// (to the right / downwards) when the Chinese text does not fit. Buttons on the
// same row are repacked left-to-right only when widening them would make them
// overlap (e.g. a row of long Chinese button labels); buttons in different
// columns are not moved. Targeted moves use oapiZhOffsetControls.
// ---------------------------------------------------------------------------

struct ZhFitDlgBtn {
	HWND h;
	RECT r;      // position relative to the dialog
	int tw, th;  // measured Chinese text size in pixels
};

struct ZhFitDlgCtx {
	HWND hDlg;
	int padX, padY;
	std::vector<ZhFitDlgBtn> btns;
};

static BOOL CALLBACK ZhCollectFitBtnCb (HWND hCtrl, LPARAM lp)
{
	ZhFitDlgCtx *ctx = (ZhFitDlgCtx*)lp;
	LONG style = GetWindowLongA(hCtrl, GWL_STYLE);
	LONG type = style & 0xF;
	if (type != BS_PUSHBUTTON && type != BS_DEFPUSHBUTTON) return TRUE;
	ZhTranslateDlgItem(hCtrl);
	ZhFitDlgBtn b;
	b.h = hCtrl;
	GetWindowRect(hCtrl, &b.r);
	MapWindowPoints(NULL, ctx->hDlg, (POINT*)&b.r, 2);
	b.tw = ZhMeasureDlgItem(ctx->hDlg, hCtrl, &b.th);
	ctx->btns.push_back(b);
	return TRUE;
}

DLLEXPORT int oapiZhFitDialogButtons (HWND hDlg, int padX, int padY)
{
	if (!hDlg || !orbiter::IsChinese()) return 0;
	ZhFitDlgCtx ctx;
	ctx.hDlg = hDlg;
	ctx.padX = padX > 0 ? padX : 0;
	ctx.padY = padY > 0 ? padY : 0;
	EnumChildWindows(hDlg, ZhCollectFitBtnCb, (LPARAM)&ctx);

	const size_t n = ctx.btns.size();
	std::vector<bool> used(n, false);
	int adjusted = 0;
	const int rowGap = 4;

	for (size_t i = 0; i < n; i++) {
		if (used[i]) continue;

		// buttons on the same row (same top edge) are handled together
		std::vector<size_t> row;
		row.push_back(i);
		used[i] = true;
		for (size_t j = i + 1; j < n; j++) {
			if (used[j]) continue;
			int dt = ctx.btns[j].r.top - ctx.btns[i].r.top;
			if (dt >= -1 && dt <= 1) {
				row.push_back(j);
				used[j] = true;
			}
		}
		// order the row by horizontal position
		for (size_t a = 1; a < row.size(); a++) {
			size_t v = row[a], b = a;
			while (b > 0 && ctx.btns[row[b - 1]].r.left > ctx.btns[v].r.left) {
				row[b] = row[b - 1];
				b--;
			}
			row[b] = v;
		}

		// target size of every button in the row (only grow)
		int rowH = 0;
		for (size_t k = 0; k < row.size(); k++) {
			ZhFitDlgBtn &b = ctx.btns[row[k]];
			int w = b.r.right - b.r.left, h = b.r.bottom - b.r.top;
			int nw = w, nh = h;
			if (b.tw + ctx.padX > w) nw = b.tw + ctx.padX;
			if (b.th + ctx.padY > h) nh = b.th + ctx.padY;
			b.tw = nw;   // reuse: target width
			b.th = nh;   // reuse: target height
			if (nh > rowH) rowH = nh;
		}

		// check whether the wider buttons would overlap in the official layout
		bool overlap = false;
		int acc = ctx.btns[row[0]].r.left;
		for (size_t k = 0; k < row.size(); k++) {
			int x = ctx.btns[row[k]].r.left;
			if (k > 0 && x < acc) { overlap = true; break; }
			acc = x + ctx.btns[row[k]].tw;
		}

		if (overlap) {
			// repack the row left-to-right from the official left edge so that
			// the wider Chinese labels fit without overlapping each other
			int x = ctx.btns[row[0]].r.left;
			int y = ctx.btns[row[0]].r.top;
			for (size_t k = 0; k < row.size(); k++) {
				ZhFitDlgBtn &b = ctx.btns[row[k]];
				SetWindowPos(b.h, NULL, x, y, b.tw, rowH, SWP_NOZORDER);
				x += b.tw + rowGap;
				adjusted++;
			}
		} else {
			for (size_t k = 0; k < row.size(); k++) {
				ZhFitDlgBtn &b = ctx.btns[row[k]];
				int w = b.r.right - b.r.left, h = b.r.bottom - b.r.top;
				if (b.tw == w && b.th == h) continue;
				SetWindowPos(b.h, NULL, b.r.left, b.r.top, b.tw, b.th, SWP_NOZORDER);
				adjusted++;
			}
		}
	}
	return adjusted;
}
