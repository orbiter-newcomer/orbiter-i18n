# Orbiter 中文汉化版

本仓库是 [Orbiter](https://github.com/orbitersim/orbiter)（开源航天飞行模拟器）的**中文汉化**分支，基于官方最新代码的 64 位构建。

- 汉化仓库：https://github.com/orbiter-newcomer/orbiter-i18n （汉化代码在 `zh-cn` 分支）
- 当前版本：**v1.1**

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
| v1.0.5 | **汉化代码重构（一期）**：可剥离的汉化数据集中到独立 `.inc` 文件（官方源码改动量减少约 27%）；MFD 排版改为"官方坐标 + 中文偏置表"架构；TransX 帮助文本按宽度自动折行；D3D9Client 资源/.rc 恢复官方英文；调试代码清理；插件改为 config 加载全部 |
| v1.0.6 | **汉化代码重构（二期）**：布局逻辑抽离；MFD 固定偏移集中到 `MfdZh.inc`（官方源文件中魔法数字清零）；`cw/ch` 改为面板比例定义——面板缩放时 MFD 布局整体等比缩放，不再依赖字体度量 |
| v1.0.7 | **渲染引擎升级**：新增 `D3D9ClientDXVK`（DXVK/Vulkan）官方图形引擎插件变体，在启动器"视频"页直接选择、即时切换（不替换系统 DLL）；分发包构建自动创建 `Textures2` 高清纹理目录骨架 |
| v1.1 | **汉化代码架构重组（三期）**：汉化逻辑与数据彻底分离（`ZhCommon.inc` 只放逻辑；`MfdZh` / `InstrumentZh` / `DialogZh` / `LaunchpadZh` 只放数据）；启动器窗口文本与主程序对话框文本独立成文件；飞船数据改为"本体 + 游戏外工具"双文件；外部 MFD 改为插件自治汉化（插件各自 `*Zh.inc`，中文模式按钮统一字体渲染）；新增玻璃座舱面板导航按钮 / RCS 按钮 / 玻璃座舱 HUD 中文；新增开发者 API（`oapiZhHudText` / `oapiZhActive`）；修复语言切换/设备重建后 HUD 字体失效导致的崩溃、DXVK 引擎下通用相机崩溃、字体降档错误等 |

---

## 四、完整汉化内容（累计）

### 1. 核心界面

- **Launchpad 启动器**：全部页面（场景/参数/视频/选项/模块/关于）与各类消息框
- **选项页**：UI 设置、语言选择器
- **物体信息窗口**（Object Info）
- **MFD 选择菜单 / MFD 按钮菜单 / 输入框**
- **菜单栏与自定义命令**
- **Win32 对话框**（运行时重标签，保持 rc 文件英文原样）
- **主程序 ImGui 对话框**（相机 / 物体信息 / 时间加速 / 地图 / 自定义功能等）

### 2. MFD 仪器

**内置 MFD**：Orbit（轨道）、Surface（表面）、Map（地图）、HSI、VOR/VTOL、Docking（对接）、Align Planes（对准轨道面）、Sync Orbit（同步轨道）、Transfer（转移）、COM/NAV（通信导航）

**第三方 MFD**（按钮标签由各插件自己提供中文）：终端 MFD（Terminal MFD / LuaMFD）、通用相机 MFD（Generic Camera）、上升剖面 MFD（Ascent profile）、上升自动驾驶（AscentAP）、姿态控制 MFD（Attitude RCS）、转移X MFD（TransX）

汉化内容覆盖：MFD 标题、数据标签、按钮菜单、状态文字、帮助文本、错误提示。

### 3. 场景编辑器（Scenario Editor）

完整界面汉化：焦点/相机目标选择、基地下拉（39 个基地）、天体选择、日期页（传播方式）、姿态页、着陆页、对接页、推进剂设置、自定义按钮等。

### 4. 载具编辑页与载具配置

- **DeltaGlider**：动画设置、乘客、损伤页
- **ShuttleA**：动画设置（起落架/对接口/气闸/辅助舱等）
- **启动器"载具配置"**：DG Configuration、Atlantis Configuration

### 5. 2D 玻璃座舱面板与 HUD（v1.1 新增）

- **MFD 按钮**：全部内置 MFD 按钮 + 外部 MFD 按钮（中文模式统一字体渲染；模式选择页保留官方箭头/缩写图集）
- **面板底部按钮**：PWR **电源**（红色）、SEL **选择**、MNU **说明**；MFD 关闭时的大**电源**按钮
- **底部导航按钮**：自稳 / 水平 / 顺行 / 逆行 / 法向+ / 法向- / 定高
- **引擎状态块 RCS 按钮**：关闭 / 旋转 / 平移（激活高亮）
- **玻璃座舱 HUD**：模式标题（轨道/地表/对接）、参考天体中文名、读数标签（高度/半径/地速/轨速/真速）、NAV→接收机、空速刹车提示（减速板）

### 6. 天文与标签

- **天文馆模式**：88 个星座中文名（仙女座/猎户座/大熊座…）
- **标签模式**：天体名（地球/月球/火星…）、**39 个发射基地名**（拜科努尔/卡纳维拉尔角/酒泉/西昌/太原/种子岛/佩内明德…）
- 地物标签（城市/陨石坑等）**保持英文**（拉丁科学命名惯例，按需求不翻译）

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
| 核心 | 中文文本经 ANSI API 显示乱码 | 统一改用 W API（UTF-8 → UTF-16） |
| 核心 / HUD | 切换语言或重开场景（图形设备重建）后，HUD 中文字体缓存仍指向已销毁的设备字体 → 打开场景崩溃 | 图形设备销毁回调中同步释放 HUD 字体缓存，中文绘制时按需重建 |
| D3D9Client / GenericCamera | DXVK 引擎变体下通用相机 MFD 打不开（插件只查找 `D3D9Client.dll`） | 依次尝试 `D3D9Client.dll` / `D3D9ClientDXVK.dll` |
| 面板按钮 | 中文按钮字号降档逻辑错误导致字体过小；RCS 按钮隐藏后坐标被二次读取导致按钮消失 | 修正降档策略；按钮坐标静态缓存，按背景顶点变化自动刷新 |

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

# 安装到 install64
cmake --install build64 --config Release
```

### 安装说明

- 执行 `cmake --install` 时会自动创建 `Textures2\`（高清纹理覆盖目录，加载优先级高于 `Textures\`）的**完整目录骨架**，结构与 `Textures\` 一致；以后直接把高清贴图放进对应子目录即可（同名文件自动覆盖低清版）

### 渲染引擎（可选）

启动器 **视频** 页的“图形引擎”下拉框提供两个 D3D9Client 变体（同一渲染器、不同 D3D9 实现）：

- **D3D9Client**：系统 Direct3D9 / Direct3D9On12（默认，官方行为）
- **D3D9ClientDXVK**：通过 **DXVK** 使用 Vulkan 渲染（需要系统 Vulkan 运行时；对老 D3D9 驱动/新显卡更友好）

DXVK 翻译层文件位于 `RenderingBackends\dxvk\`（随分发包分发），由 `D3D9ClientDXVK` 引擎插件在运行时加载；**不替换、不删除任何 DLL**，切换引擎即时生效（走官方引擎加载机制）。dgVoodoo2 变体因与系统 D3D9 实现共存冲突（全局 API 拦截架构）未包含。

---

## 七、开发者接口与开发规范

> 本章面向需要接入本汉化体系的其他插件 / 载具开发者。所有对外接口都在 `Orbitersdk/include/OrbiterAPI.h` 中声明，插件工程无需额外依赖。

### 1. 对外 API

| API | 说明 |
|---|---|
| `const char *oapiTr(const char *english)` | 查询翻译表：中文模式返回译文（UTF-8），英文模式返回原文。返回指针在进程生命周期内有效 |
| `int oapiZhActive()` | 当前是否为中文模式（返回 1/0）。用于插件只应在中文模式生效的布局偏置/文字替换 |
| `void oapiZhHudText(int x, int y, int size, const char *utf8)` | 以屏幕坐标登记一段玻璃座舱 HUD 中文文字（中文模式生效），由主程序在 HUD 渲染后用字体统一绘制。用于无法用 HUD 纹理字符集表达的船载 HUD 标注 |

### 2. 翻译表与键规则（`Src/Orbiter/Localization.cpp`）

- 翻译表是 `{ 英文, 中文 }` 键值对数组 `g_translations`，运行时构建正/反向查找表
- `orbiter::Tr(key)`：普通查表；`orbiter::Tr2(aliasKey)`：按"别名键"查表
- **别名键规则**：当同一个英文键在不同上下文语义不同（如 Map MFD 按钮 `DN` = 向下滚动，而 Align MFD 内容里的 `DN` = 降交点），用独立别名键（`MFD_MAP_DN` 等）隔离翻译；别名键不参与反向映射，不会污染"中文→英文"表
- **大小写规则**：仅大小写不同但译文不同的键会被标记为"歧义"，此时 `Tr()` 使用严格大小写匹配
- 新增文本的步骤：
  1. 在翻译表中按所在分区（内置 MFD / 对话框 / 启动器 / 插件 MFD / 术语等）添加 `{ "English", "中文" }`
  2. 若该键与其他上下文冲突，创建别名键并在调用处改用 `Tr2()`
  3. 同时验证英文模式：英文模式必须显示原文（不得返回中文）

### 3. 汉化数据文件组织（本仓库规范）

| 类别 | 文件（`Src/Orbiter/`） | 内容 |
|---|---|---|
| 汉化逻辑（唯一放"具体过程"） | `ZhCommon.inc` | 字体缓存、文本宽度估算、居中绘制、按钮标签排版、MFD 排版算法、`ZH_PANEL` 面板绘制流程、`ZH_HUD` HUD 登记/绘制流程、对外接口实现 |
| MFD 数据 | `MfdZh.inc` | 内置 MFD 按钮文字映射数据、各 MFD 布局偏置数据（注释标明"此处使用右对齐/实测列宽"） |
| 其他仪表数据 | `InstrumentZh.inc` | 玻璃座舱面板数据（字号参数、PWR/SEL/MNU、导航/RCS 按钮文本）+ 玻璃座舱 HUD 数据（模式标题、读数标签） |
| 主程序对话框数据 | `DialogZh.inc` | Orbiter 主程序全部 ImGui 对话框文本 |
| 启动器数据 | `LaunchpadZh.inc` | 启动器主窗口/各选项卡/Options 页/加载画面文本（段 A）+ Extra 页补充控件文本（段 B，宏区分） |

**原则**：数据文件只放数据/参数；绘制流程、字体管理、排版算法、对外接口全部在 `ZhCommon.inc`。数据文件通过 `#include` 单向引入逻辑文件（数据 → MfdZh → ZhCommon），不产生循环依赖。

### 4. 飞船 / 插件 / 窗口数据规范

- **每艘飞船最多两个文件**（放在飞船目录下）：
  - `<Ship>Zh.inc`：飞船本体（游戏内 UI、HUD 辅助等）
  - `<Ship>ToolsZh.inc`：游戏外界面（启动器"载具配置"对话框、场景编辑器载具编辑页）。同一文件被不同 DLL 包含时用宏段区分（如 `DG_ZH_EDITOR` / `DG_ZH_CONFIG`）
  - 示例：`DeltaGliderZh.inc` + `DeltaGliderToolsZh.inc`；`AtlantisToolsZh.inc`；`ShuttleAToolsZh.inc`
- **窗口 / 对话框数据**：每个编译目标（DLL）一个 `*Zh.inc`，集中该 DLL 的全部对话框文本，如 `D3D9ClientZh.inc`（AtmoControls/DebugControls/VideoTab 三段合并）、`EditorZh.inc`、`AtmConfigZh.inc`、`MeshdebugZh.inc`
- **官方源码只保留调用点**：`#include` + 少量条件调用（如 `if (orbiter::IsChinese()) DrawZhMfdButtonLabels();`），不把汉化字符串写进官方 `.cpp`

### 5. 外部 MFD 汉化接入规范（插件开发者）

中文模式下所有 MFD 按钮统一由主程序字体渲染（隐藏字符图集 → 字体绘制 `ButtonLabel()` 的返回值），因此插件只需**在中文模式返回中文标签**：

```cpp
char *MyMFD::ButtonLabel (int bt)
{
    static const char *en[] = {"INP", "NEW", "DEL"};
    static const char *zh[] = {"输入", "新建", "删除"};
    if (bt < 0 || bt >= 3) return 0;
    if (oapiZhActive()) return (char*)zh[bt];   // 中文模式返回中文
    return (char*)en[bt];                        // 英文模式保持原文
}
```

- 标签数据建议放在插件目录内的 `<Plugin>Zh.inc`
- 若插件同时需要布局偏置，用 `oapiZhActive()` 判断，英文模式必须返回 0 偏置（保持官方布局）
- 未汉化的第三方 MFD：中文模式会用字体绘制其英文原文（可读、不会崩溃），无需适配
- 参考实现：`Src/Plugin/TransX/TransXZh.inc`、`Src/Plugin/LuaMFD/LuaMfdZh.inc`、`Orbitersdk/samples/AscentMFD/AscentMFDZh.inc`、`OVP/D3D9Client/samples/GenericCamera/GenericCameraZh.inc`、Lua 脚本 MFD 统一映射 `Src/Module/LuaScript/LuaInterpreter/LuaMfdZh.inc`

### 6. 船载 HUD 中文标注接入

无法用 HUD 纹理字符集表达的中文标注（如空速刹车"减速板"），在船模块的 HUD 渲染回调中调用：

```cpp
oapiZhHudText (hps->CX, (int)(hps->CY - hps->Markersize/2), size, oapiTr("AIRBRK"));
```

主程序会将其排入玻璃座舱 HUD 的中文绘制层；英文模式该调用无效（调用方可先用 `oapiZhActive()` 判断）。参考实现：`Src/Vessel/DeltaGlider/DeltaGliderZh.inc`。

### 7. 硬性约束与验证

- **英文模式与上游行为完全一致**：所有汉化代码都在 `orbiter::IsChinese()` 分支内；英文模式不得出现中文、不得改变布局/字号/颜色
- 数据文件（`*Zh.inc`）必须只放数据；逻辑改动放 `ZhCommon.inc` 或对应模块的辅助函数
- 推荐验证流程：
  1. `cmake --build build64 --config Release --parallel 1`（构建）
  2. `cmake --install build64 --config Release`（安装）
  3. 启动游戏分别验证：中文模式各界面 → 切换到 English 回归 → 再切回中文（设备重建）确认无崩溃

---

## 八、贡献者

- 汉化与测试：[@orbiter-newcomer](https://github.com/orbiter-newcomer)
- AI 辅助开发：DeepSeek（翻译、代码修改、缺陷排查）

## 九、许可

遵循原项目 [MIT 许可](https://github.com/orbitersim/orbiter/blob/main/LICENSE)。
