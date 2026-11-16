// =============================================================================
// LuaMfdZh.inc - Terminal MFD（LuaMFD 插件）的按钮中文标签数据（中文模式）
//
// 按钮顺序与 LuaMFD.cpp 的 ScriptMFD::ButtonLabel 一致。
// 标签可由运行时布局文件覆盖（i18n/layout/zh_CN/layout.ini -> [mfd.lua_terminal.buttons]）。
// 其它 Lua 脚本定义的 MFD（Attitude RCS / AscentAP / CameraMFD 等）的按钮
// 中文映射在主程序 LuaInterpreter 的 LuaMfdZh.inc 中处理。
// =============================================================================
#ifndef LUAMFD_TERMINAL_ZH_INC
#define LUAMFD_TERMINAL_ZH_INC

#include "orbitersdk.h"

inline const char *ZhLuaMfdTermLabel (int i)
{
	static const char *def[5] = { "输入", "新建", "删除", "下页", "上页" };
	static const char *val[5] = { 0, 0, 0, 0, 0 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		char key[32];
		for (int k = 0; k < 5; k++) {
			sprintf_s(key, 32, "btn%d", k);
			val[k] = oapiGetLayoutString("mfd.lua_terminal.buttons", key, 0);
			if (!val[k] || !*val[k]) val[k] = def[k];
		}
	}
	return (i >= 0 && i < 5) ? val[i] : 0;
}

#endif // LUAMFD_TERMINAL_ZH_INC