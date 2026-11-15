// =============================================================================
// DeltaGliderZh.inc - DeltaGlider 飞船「本体」汉化数据（游戏内）
//
// 内容：HUD 中文辅助（空速刹车提示 AIRBRK → 减速板）。
// 场景编辑器的载具编辑页、启动器 DG Configuration 等「游戏外」数据见
// 同目录 DeltaGliderToolsZh.inc。
// =============================================================================
// --------------------------------------------------------------
// HUD 中文辅助：空速刹车提示（AIRBRK）
// --------------------------------------------------------------
// 空速刹车提示文本：中文模式返回"减速板"，英文模式返回 "AIRBRK"
inline const char *ZhDgAirbrakeText ()
{
	return oapiTr ("AIRBRK");
}

// 玻璃座舱 HUD 的英文 AIRBRK 图标：中文模式下改为主程序字体绘制，返回 true 表示已替代
inline bool ZhDgAirbrakeReplaceIcon (int x, int y, int size)
{
	const char *txt = oapiTr ("AIRBRK");
	if (strcmp (txt, "AIRBRK") == 0) return false;   // 英文模式：保留原纹理图标
	oapiZhHudText (x, y, size, txt);
	return true;
}
