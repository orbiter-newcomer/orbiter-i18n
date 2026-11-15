// =============================================================================
// InstrumentZh.inc - 其他仪表数据：2D 玻璃座舱 HUD + 玻璃座舱面板
//
// 本文件只放数据/参数，不含具体绘制过程：
//   - 玻璃座舱面板：字号档位参数、按钮文本数据（PWR/SEL/MNU、导航按钮、RCS 按钮）
//   - 玻璃座舱 HUD：模式标题、读数标签
//
// 绘制流程、字体管理、按钮标签排版等具体过程在 ZhCommon.inc：
//   Defpanel.cpp 在 #define ZH_PANEL 后包含本文件（编译 ZhCommon 的 ZH_PANEL 段）
//   hud.cpp      在 #define ZH_HUD   后包含本文件（编译 ZhCommon 的 ZH_HUD 段）
// MFD 相关数据（按钮文字映射、布局偏移）在 MfdZh.inc。
// =============================================================================
#ifndef INSTRUMENTZH_INC
#define INSTRUMENTZH_INC

// =============================================================================
// 一、玻璃座舱面板数据（Defpanel）
// =============================================================================

// 字号档位参数：字号 = clamp(面板高度 * num / den, lo, hi)
struct ZhFontSizeSpec { int num, den, lo, hi; };
static const ZhFontSizeSpec g_zhPanelFontSpec[4] = {
	{ 1, 38, 18, 34 },   // 档 0：1-2 字（大号）
	{ 1, 54, 14, 26 },   // 档 1：3 字（中号）
	{ 1, 49, 14, 26 },   // 档 2：4 字（小号）
	{ 1, 65, 13, 18 },   // 档 3：状态块/小按钮（特小号，已适当加大）
};

// 底部导航按钮（HUD 开启时显示）：
// KILL ROT / HORZ LVL / PRO GRD / RETR GRD / NML+ / NML- / HOLD ALT
static const char *g_zhNavBtnLabel[7] = { "自稳", "水平", "顺行", "逆行", "法向+", "法向-", "定高" };

// 引擎状态块 RCS 模式按钮：OFF / ROT / LIN
static const char *g_zhRcsBtnLabel[3] = { "关闭", "旋转", "平移" };

// =============================================================================
// 二、玻璃座舱 HUD 数据
// =============================================================================

// HUD 模式标题（0=轨道 1=地表 2=对接）
static const char *g_zhHudModeTitle[3] = { "轨道", "地表", "对接" };

// 读数标签（hud.dds 中的 label 图块）：label 1..5 = ALT / RAD / GS / OS / TAS
static const char *g_zhHudReadoutLabel[6] = { 0, "高度", "半径", "地速", "轨速", "真速" };

// 数据就绪后引入 MFD 数据与汉化逻辑
// （MfdZh.inc 内含 ZhCommon.inc；ZH_PANEL / ZH_HUD 段使用以上数据）
#include "MfdZh.inc"

#endif // INSTRUMENTZH_INC
