// =============================================================================
// MfdZh.inc - MFD 相关的中文数据（本文件只放数据/参数，不含具体过程）
//
// 内容包括：
//   1. 各 MFD 的中文布局偏置数据（zhOff_*）：以官方坐标为基准，中文模式叠加
//   2. MFD 按钮标签文字映射数据（内置 MFD 别名表；外部 MFD 由各插件自行提供中文标签）
//
// 具体排版过程（右对齐多段文本、实测列宽三列布局等）在 ZhCommon.inc 中实现；
// 本文件只通过注释标明各偏置的用途（"此处使用右对齐/实测列宽"等）。
// 各 MFD 官方 .cpp 只保留布局公式与调用点，偏置数值统一在此。
// 英文模式（Language=en）下按钮映射返回原文、偏置不使用，行为与官方完全一致。
//
// 相关文件：
//   ZhCommon.inc —— 汉化公共逻辑（字体/文本绘制/排版算法/面板与 HUD 流程）
//   InstrumentZh.inc —— 其他仪表数据（玻璃座舱面板 + HUD）
//   （面板与 HUD 的数据都集中在 InstrumentZh.inc）
// =============================================================================
#ifndef MFDZH_INC
#define MFDZH_INC

#include "Localization.h"
#include "Mfd.h"
#include <string.h>

// -----------------------------------------------------------------------------
// 一、各 MFD 的中文布局偏置数据
// 约定：用整数单位表达分数偏移（cw/2、ch/4 等），便于维护、避免魔法小数。
// -----------------------------------------------------------------------------

// ---- 着陆 / VOR-VTOL MFD（MfdLanding.cpp）----
static const struct {
	int navRowX;            // 右上 NAV 行左移量（单位 cw；"接收机…"比"NAV…"宽）
	int cirScaleLblHalfCw;  // 瞄准圈数量级标签额外左移（单位 cw/2，正数=向左）
} zhOff_Landing = { -3, 1 };

// ---- 对接 MFD（MfdDocking.cpp）----
static const struct {
	int navRowX;            // 右上 NAV 行左移量（单位 cw）
	int colGap;             // 中文实测列间距（单位 cw，第二列起）
	int arrowHalfCw;        // 箭头位于行尾后的额外间距（单位 cw/2）
} zhOff_Dock = { -3, 2, 1 };

// ---- 轨道同步 MFD（MfdSync.cpp）----
static const struct {
	int colGap;             // 三列紧凑排列的列间距（单位 cw）
	int rightHalfCw;        // 表格右对齐时的右边距（单位 cw/2）
} zhOff_Sync = { 1, 1 };

// ---- 轨道 MFD（MfdOrbit.cpp）----
static const struct {
	int headerRightCw;      // 表头右对齐边距（单位 cw）
	int tgtRightHalfCw;     // 目标列右对齐边距（单位 cw/2）
} zhOff_Orbit = { 1, 1 };

// ---- 地图 MFD（MfdMap.cpp）----
static const struct {
	int infoRowCh;          // 信息行上移（单位 ch）
	int infoRowHalfCh;      // 额外上移（单位 ch/2）
} zhOff_Map = { 1, 1 };

// ---- MFD 菜单（Mfd.cpp）----
static const struct {
	int rowCh;              // 菜单行距基础部分（单位 ch）
	int rowQuarterCh;       // 菜单行距额外部分（单位 ch/4）
	int keyCh;              // 快捷键行下移基础部分（单位 ch）
	int keyQuarterCh;       // 快捷键行下移额外部分（单位 ch/4）
	int itemGapHalfCw;      // 两行菜单项间距（单位 cw/2）
	int rightGapHalfCw;     // 右侧菜单右对齐边距（单位 cw/2）
} zhOff_Menu = { 2, 1, 1, 1, 1, 1 };

// -----------------------------------------------------------------------------
// 二、MFD 按钮标签文字映射数据
// 查询过程见 ZhCommon.inc（ZH_PANEL 段 ZhPanel::MfdButtonText）。

// MFD 控制按钮（玻璃座舱面板底部）：电源 / 选择 / 说明
static const char *g_zhPanelBtnAlias[3] = { "PANEL_PWR", "PANEL_SEL", "PANEL_MNU" };
// -----------------------------------------------------------------------------

// 内置 MFD 的按钮标签别名表：个别按钮标签与 MFD 内容文本共用同一个英文键、但语义不同，
// 这时改查独立别名键：
//  - Map MFD 的 DN 表示"向下滚动"，而 Align MFD 内容里的 DN 是"降交点"
//  - 旧版 Map MFD 的 "<<" ">>" "ZM" 与 Comms MFD 的 "<<" ">>" 语义不同
//  - Map MFD 按钮 TRK 是跟踪开关，屏幕内容里的 TRK 是"轨迹"标签
//  - Transfer MFD 按钮 HTO 是规划开关，屏幕内容里的 HTO 是"假设轨道"标题
struct ZhMfdBtnAlias { int type; const char *label; const char *alias; };
static const ZhMfdBtnAlias g_zhBuiltinBtnAliases[] = {
	{ MFD_MAP,      "DN",  "MFD_MAP_DN" },
	{ MFD_MAP,      "<<",  "MFD_MAPOLD_LEFT" },
	{ MFD_MAP,      ">>",  "MFD_MAPOLD_RIGHT" },
	{ MFD_MAP,      "ZM",  "MFD_MAPOLD_ZOOM" },
	{ MFD_MAP,      "TRK", "MFD_MAP_TRK" },
	{ MFD_OSYNC,    "MOD", "MFD_SYNC_MOD" },
	{ MFD_TRANSFER, "HTO", "MFD_TRANS_HTO" },
};


// 具体排版算法与映射查询过程见 ZhCommon.inc
#include "ZhCommon.inc"

#endif // MFDZH_INC
