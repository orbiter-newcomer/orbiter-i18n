// =============================================================================
// LuaMfdZh.inc - Terminal MFD（LuaMFD 插件）的按钮中文标签数据（中文模式）
//
// 按钮顺序与 LuaMFD.cpp 的 ScriptMFD::ButtonLabel 一致。
// 其它 Lua 脚本定义的 MFD（Attitude RCS / AscentAP / CameraMFD 等）的按钮
// 中文映射在主程序 LuaInterpreter 的 LuaMfdZh.inc 中处理。
// =============================================================================
#ifndef LUAMFD_TERMINAL_ZH_INC
#define LUAMFD_TERMINAL_ZH_INC

static const char *g_zhLuaMfdTermLabel[] = {
	"输入", "新建", "删除", "下页", "上页"
};

#endif // LUAMFD_TERMINAL_ZH_INC
