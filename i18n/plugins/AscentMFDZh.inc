// =============================================================================
// AscentMFDZh.inc - Ascent profile MFD 的按钮中文标签数据（中文模式）
//
// 按钮顺序与 AscentMFD.cpp 的 AscentMFD::ButtonLabel 一致。
// 标签可由运行时布局文件覆盖（i18n/layout/zh_CN/layout.ini -> [mfd.ascent.buttons]）；
// 英文模式不使用本数据（保持原英文标签与图集渲染）。
// =============================================================================
#ifndef ASCENTMFDZH_INC
#define ASCENTMFDZH_INC

#include "orbitersdk.h"

inline const char *ZhAscentMfdBtnLabel (int i)
{
	static const char *def[4] = { "翻页", "高度范围", "径向速度", "切向速度" };
	static const char *val[4] = { 0, 0, 0, 0 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		char key[32];
		for (int k = 0; k < 4; k++) {
			sprintf_s(key, 32, "btn%d", k);
			val[k] = oapiGetLayoutString("mfd.ascent.buttons", key, 0);
			if (!val[k] || !*val[k]) val[k] = def[k];
		}
	}
	return (i >= 0 && i < 4) ? val[i] : 0;
}

#endif // ASCENTMFDZH_INC