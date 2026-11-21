// =============================================================================
// OptionsVisualZh.inc - 启动器 Options -> Visual（视觉设置）页中文布局偏置
//
// 遵循"官方布局 + 偏置"规则（见 i18n/doc/i18n.md）：
//   - .rc 资源保持官方英文布局；英文模式下本文件不做任何布局改动
//   - 中文模式：右侧中文译文比英文宽（"再入等离子火焰"会压住分组框右边线）、
//     "地形自投影"离下方分组框过近，因此把三个分组框加宽、把"行星效果"分组框
//     加高，并把其下方内容整体下移
//   - 偏置数值来自 i18n/layout/zh_CN/layout.ini -> [dialog.visual]，单位 DLU
//     （对话框单位；运行时按当前 DPI 换算为像素，修改无需重新编译）
//   - 首次调用时记录官方布局快照，切换语言时先恢复官方布局再应用当前语言，
//     因此来回切换语言不会累积偏移
// =============================================================================
#ifndef OPTIONSVISUALZH_INC
#define OPTIONSVISUALZH_INC

#include <vector>
#include "Layout.h"   // orbiter::LayoutInt（orbiter::IsChinese 由 Localization.h 提供）

namespace {

// 页面中的一个子控件的官方布局矩形（页面客户坐标）。
struct ZhVisualItem {
	HWND h;
	bool groupBox;
	RECT r;
};

// Options -> Visual 页的布局状态：官方布局快照 + 当前已应用的语言。
struct ZhVisualState {
	HWND                      hPage;
	bool                      snapshotTaken;
	bool                      hasApplied;
	bool                      appliedChinese;
	RECT                      pageRect;
	std::vector<ZhVisualItem> items;
	HWND                      hGroup[3];   // 自上而下：行星效果 / 通用效果 / 相对论效应

	ZhVisualState()
		: hPage(0), snapshotTaken(false), hasApplied(false), appliedChinese(false)
	{
		memset(&pageRect, 0, sizeof(pageRect));
		hGroup[0] = hGroup[1] = hGroup[2] = 0;
	}
};

ZhVisualState g_zhVisualState;

BOOL CALLBACK ZhVisualSnapshotCb (HWND hCtrl, LPARAM lp)
{
	ZhVisualState *st = (ZhVisualState*)lp;
	if (GetParent(hCtrl) != st->hPage) return TRUE;   // only direct children

	ZhVisualItem item;
	item.h = hCtrl;
	item.groupBox = ((GetWindowLongA(hCtrl, GWL_STYLE) & 0xF) == BS_GROUPBOX);
	GetWindowRect(hCtrl, &item.r);
	MapWindowPoints(HWND_DESKTOP, st->hPage, (POINT*)&item.r, 2);
	st->items.push_back(item);
	return TRUE;
}

// 记录页面官方布局（控件矩形 + 页面尺寸 + 三个分组框句柄）。
void ZhVisualTakeSnapshot (HWND hPage)
{
	ZhVisualState &st = g_zhVisualState;
	st = ZhVisualState();          // drop any previous page data
	st.hPage = hPage;
	GetWindowRect(hPage, &st.pageRect);
	EnumChildWindows(hPage, ZhVisualSnapshotCb, (LPARAM)&st);

	// 分组框按上边界排序：行星效果、通用效果、相对论效应
	std::vector<size_t> groups;
	for (size_t i = 0; i < st.items.size(); i++)
		if (st.items[i].groupBox) groups.push_back(i);
	for (size_t a = 1; a < groups.size(); a++) {
		size_t v = groups[a], b = a;
		while (b > 0 && st.items[groups[b-1]].r.top > st.items[v].r.top) {
			groups[b] = groups[b-1];
			b--;
		}
		groups[b] = v;
	}
	for (int i = 0; i < 3 && i < (int)groups.size(); i++)
		st.hGroup[i] = st.items[groups[i]].h;
	st.snapshotTaken = true;
}

// 恢复官方布局（英文模式，或应用中文偏置前的基准）。
void ZhVisualRestoreOfficial (void)
{
	ZhVisualState &st = g_zhVisualState;
	const UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
	for (size_t i = 0; i < st.items.size(); i++) {
		const ZhVisualItem &it = st.items[i];
		SetWindowPos(it.h, NULL, it.r.left, it.r.top,
			it.r.right - it.r.left, it.r.bottom - it.r.top, flags);
	}
	SetWindowPos(st.hPage, NULL, 0, 0,
		st.pageRect.right - st.pageRect.left,
		st.pageRect.bottom - st.pageRect.top, flags | SWP_NOMOVE);
}

// 在官方布局快照的基础上应用中文偏置。
void ZhVisualApplyChinese (void)
{
	ZhVisualState &st = g_zhVisualState;

	// 偏置（DLU）换算为当前 DPI 下的像素：1 DLU -> ux/uy px
	RECT unit = { 0, 0, 1, 1 };
	MapDialogRect(st.hPage, &unit);
	const int ux = (unit.right > 0 ? unit.right : 1);
	const int uy = (unit.bottom > 0 ? unit.bottom : 1);
	const int extraW  = orbiter::LayoutInt("dialog.visual", "groupboxExtraWidth", 12) * ux;
	const int extraH  = orbiter::LayoutInt("dialog.visual", "planetaryExtraHeight", 8) * uy;
	const int offsetY = orbiter::LayoutInt("dialog.visual", "lowerBlockYOffset", 9) * uy;
	if (extraW == 0 && extraH == 0 && offsetY == 0) return;

	// "下方内容"边界：通用效果分组框的官方上边界
	RECT lower = { 0, 0, 0, 0 };
	GetWindowRect(st.hGroup[1], &lower);
	MapWindowPoints(HWND_DESKTOP, st.hPage, (POINT*)&lower, 2);
	const int lowerTop = lower.top;

	const UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
	for (size_t i = 0; i < st.items.size(); i++) {
		const ZhVisualItem &it = st.items[i];
		int x = it.r.left, y = it.r.top;
		int w = it.r.right - it.r.left, h = it.r.bottom - it.r.top;
		if (it.groupBox) {
			w += extraW;                                // 三个分组框加宽
			if (it.h == st.hGroup[0]) h += extraH;      // 行星效果分组框加高
		}
		if (it.r.top >= lowerTop) y += offsetY;         // 下方内容整体下移
		if (x == it.r.left && y == it.r.top &&
			w == it.r.right - it.r.left && h == it.r.bottom - it.r.top)
			continue;
		SetWindowPos(it.h, NULL, x, y, w, h, flags);
	}

	// 页面窗口同步放大（滚动范围由 OptionsPageContainer::SetPageSize 重算）
	SetWindowPos(st.hPage, NULL, 0, 0,
		(st.pageRect.right - st.pageRect.left) + extraW,
		(st.pageRect.bottom - st.pageRect.top) + offsetY,
		flags | SWP_NOMOVE);
}

} // namespace

// 入口：页面初始化与每次显示页面（OptionsPage::UpdateControls）时调用。
void ZhAdjustOptionsVisualLayout (HWND hPage)
{
	if (!hPage) return;

	ZhVisualState &st = g_zhVisualState;
	if (st.hPage != hPage || !st.snapshotTaken)
		ZhVisualTakeSnapshot(hPage);
	if (!st.hGroup[0] || !st.hGroup[1] || !st.hGroup[2])
		return;   // 与预期资源不符（缺少三个分组框）：不做任何布局调整

	const bool wantChinese = orbiter::IsChinese();
	if (st.hasApplied && st.appliedChinese == wantChinese) return;
	if (!st.hasApplied && !wantChinese) {   // 初始创建即英文：官方布局原样
		st.hasApplied = true;
		st.appliedChinese = false;
		return;
	}

	ZhVisualRestoreOfficial();
	if (wantChinese) ZhVisualApplyChinese();
	st.hasApplied = true;
	st.appliedChinese = wantChinese;
}

#endif // OPTIONSVISUALZH_INC
