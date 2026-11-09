# Orbiter 中文汉化版

本仓库是 [Orbiter](https://github.com/orbitersim/orbiter)（开源航天飞行模拟器）的**中文汉化**分支，基于上游 64 位构建代码。

## 版本

- 当前版本：**v1.0.1**

## 汉化内容

### MFD 仪器（完整汉化）

- **终端 MFD**（Terminal MFD）：菜单、状态文字、命令输入框
- **通用相机 MFD**（Generic Camera）：按钮菜单、状态消息、视图信息
- **上升剖面 MFD**（Ascent profile）：图表标题、轴标签、范围输入框
- **姿态控制 MFD**（Attitude RCS）：模式名、轴名、按钮菜单、配置页
- **转移X MFD**（TransX）：变量名、帮助系统、按钮菜单、精度档位、数据标签、计划类型、交会术语

### 核心界面

- **场景编辑器**（Scenario Editor）：全量界面汉化（基地下拉、天体选择、日期页、姿态页、对接页、推进剂按钮等）
- **载具编辑页**：DeltaGlider、ShuttleA 的动画/乘客/损伤设置页
- **物体信息窗口**、**MFD 选择菜单**、**输入框**、**MFD 按钮菜单**
- **菜单栏与自定义命令**

### 术语统一

| 英文 | 中文 |
|---|---|
| Eject | 出发 |
| Intercept | 交会 |
| Through point | 飞掠（TransX 专用键隔离） |
| Coarse / Medium / Fine | 粗略 / 中等 / 精细 |

### TransX 缺陷修复

- 近点纬度 NaN（`asin` 误用行星半径归一化，改为位置向量模长）
- 发射方位角改用标准公式 `cos(i) = cos(lat)·sin(azimuth)`，无解时显示"航向:不可用"
- 栈缓冲区溢出导致崩溃（小缓冲区统一扩大）
- 中文 UTF-8 截断乱码（缓冲区过小）
- 变量名重复绘制导致的文字重叠

## 重要说明

> **本汉化代码以后可能进行重构。**
>
> 当前实现以功能正确为优先，部分实现方式（例如翻译表的全局查找、显示时翻译、中文缓冲区处理、MFD 特例隔离等）后续可能进行优化重构。
>
> 重构时请务必保持：**英文模式与原始上游行为完全一致**，中文调整仅作为固定偏移或条件分支存在。

## 构建

Windows 64 位构建（需 Visual Studio 2022 与 DirectX SDK）：

```
build64.cmd
```

构建产物位于 `orbiter/out/build/windows-x64-debug/`。

## 贡献者

- 汉化与测试：[@orbiter-newcomer](https://github.com/orbiter-newcomer)
- AI 辅助开发：DeepSeek（翻译、代码修改、缺陷排查）

## 许可

遵循原项目 [MIT 许可](https://github.com/orbitersim/orbiter/blob/main/LICENSE)。
