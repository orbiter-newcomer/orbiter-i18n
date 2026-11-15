// =============================================================================
// 汉化扩展：通用相机 MFD（GenericCamera）中文模式布局偏置
//
// 原则与 MfdZh.inc 相同：官方坐标保持不变，中文模式只叠加偏置量。
// 本文件由 MFD.cpp 包含，官方文件中只保留 include 和偏置函数的调用点。
// 英文模式下偏置全部为 0，外观与官方原版一致。
// =============================================================================
#ifndef __GENERICCAMERA_ZH
#define __GENERICCAMERA_ZH

// 底部状态行文字的 Y 偏置：中文模式上移 4 像素，避免中文行贴住 MFD 底边
static int ZhGcamStatusYOffset ()
{
	return oapiZhActive() ? -4 : 0;
}

// 底部状态条（半透明黑条）在中文模式下向上扩展的像素数（与文字偏置同步）
static int ZhGcamStatusBarExtend ()
{
	return oapiZhActive() ? 4 : 0;
}

// 按钮中文标签（中文模式；按钮顺序与 CameraMFD::ButtonLabel 一致）
static const char *g_zhGcamBtnLabel[] = {
	"附件+", "附件-", "接口+", "接口-", "前进", "后退",
	"选船", "夜视", "放大", "缩小", "父级", "准星"
};

#endif // !__GENERICCAMERA_ZH
