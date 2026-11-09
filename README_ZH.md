# Orbiter 中文汉化版

本仓库是 [Orbiter](https://github.com/orbitersim/orbiter)（开源航天飞行模拟器）的**中文汉化**分支，基于官方最新代码的 64 位构建。

- 汉化仓库：https://github.com/orbiter-newcomer/orbiter-i18n （汉化代码在 `zh-cn` 分支）
- 当前版本：**v1.0.4**

---

## 一、如何切换语言

### 方法一：游戏内切换（推荐）

1. 启动 `Orbiter.exe` 进入 **Launchpad**（启动器）
2. 点击 **Options**（选项）
3. 切换到 **UI** 页面
4. 找到 **Language / 语言** 下拉框，选择 **中文（简体）**
5. 设置**立即生效**（Launchpad 界面实时切换为中文），并自动保存

### 方法二：编辑配置文件

编辑游戏根目录下的 `Localization.cfg`：

```ini
; 中文模式
Language=zh

; 英文模式（默认）
Language=en
```

---

## 二、英文版说明

**英文模式与官方原版完全一致**：所有汉化改动仅在中文模式生效，英文模式下界面、MFD、文本均保持官方原始行为（这是本项目的硬性约束）。

- 需要英文时：按上面的方法切换到 **English**（或 `Language=en`）
- 英文模式下不加载任何中文翻译、不改变任何布局

---

## 三、版本历史（累计改动）

| 版本 | 主要改动 |
|---|---|
| v0.9 | 初版汉化框架（翻译表、语言切换机制） |
| v0.9.1 | 核心界面汉化（启动器、选项页、对话框） |
| v0.9.1.1 | MFD 基础汉化、字体处理 |
| v0.9.2 | 字体统一（Noto CJK）、英文模式隔离、布局偏移架构（中文偏移不污染英文） |
| v0.9.3 | 场景编辑器、载具编辑页全面汉化；乱码系统性修复（W API、UTF-8 路径） |
| v1.0 | MFD 汉化完成（终端/通用相机/上升剖面/姿态控制/转移X）；TransX 修复（近点纬度、发射方位角、栈溢出、UTF-8 截断） |
| v1.0.1 | TransX 航向标签翻译修复 |
| v1.0.2 | 88 星座中文名、天体/发射基地标签翻译、地物数据乱码修复、Map MFD 英文布局隔离、无解数据统一（N/A） |
| v1.0.3 | 分发包完善：SPICE 星历（orbiter.bsp）、低清行星贴图、新版 MSVC 运行时 |
| v1.0.4 | **修复 TransX MFD 打开即崩溃**（BodyProvider 父天体指针未初始化）；重新构建分发包 |

---

## 四、完整汉化内容（累计）

### 1. 核心界面

- **Launchpad 启动器**：全部页面（场景/参数/视频/选项/模块/关于）
- **选项页**：UI 设置、语言选择器
- **物体信息窗口**（Object Info）
- **MFD 选择菜单 / MFD 按钮菜单 / 输入框**
- **菜单栏与自定义命令**
- **Win32 对话框**（运行时重标签，保持 rc 文件英文原样）

### 2. MFD 仪器

**内置 MFD**：Orbit（轨道）、Surface（表面）、Map（地图）、HSI、VOR/VTOL、Docking（对接）、Align Planes（对准轨道面）、Sync Orbit（同步轨道）、Transfer（转移）、COM/NAV（通信导航）

**第三方 MFD**：
- **终端 MFD**（Terminal MFD / LuaMFD）
- **通用相机 MFD**（Generic Camera）
- **上升剖面 MFD**（Ascent profile）
- **姿态控制 MFD**（Attitude RCS）
- **转移X MFD**（TransX）：变量名、帮助系统、菜单、精度档位、数据标签、计划类型、交会术语

汉化内容覆盖：MFD 标题、数据标签、按钮菜单、状态文字、帮助文本、错误提示。

### 3. 场景编辑器（Scenario Editor）

完整界面汉化：焦点/相机目标选择、基地下拉（39 个基地）、天体选择、日期页（传播方式）、姿态页、着陆页、对接页、推进剂设置、自定义按钮等。

### 4. 载具编辑页

- **DeltaGlider**：动画设置、乘客、损伤
- **ShuttleA**：动画设置（起落架/对接口/气闸/辅助舱等）

### 5. 天文与标签

- **天文馆模式**：88 个星座中文名（仙女座/猎户座/大熊座…）
- **标签模式**：天体名（地球/月球/火星…）、**39 个发射基地名**（拜科努尔/卡纳维拉尔角/酒泉/西昌/太原/种子岛/佩内明德…）
- 地物标签（城市/陨石坑等）**保持英文**（拉丁科学命名惯例，按需求不翻译）

### 6. 术语统一

| 英文 | 中文 |
|---|---|
| Eject | 出发 |
| Intercept | 交会 |
| Through point | 飞掠（TransX 专用键隔离） |
| Escape | 逃逸 |
| Coarse / Medium / Fine | 粗略 / 中等 / 精细 |
| 精度档位（10 档） | 粗糙/粗略/中等/精细/超精细/极精细/超高精/微调/自动最小/重置 |

### 7. 字体

- 中文界面使用 **Noto Sans CJK SC** 字体（ImGui 与 MFD 均支持）
- 英文模式保持原始字体与字号
- 语言运行时切换后字体自动重建（无乱码/无方块）

---

## 五、主要缺陷修复（超出翻译范围的代码修复）

本汉化过程中同时修复了若干上游/第三方缺陷：

| 组件 | 问题 | 修复 |
|---|---|---|
| TransX | **打开 MFD 即崩溃（0xc0000005）**：初始化太阳系时父天体指针未初始化，找不到父体时解引用野指针 | 初始化指针、排除 NaN 距离、找不到父体时兜底挂到太阳 |
| TransX | 近点纬度显示 NaN（asin 误用行星半径归一化） | 改用位置向量模长归一化 |
| TransX | 发射方位角无解时仍输出无意义航向 | 改用标准公式 `cos(i)=cos(φ)·sin(A)`，无解显示 N/A / 不可用 |
| TransX | 栈缓冲区溢出导致崩溃 | 统一扩大缓冲区（20/30 → 64 字节） |
| TransX | 中文 UTF-8 截断乱码 | 扩大缓冲区、规范格式化 |
| TransX | 变量名重复绘制导致文字重叠 | 移除重复绘制 |
| 地物数据 | 20 个 `.mkr` 文件含 UTF-8 特殊字符导致乱码 | 规范化为 ASCII（Sao Paulo / Xi'an 等） |
| D3D9Client | 语言运行时切换后 ImGui 字体乱码 | CJK 字体始终合并进字体图集 |
| 核心 | 中文文本经 ANSI API 显示乱码 | 统一改用 W API（UTF-8 → UTF-16）|

---

## 六、构建方法

### 环境要求

- Visual Studio 2022（MSVC 14.44+）
- DirectX SDK（June 2010）
- CMake 3.31+

### Debug 构建（可选）

```powershell
cmake --build build64 --config Debug --parallel 6
```

### Release 构建（Visual Studio / CMake 生成器）

```powershell
# 配置（VS 2022 生成器，x64，安装前缀指向 install64）
cmake -S . -B build64 -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_INSTALL_PREFIX="<源码目录>/install64" `
  -DDXSDK_DIR="D:/Microsoft DirectX SDK (June 2010)"

# 6 核并行构建
cmake --build build64 --config Release --parallel 6
```

构建产物同步到 `install64/Orbiter` 后即可运行。

---

## 七、分发包（v1.0.4）

分发包包含完整的可运行文件：

- **Orbiter.exe**（Release 构建，含全部汉化）
- **汉化插件**：22 个插件 DLL + 场景编辑器 + D3D9Client 渲染器
- **SPICE 星历**：`Kernels/orbiter.bsp`（约 1.5GB，必需）
- **低清行星贴图**：官方低清行星贴图（D3D9Moon / D3D9Mars 等）
- **新版 MSVC 运行时**（14.44，兼容 VS 2022 构建）
- **姿态控制 MFD**（ScriptMFD，含 Attitude RCS 脚本）
- 已禁用开发用 MFD 模板（MFDTemplate）

**注意**：分发包**不含高清行星贴图**（Orbiter 高清贴图为官方单独分发，需从 Orbiter 官方网站下载）。

---

## 八、重要说明（可重构性）

> **本汉化代码以后可能进行重构。**
>
> 当前实现以功能正确为优先，部分实现方式（例如翻译表的全局查找、显示时翻译、中文缓冲区处理、MFD 特例隔离等）后续可能进行优化重构。
>
> 重构时请务必保持：**英文模式与原始上游行为完全一致**，中文调整仅作为固定偏移或条件分支存在。

---

## 九、贡献者

- 汉化与测试：[@orbiter-newcomer](https://github.com/orbiter-newcomer)
- AI 辅助开发：DeepSeek（翻译、代码修改、缺陷排查）

## 十、许可

遵循原项目 [MIT 许可](https://github.com/orbitersim/orbiter/blob/main/LICENSE)。
