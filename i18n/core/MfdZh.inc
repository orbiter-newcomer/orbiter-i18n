// =============================================================================
// MfdZh.inc - MFD 相关的中文数据（数据/参数，不含具体排版过程）
//
// 内容包括：
//   1. 各 MFD 的中文布局偏置（ZhOff*）：以官方坐标为基准，中文模式叠加。
//      数值默认值在代码中，可由运行时数据文件覆盖而无需重新编译：
//        i18n/layout/zh_CN/layout.ini  -> [mfd.landing] / [mfd.docking] / ...
//   2. MFD 按钮标签文字映射数据（内置 MFD 别名表；外部 MFD 由各插件自行提供）
//
// 具体排版过程（右对齐多段文本、实测列宽三列布局等）在 ZhCommon.inc 中实现。
// 英文模式（Language=en）下按钮映射返回原文、偏置不使用（layout.ini 不加载）。
//
// 相关文件：
//   ZhCommon.inc         —— 汉化公共逻辑（字体/文本绘制/排版算法/面板与 HUD 流程）
//   InstrumentZh.inc     —— 其他仪表数据（玻璃座舱面板 + HUD）
//   i18n/layout/zh_CN/layout.ini —— 运行时可调布局数据
// =============================================================================
#ifndef MFDZH_INC
#define MFDZH_INC

#include "Localization.h"
#include "Layout.h"
#include "Mfd.h"
#include <string.h>

// -----------------------------------------------------------------------------
// 一、各 MFD 的中文布局偏置（惰性读取 layout.ini，缺失时使用编译默认值）
//
// 约定：用整数单位表达分数偏移（cw/2、ch/4 等），便于维护、避免魔法小数。
// Getter 返回每个编译单元内的单例；首次访问时从 layout 数据读取覆盖。
// -----------------------------------------------------------------------------

// ---- 着陆 / VOR-VTOL MFD（MfdLanding.cpp）----
struct ZhOffLandingT {
	int navRowX;            // 右上 NAV 行左移量（单位 cw；"接收机…"比"NAV…"宽）
	int cirScaleLblHalfCw;  // 瞄准圈数量级标签额外左移（单位 cw/2，正数=向左）
};
inline ZhOffLandingT &ZhOffLanding ()
{
	static ZhOffLandingT v = { -3, 1 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		v.navRowX           = orbiter::LayoutInt("mfd.landing", "navRowX", v.navRowX);
		v.cirScaleLblHalfCw = orbiter::LayoutInt("mfd.landing", "cirScaleLblHalfCw", v.cirScaleLblHalfCw);
	}
	return v;
}

// ---- 对接 MFD（MfdDocking.cpp）----
struct ZhOffDockT {
	int navRowX;            // 右上 NAV 行左移量（单位 cw）
	int colGap;             // 中文实测列间距（单位 cw，第二列起）
	int arrowHalfCw;        // 箭头位于行尾后的额外间距（单位 cw/2）
};
inline ZhOffDockT &ZhOffDock ()
{
	static ZhOffDockT v = { -3, 2, 1 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		v.navRowX     = orbiter::LayoutInt("mfd.docking", "navRowX", v.navRowX);
		v.colGap      = orbiter::LayoutInt("mfd.docking", "colGap", v.colGap);
		v.arrowHalfCw = orbiter::LayoutInt("mfd.docking", "arrowHalfCw", v.arrowHalfCw);
	}
	return v;
}

// ---- 轨道同步 MFD（MfdSync.cpp）----
struct ZhOffSyncT {
	int colGap;             // 三列紧凑排列的列间距（单位 cw）
	int rightHalfCw;        // 表格右对齐时的右边距（单位 cw/2）
};
inline ZhOffSyncT &ZhOffSync ()
{
	static ZhOffSyncT v = { 1, 1 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		v.colGap      = orbiter::LayoutInt("mfd.sync", "colGap", v.colGap);
		v.rightHalfCw = orbiter::LayoutInt("mfd.sync", "rightHalfCw", v.rightHalfCw);
	}
	return v;
}

// ---- 轨道 MFD（MfdOrbit.cpp）----
struct ZhOffOrbitT {
	int headerRightCw;      // 表头右对齐边距（单位 cw）
	int tgtRightHalfCw;     // 目标列右对齐边距（单位 cw/2）
};
inline ZhOffOrbitT &ZhOffOrbit ()
{
	static ZhOffOrbitT v = { 1, 1 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		v.headerRightCw  = orbiter::LayoutInt("mfd.orbit", "headerRightCw", v.headerRightCw);
		v.tgtRightHalfCw = orbiter::LayoutInt("mfd.orbit", "tgtRightHalfCw", v.tgtRightHalfCw);
	}
	return v;
}

// ---- 地图 MFD（MfdMap.cpp）----
struct ZhOffMapT {
	int infoRowCh;          // 信息行上移（单位 ch）
	int infoRowHalfCh;      // 额外上移（单位 ch/2）
};
inline ZhOffMapT &ZhOffMap ()
{
	static ZhOffMapT v = { 1, 1 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		v.infoRowCh     = orbiter::LayoutInt("mfd.map", "infoRowCh", v.infoRowCh);
		v.infoRowHalfCh = orbiter::LayoutInt("mfd.map", "infoRowHalfCh", v.infoRowHalfCh);
	}
	return v;
}

// ---- MFD 菜单（Mfd.cpp）----
struct ZhOffMenuT {
	int rowCh;              // 菜单行距基础部分（单位 ch）
	int rowQuarterCh;       // 菜单行距额外部分（单位 ch/4）
	int keyCh;              // 快捷键行下移基础部分（单位 ch）
	int keyQuarterCh;       // 快捷键行下移额外部分（单位 ch/4）
	int itemGapHalfCw;      // 两行菜单项间距（单位 cw/2）
	int rightGapHalfCw;     // 右侧菜单右对齐边距（单位 cw/2）
};
inline ZhOffMenuT &ZhOffMenu ()
{
	static ZhOffMenuT v = { 2, 1, 1, 1, 1, 1 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		v.rowCh          = orbiter::LayoutInt("mfd.menu", "rowCh", v.rowCh);
		v.rowQuarterCh   = orbiter::LayoutInt("mfd.menu", "rowQuarterCh", v.rowQuarterCh);
		v.keyCh          = orbiter::LayoutInt("mfd.menu", "keyCh", v.keyCh);
		v.keyQuarterCh   = orbiter::LayoutInt("mfd.menu", "keyQuarterCh", v.keyQuarterCh);
		v.itemGapHalfCw  = orbiter::LayoutInt("mfd.menu", "itemGapHalfCw", v.itemGapHalfCw);
		v.rightGapHalfCw = orbiter::LayoutInt("mfd.menu", "rightGapHalfCw", v.rightGapHalfCw);
	}
	return v;
}

// -----------------------------------------------------------------------------
// 二、MFD 按钮标签文字映射数据
// 查询过程见 ZhCommon.inc（ZH_PANEL 段 ZhPanel::MfdButtonText）。
// -----------------------------------------------------------------------------

// MFD 控制按钮（玻璃座舱面板底部）：电源 / 选择 / 说明
static const char *g_zhPanelBtnAlias[3] = { "PANEL_PWR", "PANEL_SEL", "PANEL_MNU" };

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
