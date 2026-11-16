// =============================================================================
// TransXZh.inc - TransX MFD 的按钮中文标签数据（中文模式）
//
// 按钮顺序与 transx.cpp 的 TransxMFD::ButtonLabel 一致。
// 标签可由运行时布局文件覆盖（i18n/layout/zh_CN/layout.ini -> [mfd.transx.buttons]）；
// 英文模式不使用本数据（保持原英文标签与图集渲染）。
// =============================================================================
#ifndef TRANSXZH_INC
#define TRANSXZH_INC

#include "orbitersdk.h"

inline const char *ZhTransXBtnLabel (int i)
{
	static const char *def[12] = {
		"帮助", "前进", "后退", "视图", "下项", "上项",
		"调整+", "调整-", "增加", "减少", "执行", "输入"
	};
	static const char *val[12] = { 0 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		char key[32];
		for (int k = 0; k < 12; k++) {
			sprintf_s(key, 32, "btn%d", k);
			val[k] = oapiGetLayoutString("mfd.transx.buttons", key, 0);
			if (!val[k] || !*val[k]) val[k] = def[k];
		}
	}
	return (i >= 0 && i < 12) ? val[i] : 0;
}

#endif // TRANSXZH_INC