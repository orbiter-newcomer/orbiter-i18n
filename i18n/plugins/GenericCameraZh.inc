// =============================================================================
// GenericCameraZh.inc - 通用相机 MFD（GenericCamera）中文模式布局数据
//
// 原则与 MfdZh.inc 相同：官方坐标保持不变，中文模式只叠加偏置量。
// 数值可由运行时布局文件覆盖（i18n/layout/zh_CN/layout.ini ->
// [mfd.genericcamera] / [mfd.genericcamera.buttons]）；英文模式偏置全部为 0，
// 外观与官方原版一致。
// =============================================================================
#ifndef __GENERICCAMERA_ZH
#define __GENERICCAMERA_ZH

#include "orbitersdk.h"

// 底部状态行文字的 Y 偏置：中文模式上移（默认 4 像素），避免中文行贴住 MFD 底边
static int ZhGcamStatusYOffset ()
{
	if (!oapiZhActive()) return 0;
	return oapiGetLayoutInt("mfd.genericcamera", "statusYOffset", -4);
}

// 底部状态条（半透明黑条）在中文模式下向上扩展的像素数（默认 4）
static int ZhGcamStatusBarExtend ()
{
	if (!oapiZhActive()) return 0;
	return oapiGetLayoutInt("mfd.genericcamera", "statusBarExtend", 4);
}

// 按钮中文标签（中文模式；按钮顺序与 CameraMFD::ButtonLabel 一致）
inline const char *ZhGcamBtnLabel (int i)
{
	static const char *def[12] = {
		"附件+", "附件-", "接口+", "接口-", "前进", "后退",
		"选船", "夜视", "放大", "缩小", "父级", "准星"
	};
	static const char *val[12] = { 0 };
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		char key[32];
		for (int k = 0; k < 12; k++) {
			sprintf_s(key, 32, "btn%d", k);
			val[k] = oapiGetLayoutString("mfd.genericcamera.buttons", key, 0);
			if (!val[k] || !*val[k]) val[k] = def[k];
		}
	}
	return (i >= 0 && i < 12) ? val[i] : 0;
}

#endif // !__GENERICCAMERA_ZH