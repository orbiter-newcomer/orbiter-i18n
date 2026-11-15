// =============================================================================
// TrackIRZh.inc - TrackIR 插件界面中文文本（本地化逻辑）
//
// 迁移说明：英文为键、中文为值的数据已并入核心翻译表（i18n/core/PluginZh.inc），
// 本文件只保留本地化逻辑；所有文本通过 oapiTr() 查询。
// 英文模式 = 原版（oapiZhActive() 为 0 时直接返回）。
// =============================================================================
#ifndef TRACKIRZH_INC
#define TRACKIRZH_INC

#include <windows.h>
#include "orbitersdk.h"

// 把窗口（及其直接子控件）上的英文文本替换为核心翻译表中的中文；英文模式不做任何事
static void TrackIRZhLocalizeWindow (HWND hWnd)
{
	if (!hWnd || oapiZhActive() == 0) return;
	wchar_t buf[256];
	GetWindowTextW (hWnd, buf, 256);
	if (buf[0]) {
		char utf8[1024];
		if (oapiWideToUtf8 (buf, utf8, sizeof(utf8)) > 0) {
			const char *zh = oapiTr (utf8);
			if (strcmp (zh, utf8) != 0) SetWindowTextW (hWnd, oapiUtf8ToWString (zh).c_str());
		}
	}
	for (HWND hChild = FindWindowExW (hWnd, NULL, NULL, NULL); hChild;
	     hChild = FindWindowExW (hWnd, hChild, NULL, NULL)) {
		GetWindowTextW (hChild, buf, 256);
		if (!buf[0]) continue;
		char utf8[1024];
		if (oapiWideToUtf8 (buf, utf8, sizeof(utf8)) > 0) {
			const char *zh = oapiTr (utf8);
			if (strcmp (zh, utf8) != 0) SetWindowTextW (hChild, oapiUtf8ToWString (zh).c_str());
		}
	}
}

#endif // TRACKIRZH_INC
