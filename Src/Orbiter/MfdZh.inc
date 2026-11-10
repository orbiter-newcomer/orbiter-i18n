// MfdZh.inc - MFD 中文模式固定布局偏置数据（官方坐标为基准，仅中文模式生效）
//
// 汉化时为避免中文文本重叠/超出边框，曾在各 MFD 源代码里硬编码固定偏移。
// 本文件把这些偏移集中管理：官方 .cpp 只保留布局公式与调用点，偏置数值统一在此。
// 约定：用整数单位表达分数偏移（cw/2、ch/4 等），便于维护、避免魔法小数。
// 英文模式（Language=en）不使用这些数据，行为与官方完全一致。

#ifndef MFDZH_INC
#define MFDZH_INC

// ---- 着陆 / VOR-VTOL MFD（MfdLanding.cpp）----
static const struct {
	int navRowX;            // 右上 NAV 行左移量（单位 cw；“接收机…”比“NAV…”宽）
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

#endif // MFDZH_INC
