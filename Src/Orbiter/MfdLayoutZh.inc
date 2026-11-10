// MfdLayoutZh.inc - MFD 中文排版辅助（运行时实测宽度，随语言自适应）
//
// 把“右对齐表头”“实测列宽布局”等排版逻辑从官方 .cpp 中抽出的公共实现：
// 官方文件中只保留各文本的构造（Tr() 调用点），布局计算集中在这里。
// 英文模式同样走这些 helper（实测宽度与官方固定坐标等价），不改变官方行为。

#ifndef MFDLAYOUTZH_INC
#define MFDLAYOUTZH_INC

// 右对齐绘制若干文本段：
//   rightX   右边界（最后一段文本的右端）
//   y        行位置
//   texts    文本数组，n 段
//   colors   每段颜色，n 项（可为 NULL）
//   gaps     每段之后的间距（单位：字符宽 cw），最后一项应为 0
//   n        段数
//   cw       字符宽
// 返回最左绘制位置。
inline int DrawRightAlignedSegments(oapi::Sketchpad *skp, int rightX, int y,
	const char *const *texts, const DWORD *colors, const int *gaps, int n, int cw)
{
	int total = 0;
	for (int i = 0; i < n; i++)
		total += (int)skp->GetTextWidth(texts[i], (int)strlen(texts[i])) + gaps[i]*cw;
	int x = rightX - total;
	for (int i = 0; i < n; i++) {
		if (colors) skp->SetTextColor(colors[i]);
		skp->Text(x, y, texts[i], (int)strlen(texts[i]));
		x += (int)skp->GetTextWidth(texts[i], (int)strlen(texts[i])) + gaps[i]*cw;
	}
	return rightX - total;
}

// 三列（表头 + 两行数据）按实测列宽紧凑排列，返回下一行 y。
// 画笔（pen）由调用方在绘制前设置。
inline int DrawMeasured3Columns(oapi::Sketchpad *skp, int x, int y, int cw, int ch,
	const char *const *hcol, const char *const *row1, const char *const *row2)
{
	int colx[3], colw[3];
	for (int i = 0; i < 3; i++) {
		colw[i] = (int)skp->GetTextWidth(hcol[i], (int)strlen(hcol[i]));
		int w = (int)skp->GetTextWidth(row1[i], (int)strlen(row1[i]));
		if (w > colw[i]) colw[i] = w;
		w = (int)skp->GetTextWidth(row2[i], (int)strlen(row2[i]));
		if (w > colw[i]) colw[i] = w;
		colw[i] += cw;
	}
	colx[0] = x; colx[1] = colx[0]+colw[0]; colx[2] = colx[1]+colw[1];
	for (int i = 0; i < 3; i++) skp->Text(colx[i], y, hcol[i], (int)strlen(hcol[i]));
	y += ch+1;
	for (int i = 0; i < 3; i++) skp->Line(colx[i], y, colx[i]+colw[i]-cw/2, y);
	y += 1;
	for (int i = 0; i < 3; i++) skp->Text(colx[i], y, row1[i], (int)strlen(row1[i]));
	y += ch;
	for (int i = 0; i < 3; i++) skp->Text(colx[i], y, row2[i], (int)strlen(row2[i]));
	return y + 2*ch;
}

#endif // MFDLAYOUTZH_INC
