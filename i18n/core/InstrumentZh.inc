// =============================================================================
// InstrumentZh.inc - 其他仪表数据：2D 玻璃座舱 HUD + 玻璃座舱面板
//
// 本文件只放数据/参数，不含具体绘制过程：
//   - 玻璃座舱面板：字号档位参数、按钮文本数据（PWR/SEL/MNU、导航按钮、RCS 按钮）
//   - 玻璃座舱 HUD：模式标题、读数标签
//
// 数据可由运行时布局文件覆盖（无需重新编译）：
//   i18n/layout/zh_CN/layout.ini  -> [panel.font] / [panel.buttons] / [hud]
// 默认值写在 Getter 内（与历史编译期数值一致）；英文模式不加载 layout.ini，
// 且本文件的标签数据只在中文模式路径使用。
//
// 绘制流程、字体管理、按钮标签排版等具体过程在 ZhCommon.inc：
//   Defpanel.cpp 在 #define ZH_PANEL 后包含本文件（编译 ZhCommon 的 ZH_PANEL 段）
//   hud.cpp      在 #define ZH_HUD   后包含本文件（编译 ZhCommon 的 ZH_HUD 段）
// MFD 相关数据（按钮文字映射、布局偏移）在 MfdZh.inc。
// =============================================================================
#ifndef INSTRUMENTZH_INC
#define INSTRUMENTZH_INC

#include "Layout.h"
#include <stdio.h>

// =============================================================================
// 一、玻璃座舱面板数据（Defpanel）
// =============================================================================

// 字号档位参数：字号 = clamp(面板高度 * num / den, lo, hi)
struct ZhFontSizeSpec { int num, den, lo, hi; };
inline const ZhFontSizeSpec &ZhPanelFontSpec (int i)
{
	static ZhFontSizeSpec spec[4] = {
		{ 1, 38, 18, 34 },   // 档 0：1-2 字（大号）
		{ 1, 54, 14, 26 },   // 档 1：3 字（中号）
		{ 1, 49, 14, 26 },   // 档 2：4 字（小号）
		{ 1, 65, 13, 18 },   // 档 3：状态块/小按钮（特小号，已适当加大）
	};
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		char key[32];
		for (int k = 0; k < 4; k++) {
			sprintf_s(key, 32, "spec%d.num", k); spec[k].num = orbiter::LayoutInt("panel.font", key, spec[k].num);
			sprintf_s(key, 32, "spec%d.den", k); spec[k].den = orbiter::LayoutInt("panel.font", key, spec[k].den);
			sprintf_s(key, 32, "spec%d.lo", k);  spec[k].lo  = orbiter::LayoutInt("panel.font", key, spec[k].lo);
			sprintf_s(key, 32, "spec%d.hi", k);  spec[k].hi  = orbiter::LayoutInt("panel.font", key, spec[k].hi);
		}
	}
	return spec[(i >= 0 && i < 4) ? i : 0];
}

// 底部导航按钮（HUD 开启时显示）：
// KILL ROT / HORZ LVL / PRO GRD / RETR GRD / NML+ / NML- / HOLD ALT
inline const char *ZhNavBtnLabel (int i)
{
	static const char *def[7] = { "自稳", "水平", "顺行", "逆行", "法向+", "法向-", "定高" };
	static const char *val[7] = { 0, 0, 0, 0, 0, 0, 0 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		char key[32];
		for (int k = 0; k < 7; k++) {
			sprintf_s(key, 32, "nav%d", k);
			val[k] = orbiter::LayoutString("panel.buttons", key, 0);
			if (!val[k] || !*val[k]) val[k] = def[k];
		}
	}
	return (i >= 0 && i < 7) ? val[i] : 0;
}

// 引擎状态块 RCS 模式按钮：OFF / ROT / LIN
inline const char *ZhRcsBtnLabel (int i)
{
	static const char *def[3] = { "关闭", "旋转", "平移" };
	static const char *val[3] = { 0, 0, 0 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		char key[32];
		for (int k = 0; k < 3; k++) {
			sprintf_s(key, 32, "rcs%d", k);
			val[k] = orbiter::LayoutString("panel.buttons", key, 0);
			if (!val[k] || !*val[k]) val[k] = def[k];
		}
	}
	return (i >= 0 && i < 3) ? val[i] : 0;
}

// =============================================================================
// 二、玻璃座舱 HUD 数据
// =============================================================================

// HUD 模式标题（0=轨道 1=地表 2=对接）
inline const char *ZhHudModeTitle (int i)
{
	static const char *def[3] = { "轨道", "地表", "对接" };
	static const char *val[3] = { 0, 0, 0 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		char key[32];
		for (int k = 0; k < 3; k++) {
			sprintf_s(key, 32, "mode%d", k);
			val[k] = orbiter::LayoutString("hud", key, 0);
			if (!val[k] || !*val[k]) val[k] = def[k];
		}
	}
	return (i >= 0 && i < 3) ? val[i] : 0;
}

// 读数标签（hud.dds 中的 label 图块）：label 1..5 = ALT / RAD / GS / OS / TAS
inline const char *ZhHudReadoutLabel (int i)
{
	static const char *def[6] = { 0, "高度", "半径", "地速", "轨速", "真速" };
	static const char *val[6] = { 0, 0, 0, 0, 0, 0 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		char key[32];
		for (int k = 1; k < 6; k++) {
			sprintf_s(key, 32, "readout%d", k);
			val[k] = orbiter::LayoutString("hud", key, 0);
			if (!val[k] || !*val[k]) val[k] = def[k];
		}
	}
	return (i >= 0 && i < 6) ? val[i] : 0;
}

// 数据就绪后引入 MFD 数据与汉化逻辑
// （MfdZh.inc 内含 ZhCommon.inc；ZH_PANEL / ZH_HUD 段使用以上数据）
#include "MfdZh.inc"

#endif // INSTRUMENTZH_INC
