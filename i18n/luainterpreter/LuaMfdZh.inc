// =============================================================================
// LuaMfdZh.inc - Lua 脚本定义 MFD 的按钮标签翻译（中文模式）
//
// 覆盖通过 Lua 脚本定义、由主程序 VesselMFD 渲染按钮标签的 MFD：
//   Config/MFD/Attitude.cfg     （Attitude RCS）
//   APMFD.lua                   （AscentAP，Lua 版）
//   CameraMFD.lua               （相机 MFD，Lua 版）
//
// 翻译数据在 scripts.po（msgctxt = "mfd.button"，英文标签为 msgid），
// 不再编译进程序；未列入的标签返回原文，英文模式返回原文。
// =============================================================================
#ifndef LUA_MFD_ZH_INC
#define LUA_MFD_ZH_INC

#include "orbitersdk.h"

// 中文模式下把 Lua 脚本返回的英文按钮标签翻译为当前语言；未命中返回原文
static const char *ZhLuaMfdLabel (const char *en)
{
	if (!en || !oapiZhActive()) return en;
	return oapiTrContext ("mfd.button", en);
}

#endif // LUA_MFD_ZH_INC