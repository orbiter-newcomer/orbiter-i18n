// =============================================================================
// TrackIRZh.inc - TrackIR 插件界面中文文本（数据 + 本地化逻辑）
//
// 汉化扩展：英文资源仍在 TrackIR.rc 中（英文模式 = 原版）；中文模式下由本文件
// 的 TrackIRZhLocalizeWindow() 在对话框创建时把已知英文文本替换为中文。
// 数据与逻辑分离：文本数据集中在本文件，配置对话框代码（TrackIRconfig.cpp）
// 只保留 ASCII 调用，避免插件源码混入中文。
// =============================================================================
#ifndef TRACKIRZH_INC
#define TRACKIRZH_INC

#include <windows.h>
#include "orbitersdk.h"

struct TrackIRZhEntry { const wchar_t *en; const wchar_t *zh; };

static const TrackIRZhEntry g_trackIRZhEntries[] = {
	{ L"Orbiter: TrackIR Configuration", L"Orbiter：TrackIR 配置" },
	{ L"OK", L"确定" },
	{ L"Cancel", L"取消" },
	{ L"TrackIR Module", L"TrackIR 模块" },
	{ L"TrackIR Module Version", L"TrackIR 模块版本" },
	{ L"Not found", L"未找到" },
	{ L"Not active", L"未激活" },
	{ L"Activate TrackIR", L"启用 TrackIR" },
	{ L"Cockpit views", L"座舱视角" },
	{ L"Virtual cockpit view", L"虚拟座舱视角" },
	{ L"2-D instrument panel view", L"2D 仪表面板视角" },
	{ L"Generic glass-cockpit view", L"通用玻璃座舱视角" },
	{ L"External views", L"外部视角" },
	{ L"Track view", L"追踪视角" },
	{ L"Ground view", L"地面视角" },
	{ L"VC mouse interaction", L"座舱鼠标交互" },
	{ L"Tracking mode", L"追踪模式" },
	{ L"Track rotation", L"追踪旋转" },
	{ L"Track position", L"追踪位置" },
	{ L"Freeze camera after mouse move for", L"鼠标移动后冻结相机" },
	{ L"Default range: ?", L"默认范围：?" },
	{ L"seconds", L"秒" },
	{ L"deg.", L"度" },
	{ L"m", L"米" },
	{ L"Track zoom", L"追踪缩放" },
	{ L"by head orientation", L"按头部朝向" },
	{ L"by head position", L"按头部位置" },
	{ L"Parameters", L"参数" },
	{ L"Dead zone (0-100) :", L"死区（0-100）：" },
	{ L"Speed (1-100) :", L"速度（1-100）：" },
};

// 中文模式（主程序 Localization 设置）
inline bool TrackIRZhActive () { return oapiZhActive() != 0; }

static void TrackIRZhTranslate (wchar_t *buf, int size)
{
	for (size_t i = 0; i < sizeof(g_trackIRZhEntries)/sizeof(g_trackIRZhEntries[0]); i++) {
		if (wcscmp (buf, g_trackIRZhEntries[i].en) == 0) {
			wcsncpy_s (buf, size, g_trackIRZhEntries[i].zh, _TRUNCATE);
			return;
		}
	}
}

// 把窗口（及其直接子控件）上的已知英文文本替换为中文；英文模式不做任何事
static void TrackIRZhLocalizeWindow (HWND hWnd)
{
	if (!hWnd || !TrackIRZhActive()) return;
	wchar_t buf[256];
	GetWindowTextW (hWnd, buf, 256);
	if (buf[0]) {
		TrackIRZhTranslate (buf, 256);
		SetWindowTextW (hWnd, buf);
	}
	for (HWND hChild = FindWindowExW (hWnd, NULL, NULL, NULL); hChild;
	     hChild = FindWindowExW (hWnd, hChild, NULL, NULL)) {
		GetWindowTextW (hChild, buf, 256);
		if (!buf[0]) continue;
		TrackIRZhTranslate (buf, 256);
		SetWindowTextW (hChild, buf);
	}
}

#endif // TRACKIRZH_INC