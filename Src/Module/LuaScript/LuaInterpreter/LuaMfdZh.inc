// =============================================================================
// LuaMfdZh.inc - Lua 脚本定义 MFD 的按钮中文标签映射（中文模式）
//
// 覆盖通过 Lua 脚本定义、由主程序 VesselMFD 渲染按钮标签的 MFD：
//   Config/MFD/Attitude.cfg     （Attitude RCS）
//   APMFD.lua                   （AscentAP，Lua 版）
//   CameraMFD.lua               （相机 MFD，Lua 版）
// 未列入的标签（如测试 MFD）返回原文；英文模式不映射。
// =============================================================================
#ifndef LUA_MFD_ZH_INC
#define LUA_MFD_ZH_INC

static const struct { const char *en; const char *zh; } g_zhLuaMfdBtnMap[] = {
	// Attitude RCS（Config/MFD/Attitude.cfg）
	{ "SET", "设置" }, { "DCK", "对接" }, { "SUS", "挂起" }, { "END", "取消" },
	{ "CFG", "配置" }, { "+R",  "加旋转" }, { "-R",  "删旋转" }, { "AX",  "旋转轴" },
	{ "+V",  "加角度" }, { "-V",  "减角度" }, { "UP",  "上移" }, { "DN",  "下移" },
	{ "GO",  "启动" }, { "RTN", "返回" }, { "BAS", "基准" }, { "INV", "反向" },
	{ "CUR", "保持" }, { "NAV", "接收机" }, { "ACT", "激活" }, { "CNC", "取消" },
	// AscentAP（APMFD.lua）
	{ "L",   "发射" }, { "EA",  "接通" }, { "DA",  "断开" },
	{ "PG-", "上页" }, { "PG+", "下页" },
	{ "AZ-", "方位-" }, { "AZ+", "方位+" }, { "AL-", "高度-" }, { "AL+", "高度+" },
	{ "OM2", "圆化" },
	// CameraMFD.lua（与 C++ Generic Camera 相同标签）
	{ "NA",  "附件+" }, { "PA",  "附件-" }, { "ND",  "接口+" }, { "PD",  "接口-" },
	{ "FWD", "前进" }, { "BWD", "后退" }, { "VES", "选船" }, { "NV",  "夜视" },
	{ "ZM+", "放大" }, { "ZM-", "缩小" }, { "PAR", "父级" }, { "CRS", "准星" },
};

// 中文模式下把 Lua 脚本返回的英文按钮标签映射为中文；未命中返回原文
static const char *ZhLuaMfdLabel (const char *en)
{
	if (!en || !oapiZhActive()) return en;
	for (size_t i = 0; i < sizeof(g_zhLuaMfdBtnMap)/sizeof(g_zhLuaMfdBtnMap[0]); i++)
		if (!strcmp (g_zhLuaMfdBtnMap[i].en, en)) return g_zhLuaMfdBtnMap[i].zh;
	return en;
}

#endif // LUA_MFD_ZH_INC
