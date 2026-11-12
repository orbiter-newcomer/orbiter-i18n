// =============================================================================
// 汉化公共逻辑（本文件是唯一放"具体过程"的汉化文件）
//
// 包含：
//   * 通用逻辑：字体缓存/释放、文本宽度估算、居中绘制、按钮标签绘制、
//     MFD 排版算法（右对齐多段文本、实测列宽三列布局）
//   * 玻璃座舱面板流程（ZH_PANEL 段）：
//       字号计算、字体创建/释放、DefaultPanel 中文按钮标签绘制流程
//   * 其他仪表流程（ZH_HUD 段）：
//       HUD 中文文本登记、绘制、对外接口
//
// 数据文件（只放数据/参数，不含具体过程）：
//   MfdZh.inc   —— MFD 数据（按钮文字映射、布局偏移）
//   InstrumentZh.inc —— 其他仪表数据（玻璃座舱面板 + HUD 的数据/参数）
//   （玻璃座舱面板与 HUD 的数据都集中在 InstrumentZh.inc）
//
// 包含方式：
//   Defpanel.cpp : #define ZH_PANEL 后 #include "InstrumentZh.inc"（→ MfdZh.inc → 本文件）
//   hud.cpp      : #define ZH_HUD   后 #include "InstrumentZh.inc"（→ MfdZh.inc → 本文件）
//   Mfd*.cpp     : #include "MfdZh.inc"（获得数据 + 本文件的排版算法）
// =============================================================================
#ifndef ZHCOMMON_INC
#define ZHCOMMON_INC

#include "Localization.h"
#include "orbitersdk.h"
#include <string>
#include <map>
#include <vector>
#include <stdio.h>
#include <string.h>

namespace ZhCommon {

	// -------------------------------------------------------------------------
	// 字体缓存（按字号高度，每个编译单元一份）
	// -------------------------------------------------------------------------
	static std::map<int, oapi::Font *> g_fonts;

	inline oapi::Font *GetFont (oapi::GraphicsClient *gc, int fh)
	{
		if (!gc || fh <= 0) return 0;
		std::map<int, oapi::Font *>::iterator it = g_fonts.find (fh);
		if (it != g_fonts.end()) return it->second;
		oapi::Font *font = gc->clbkCreateFont (-fh, true, "Sans");
		g_fonts[fh] = font;
		return font;
	}

	inline void ReleaseFonts (oapi::GraphicsClient *gc)
	{
		if (gc)
			for (std::map<int, oapi::Font *>::iterator it = g_fonts.begin(); it != g_fonts.end(); ++it)
				if (it->second) gc->clbkReleaseFont (it->second);
		g_fonts.clear();
	}

	// -------------------------------------------------------------------------
	// 文本宽度估算：CJK 全角按 1.0 字号，ASCII/符号按约 0.55 字号
	// -------------------------------------------------------------------------
	inline int EstimateWidth (const wchar_t *w, int n, int fh)
	{
		int wd = 0;
		for (int i = 0; i < n; i++)
			wd += (w[i] < 128 ? (fh*11)/20 : fh);
		return wd;
	}

	// -------------------------------------------------------------------------
	// 在已设置字体/颜色的 Sketchpad 上，以 (cx,cy) 为中心绘制单行文本
	// fh 用于垂直基线微调（不同字号的视觉中心略有差异）
	// -------------------------------------------------------------------------
	inline void DrawCentered (oapi::Sketchpad *skp, const std::wstring &w, int cx, int cy, int fh)
	{
		if (!skp || w.empty()) return;
		std::string s = orbiter::WideToUtf8String (w.c_str());
		int y = (int)(cy + fh*0.38f - 1.5f);
		skp->Text (cx, y, s.c_str(), (int)s.size());
	}

	// -------------------------------------------------------------------------
	// 按钮标签绘制流程：以按钮中心为基准（1-3 字单行、4 字两行两列）
	//   fiOverride >= 0 时强制使用指定字号档位
	//   maxW > 0 时按可用宽度自动降档（2 字最低小号档，3 字以上可降到特小号档）
	// -------------------------------------------------------------------------
	inline void DrawButtonLabel (oapi::Sketchpad *skp, oapi::GraphicsClient *gc, const char *utf8,
	                             float cx, float cy, const int *fhs,
	                             int fiOverride = -1, int maxW = 0)
	{
		std::wstring w = orbiter::Utf8ToWideString (utf8);
		int n = (int)w.size();
		if (n <= 0) return;
		int fi, rows, per;
		if (n <= 3) { fi = (n <= 2 ? 0 : 1); rows = 1; per = n; }
		else { fi = 2; per = 2; rows = (n + per - 1) / per; }
		if (fiOverride >= 0) fi = fiOverride;
		else if (maxW > 0) {
			int minFi = (n <= 2 ? 2 : 3);
			int cnt = (rows == 1 ? n : per);   // 每行最多字符数
			while (fi < minFi) {
				if (EstimateWidth (w.c_str(), cnt, fhs[fi]) <= maxW) break;
				fi++;
			}
		}
		oapi::Font *font = GetFont (gc, fhs[fi]);
		if (!font) return;
		skp->SetFont (font);
		int fh = fhs[fi];
		int lineH = fh + 1;
		for (int r = 0; r < rows; r++) {
			int start = r*per;
			int cnt = (n - start < per ? n - start : per);
			if (cnt <= 0) break;
			std::wstring line = w.substr (start, cnt);
			int lineCy = (int)(cy + (r - (rows-1)*0.5f)*lineH);
			// 所有按钮文字统一右下移 1 像素（与面板/纹理标签的视觉中心对齐）
			DrawCentered (skp, line, (int)cx + 1, lineCy + 1, fh);
		}
	}

	// -------------------------------------------------------------------------
	// MFD 排版算法：右对齐绘制若干文本段
	//   rightX 右边界；texts/colors/gaps 各 n 段；gaps 单位 cw，最后一项应为 0
	//   返回最左绘制位置
	// -------------------------------------------------------------------------
	inline int DrawRightAlignedSegments (oapi::Sketchpad *skp, int rightX, int y,
		const char *const *texts, const DWORD *colors, const int *gaps, int n, int cw)
	{
		int total = 0;
		for (int i = 0; i < n; i++)
			total += (int)skp->GetTextWidth (texts[i], (int)strlen (texts[i])) + gaps[i]*cw;
		int x = rightX - total;
		for (int i = 0; i < n; i++) {
			if (colors) skp->SetTextColor (colors[i]);
			skp->Text (x, y, texts[i], (int)strlen (texts[i]));
			x += (int)skp->GetTextWidth (texts[i], (int)strlen (texts[i])) + gaps[i]*cw;
		}
		return rightX - total;
	}

	// -------------------------------------------------------------------------
	// MFD 排版算法：三列（表头 + 两行数据）按实测列宽紧凑排列，返回下一行 y
	// 画笔（pen）由调用方在绘制前设置
	// -------------------------------------------------------------------------
	inline int DrawMeasured3Columns (oapi::Sketchpad *skp, int x, int y, int cw, int ch,
		const char *const *hcol, const char *const *row1, const char *const *row2)
	{
		int colx[3], colw[3];
		for (int i = 0; i < 3; i++) {
			colw[i] = (int)skp->GetTextWidth (hcol[i], (int)strlen (hcol[i]));
			int w = (int)skp->GetTextWidth (row1[i], (int)strlen (row1[i]));
			if (w > colw[i]) colw[i] = w;
			w = (int)skp->GetTextWidth (row2[i], (int)strlen (row2[i]));
			if (w > colw[i]) colw[i] = w;
			colw[i] += cw;
		}
		colx[0] = x; colx[1] = colx[0]+colw[0]; colx[2] = colx[1]+colw[1];
		for (int i = 0; i < 3; i++) skp->Text (colx[i], y, hcol[i], (int)strlen (hcol[i]));
		y += ch+1;
		for (int i = 0; i < 3; i++) skp->Line (colx[i], y, colx[i]+colw[i]-cw/2, y);
		y += 1;
		for (int i = 0; i < 3; i++) skp->Text (colx[i], y, row1[i], (int)strlen (row1[i]));
		y += ch;
		for (int i = 0; i < 3; i++) skp->Text (colx[i], y, row2[i], (int)strlen (row2[i]));
		return y + 2*ch;
	}

} // namespace ZhCommon

// -----------------------------------------------------------------------------
// MFD 排版算法的全局入口（官方 MFD 源码按原名调用；具体实现在上面 ZhCommon 内）
// -----------------------------------------------------------------------------
inline int DrawRightAlignedSegments (oapi::Sketchpad *skp, int rightX, int y,
	const char *const *texts, const DWORD *colors, const int *gaps, int n, int cw)
{
	return ZhCommon::DrawRightAlignedSegments (skp, rightX, y, texts, colors, gaps, n, cw);
}

inline int DrawMeasured3Columns (oapi::Sketchpad *skp, int x, int y, int cw, int ch,
	const char *const *hcol, const char *const *row1, const char *const *row2)
{
	return ZhCommon::DrawMeasured3Columns (skp, x, y, cw, ch, hcol, row1, row2);
}

// =============================================================================
// 玻璃座舱面板流程（数据来自 InstrumentZh.inc / MfdZh.inc）
// =============================================================================
#if defined(ZH_PANEL)

extern Vessel *g_focusobj;

namespace ZhPanel {

	// 由参数表计算 4 档字号（参数表 g_zhPanelFontSpec 定义在 InstrumentZh.inc）
	inline void FontSizes (int paneH, int *fh)
	{
		for (int i = 0; i < 4; i++) {
			int v = paneH * g_zhPanelFontSpec[i].num / g_zhPanelFontSpec[i].den;
			fh[i] = (v < g_zhPanelFontSpec[i].lo ? g_zhPanelFontSpec[i].lo
			       : (v > g_zhPanelFontSpec[i].hi ? g_zhPanelFontSpec[i].hi : v));
		}
	}

	// MFD 按钮标签 → 中文（内置 MFD 数据在 MfdZh.inc）
	// 中文模式下所有 MFD 按钮统一走字体渲染：
	//  - 内置 MFD：查内置别名表/通用翻译表
	//  - 外部（插件/载具自定义）MFD：由插件自己在中文模式返回中文标签，主程序直接使用；
	//    未汉化的插件返回英文原文，同样按字体渲染流程显示
	inline const char *MfdButtonText (const Instrument *instr, const char *label)
	{
		if (instr->ModeName()) {
			// 外部（Lua/插件）MFD：已由主程序汉化的脚本 MFD 按钮通过别名表映射
			// （其余 MFD 的汉化由插件自治，标签保持插件返回值）
			static const struct { const char *modeKey; const char *prefix; } zhExtMfd[] = {
				{ "Attitude", "ATT_" },
			};
			const char *mode = instr->ModeName();
			for (size_t i = 0; i < sizeof(zhExtMfd)/sizeof(zhExtMfd[0]); i++) {
				if (!strstr (mode, zhExtMfd[i].modeKey)) continue;
				char key[64];
				snprintf (key, sizeof(key), "%s%s", zhExtMfd[i].prefix, label);
				const char *txt = orbiter::Tr2 (key);
				if (txt && *txt && strcmp (txt, key) != 0) return txt;
				break;
			}
			return label;
		}
		// 内置 MFD：仅对语义冲突的个别标签改查别名键
		for (size_t i = 0; i < sizeof(g_zhBuiltinBtnAliases)/sizeof(g_zhBuiltinBtnAliases[0]); i++)
			if (g_zhBuiltinBtnAliases[i].type == instr->Type() && !strcmp (label, g_zhBuiltinBtnAliases[i].label))
				return orbiter::Tr2 (g_zhBuiltinBtnAliases[i].alias);
		return orbiter::Tr (label);
	}

} // namespace ZhPanel

void CreateZhMfdButtonFonts (oapi::GraphicsClient *gc, int paneH)
{
	if (!gc) return;
	int fh[4];
	ZhPanel::FontSizes (paneH, fh);
	for (int i = 0; i < 4; i++) ZhCommon::GetFont (gc, fh[i]);
}

void ReleaseZhMfdButtonFonts (oapi::GraphicsClient *gc)
{
	ZhCommon::ReleaseFonts (gc);
}

// 中文模式绘制面板按钮标签与面板文字
// （隐藏原字符图集/位图标签，再在相同位置用字体绘制中文）
void DefaultPanel::DrawZhMfdButtonLabels ()
{
	if (!gc || !pane) return;

	oapi::Sketchpad *skp = gc->clbkGetSketchpad (NULL);
	if (!skp) return;
	skp->SetTextAlign (oapi::Sketchpad::CENTER, oapi::Sketchpad::BASELINE);
	// 面板按钮文字颜色（0xBBGGRR 格式，高字节可携带 alpha）：默认/红/黄/蓝四种面板变体
	static const DWORD btcol[4] = { 0x77DC77, 0x7777DC, 0x77DCDC, 0xDC7777 };
	// 电源按钮固定用红色（原纹理中 PWR 标签/文字为红色），格式同上
	static const DWORD pwrcol = 0xFF8888FF;

	int fhs[4];
	ZhPanel::FontSizes (pane->H, fhs);

	for (int id = 0; id < 2; id++) {
		Instrument *instr = pane->mfd[id].instr;
		if (!instr) {
			// MFD 关闭：显示半透明电源按钮。原版为 alpha≈85 的半透明底 + 红色 "PWR" 字样，
			// 这里隐藏原按钮四边形，改用 Sketchpad 绘制半透明底、绿色边框和红色中文标签。
			GroupSpec *pgrp = mesh.GetGroup (4+id);
			if (pgrp)
				for (int j = 0; j < 4; j++) pgrp->Vtx[j].x = -10000.0f;
			const char *txt = orbiter::Tr2 (g_zhPanelBtnAlias[0]);
			if (txt && *txt) {
				float x1 = bbtnx[id][0], y1 = bbtny;
				float x2 = x1 + btnw, y2 = y1 + btnh;
				skp->QuickBrush (0x55446644);    // 半透明底：RGB(68,102,68)，alpha≈85
				skp->QuickPen   (0xFF00FF00);    // 原按钮的绿色边框
				skp->Rectangle ((int)x1, (int)y1, (int)x2, (int)y2);
				skp->SetTextColor (0x558888FF);  // 半透明红字（与原文 PWR 一致）
				ZhCommon::DrawButtonLabel (skp, gc, txt, x1 + btnw*0.5f, y1 + btnh*0.5f, fhs, -1, btnw);
			}
			continue;   // 该 MFD 关闭：没有 MFD 按钮组需要处理
		}
		GroupSpec *grp = mesh.GetGroup (id*2+1);

		// 底部按钮标签（12 个侧按钮之后，每个底部按钮 4+4 顶点）；PWR 用红色，SEL/MNU 用面板色
		if (grp) {
			for (int i = 0; i < 3; i++) {
				const char *txt = orbiter::Tr2 (g_zhPanelBtnAlias[i]);
				if (!txt || !*txt) continue;
				int vofs0 = 12*16 + i*8 + 4;
				for (int m = 0; m < 4; m++) grp->Vtx[vofs0+m].x = -10000.0f;
				skp->SetTextColor (i == 0 ? pwrcol : btcol[colidx & 3]);
				ZhCommon::DrawButtonLabel (skp, gc, txt, bbtnx[id][i] + btnw*0.5f, bbtny + btnh*0.5f, fhs, -1, btnw);
			}
		}

		// 模式选择页（主页）：强制保留官方图集渲染（两侧箭头/缩写），不参与中文统一字体渲染。
		// 官方按钮标签顶点位置/UV 由 RepaintMFDButtons() 按按钮中心计算，这里调用它强制
		// 重绘一次，保证从任意 MFD 页面切回模式选择页时箭头/缩写位置与英文版完全一致。
		if (instr->InModeSelect()) {
			RepaintMFDButtons (id);
			continue;
		}

		// 中文模式统一字体渲染：内置/插件/未翻译按钮一律隐藏图集后字体绘制
		skp->SetTextColor (btcol[colidx & 3]);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 6; j++) {
				const char *label = instr->ButtonLabel (i*6+j);
				if (!label || !*label) continue;
				const char *txt = ZhPanel::MfdButtonText (instr, label);
				if (!txt || !*txt) txt = label;
				// 隐藏该按钮的原字符图集标签（3 字符 × 4 顶点）
				if (grp) {
					int vofs0 = (i*6+j)*16 + 4;
					for (int m = 0; m < 12; m++) grp->Vtx[vofs0+m].x = -10000.0f;
				}
				ZhCommon::DrawButtonLabel (skp, gc, txt,
					btnx[id][i] + btnw*0.5f, btny[j] + btnh*0.5f, fhs, -1, btnw);
			}
		}
	}

	// 底部导航按钮：原按钮是带英文标签的纹理图，改指向通用按钮底图
	// （保留 Render 中的激活高亮逻辑），中文标签由字体绘制。
	if (pane->hud && navdispmode) {
		GroupSpec *ng = mesh.GetGroup (navgrp);
		if (ng) {
			for (int i = 0; i < 7; i++) {
				for (int j = 0; j < 4; j++)
					ng->Vtx[i*4+j].tu = (316.0f+(j%2)*44.0f)/512.0f;
				skp->SetTextColor (btcol[colidx & 3]);
				ZhCommon::DrawButtonLabel (skp, gc, g_zhNavBtnLabel[i],
					ng->Vtx[i*4].x + btnw*0.5f, ng->Vtx[i*4].y + btnh*0.5f, fhs, -1, btnw);
			}
		}
	}

	// 引擎状态块
	GroupSpec *eg = mesh.GetGroup (enggrp);
	if (eg) {
		// RCS 模式按钮（OFF/ROT/LIN）：隐藏原纹理按钮，按官方按钮尺寸自绘底图/边框/中文，
		// 保证按钮宽度与位置和英文版一致；当前激活的按钮高亮显示。
		// 注意：原按钮顶点每帧会被移到屏幕外，按钮位置必须提前缓存（否则从第二帧起读到
		// -10000 的屏幕外坐标）。状态块背景顶点 Vtx[0] 不会被修改，用它的 x 变化检测面板重建。
		static float zhRcsX[3], zhRcsY[3];
		static float zhRcsRefX = -1.0e9f, zhRcsRefY = -1.0e9f;
		// 面板几何变化检测：x 和 y 都要比较（面板尺寸变化时 y 可能先变），
		// 否则会在面板缩放后沿用旧的按钮位置（历史遗留的错位问题）。
		if (eg->Vtx[0].x != zhRcsRefX || eg->Vtx[0].y != zhRcsRefY) {
			for (int i = 0; i < 3; i++) {
				zhRcsX[i] = eg->Vtx[4+i*4].x;
				zhRcsY[i] = eg->Vtx[4+i*4].y;
			}
			zhRcsRefX = eg->Vtx[0].x;
			zhRcsRefY = eg->Vtx[0].y;
		}
		int rcs = g_focusobj ? g_focusobj->AttMode() : -1;
		for (int i = 0; i < 3; i++) {
			float x1 = zhRcsX[i], y1 = zhRcsY[i];
			float bw = 29.0f*scale, bh = 18.0f*scale;
			for (int j = 0; j < 4; j++) eg->Vtx[4+i*4+j].x = -10000.0f;  // 隐藏原按钮四边形
			bool act = (rcs == i);
			// 背景纹理里自带 RCS 按钮框：非激活按钮完全不画填充（保持原纹理框线粗细一致），
			// 激活按钮只在框内部（内缩 2px，避免盖住框线）铺一层更亮的半透明底色。
			if (act) {
				skp->QuickBrush (0x88446644);
				skp->QuickPen (0x88446644);     // 边框与底色同色（不可见）
				skp->Rectangle ((int)x1+2, (int)y1+2, (int)(x1+bw)-2, (int)(y1+bh)-2);
			}
			skp->SetTextColor (btcol[colidx & 3]);   // 文字颜色与其他面板按钮统一
			ZhCommon::DrawButtonLabel (skp, gc, g_zhRcsBtnLabel[i],
				x1 + bw*0.5f, y1 + bh*0.5f, fhs, 1);           // 中号档、单行居中
		}

		// 注：状态块背景里的英文标签（MAIN PROP / MAIN ENG / HOVR ENG / RCS / TRIM）
		// 是烘焙在面板背景纹理里的图片文字，不做替换，保留英文原样（按用户要求）。
	}

	gc->clbkReleaseSketchpad (skp);
}

#endif // ZH_PANEL

// =============================================================================
// 其他仪表流程：玻璃座舱 HUD 文字（数据来自 InstrumentZh.inc）
// =============================================================================
#if defined(ZH_HUD)

namespace ZhHud {

	struct Item {
		int x, y;         // 文本中心（屏幕像素）
		int fh;           // 字号（像素高度）
		std::string text;
	};

	static std::vector<Item> g_items;

	inline void Begin ()
	{
		g_items.clear();
	}

	inline void AddText (int x, int y, const char *utf8, int fh)
	{
		if (!utf8 || !*utf8 || fh <= 0) return;
		Item it;
		it.x = x; it.y = y; it.fh = fh; it.text = utf8;
		g_items.push_back (it);
	}

	// HUD 模式横幅："轨道 <参考天体>" / "地表 <参考天体>" / "对接 <目标>"
	inline bool AddBand (int mode, int cx, int cy, int fh, const char *refname)
	{
		if (!orbiter::IsChinese()) return false;
		std::string band = g_zhHudModeTitle[(mode >= 0 && mode < 3) ? mode : 0];
		band += "  ";
		band += orbiter::Tr (refname);
		AddText (cx, cy, band.c_str(), fh);
		return true;
	}

	inline bool AddReadoutLabel (int label, int cx, int cy, int fh)
	{
		if (!orbiter::IsChinese()) return false;
		if (label < 1 || label > 5) return false;
		AddText (cx, cy, g_zhHudReadoutLabel[label], fh);
		return true;
	}

	// 对接 HUD 的 NAV 频率行："接收机X 123.45MHz"
	inline bool AddDockNav (int cx, int cy, int fh, int navidx, double freq)
	{
		if (!orbiter::IsChinese()) return false;
		char buf[64];
		sprintf (buf, "接收机%d %0.2fMHz", navidx, freq);
		AddText (cx, cy, buf, fh);
		return true;
	}

	inline void Draw (oapi::GraphicsClient *gc, DWORD col)
	{
		if (!gc || g_items.empty()) return;
		oapi::Sketchpad *skp = gc->clbkGetSketchpad (0);
		if (!skp) { g_items.clear(); return; }
		skp->SetTextAlign (oapi::Sketchpad::CENTER, oapi::Sketchpad::BASELINE);
		skp->SetTextColor (col);
		int curH = -1;
		for (size_t i = 0; i < g_items.size(); i++) {
			const Item &it = g_items[i];
			if (it.fh != curH) {
				oapi::Font *font = ZhCommon::GetFont (gc, it.fh);
				if (font) skp->SetFont (font);
				curH = it.fh;
			}
			std::wstring w = orbiter::Utf8ToWideString (it.text.c_str());
			ZhCommon::DrawCentered (skp, w, it.x, it.y, it.fh);
		}
		gc->clbkReleaseSketchpad (skp);
		g_items.clear();
	}

} // namespace ZhHud

// 玻璃座舱 HUD 中文文本叠加绘制（Pane::Render 在 HUD 渲染之后调用）
void ZhHudDrawFrame (oapi::GraphicsClient *gc, DWORD col)
{
	if (orbiter::IsChinese()) ZhHud::Draw (gc, col);
}

// 供船模块（如 DeltaGlider）登记需要中文绘制的 HUD 文字
void ZhHudAddText (int x, int y, int size, const char *utf8)
{
	if (orbiter::IsChinese()) ZhHud::AddText (x, y, utf8, size);
}

// 释放 HUD 中文字体缓存（设备重建/销毁时由面板设备回调调用，避免继续使用失效字体）
void ZhHudReleaseFonts (oapi::GraphicsClient *gc)
{
	ZhCommon::ReleaseFonts (gc);
}

#endif // ZH_HUD

#endif // ZHCOMMON_INC
