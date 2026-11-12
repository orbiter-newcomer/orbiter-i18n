# Orbiter 汉化开发交接文档 (HANDOVER)

> 最后更新：**v1.1 之后**（tag `v1.1` = commit `d728b954`）。
> 本文只记录事实：做了什么、没做什么、要注意什么、开发规范。

---

## 1. 项目概况

| 项目 | 内容 |
|---|---|
| 源码目录 | `D:\AI_projet\orbiter\orbiter`（git 管理） |
| 构建目录 | `D:\AI_projet\orbiter\orbiter\build64`（VS2022 x64，CMake 生成器） |
| 安装/运行目录 | `D:\AI_projet\orbiter\orbiter\install64\Orbiter`（`cmake --install` 产出） |
| 远程仓库 | `https://github.com/orbiter-newcomer/orbiter-i18n.git` |
| 分支 | `main`（最新）、`zh-cn`（与 main 同步） |
| 当前版本 | **v1.1**（tag `v1.1`，HEAD `d728b954`） |
| 构建环境 | VS2022 + MSVC 14.44、CMake 3.31+、DirectX SDK (June 2010) |

---

## 2. 我做了啥（v1.1 范围内）

### 2.1 汉化代码架构重组（三期）

- **逻辑与数据彻底分离**：`ZhCommon.inc` 只放逻辑（字体缓存、文本宽度估算、居中绘制、按钮标签排版、MFD 排版算法、面板/HUD 绘制流程、对外接口）；数据文件只放数据。
- **数据文件**：`MfdZh.inc`（MFD 按钮文字映射 + 布局偏置）、`InstrumentZh.inc`（玻璃座舱面板数据 + HUD 数据）、`DialogZh.inc`（主程序 ImGui 对话框文本，299 条）、`LaunchpadZh.inc`（启动器全部窗口文本 + Extra 页补充，宏段）。
- **官方源码只保留调用点**：`Defpanel.cpp`（`#define ZH_PANEL` + include + 3 调用 + HUD 字体释放 extern/调用）、`hud.cpp`（`#define ZH_HUD` + include + 6 调用）、`Pane.cpp`（1 extern + 1 调用）、`Localization.cpp`（2 处 include + `BuildTables()` 两轮合并）、`TabExtra.cpp`（1 处 include）。
- **数据迁移**：`PWR/SEL/MNU` 数据从 InstrumentZh 移入 `MfdZh`；删除合并后的 `DefpanelZh.inc`、`MfdLayoutZh.inc`、`TabExtraZh.inc`。
- **启动器/对话框文本抽取**：从 Localization.cpp 抽出启动器 28 段 + 星座 92 条 → `LaunchpadZh.inc`；主程序对话框 299 条 → `DialogZh.inc`，由 `BuildTables()` 合并（翻译机制不变）。

### 2.2 飞船 / 插件 / 窗口数据

- **飞船数据改为每船最多两个文件**（本体 + 游戏外工具，宏段区分 DLL）：
  - `DeltaGliderZh.inc`（HUD 辅助）+ `DeltaGliderToolsZh.inc`（`DG_ZH_EDITOR` 场景编辑器载具页 / `DG_ZH_CONFIG` 启动器 DG Configuration）；
  - `AtlantisToolsZh.inc`（启动器 Atlantis Configuration）；
  - `ShuttleAToolsZh.inc`（场景编辑器载具页）。
  - 删除 `DGConfiguratorZh.inc`、`AtlantisConfigZh.inc`、`ShuttleAZh.inc`。
- **D3D9Client 窗口数据合并**：`D3D9ClientZh.inc`（AtmoControls/DebugControls/VideoTab 三段，宏区分），删除原三个 inc。

### 2.3 外部 MFD 自治汉化 + 统一渲染

- 中文模式**所有 MFD 按钮统一字体渲染**（隐藏字符图集 → 字体绘制 `ButtonLabel()` 返回值）；英文模式保持原图集渲染。
- 移除 `MfdZh.inc` 的插件白名单；各插件自带中文标签并在 `ButtonLabel()` 中判断 `oapiZhActive()`：
  `Src/Plugin/TransX/TransXZh.inc`、`Src/Plugin/LuaMFD/LuaMfdZh.inc`、`Orbitersdk/samples/AscentMFD/AscentMFDZh.inc`、`OVP/D3D9Client/samples/GenericCamera/GenericCameraZh.inc`、Lua 脚本 MFD 统一映射 `Src/Module/LuaScript/LuaInterpreter/LuaMfdZh.inc`。

### 2.4 玻璃座舱面板与 HUD

- 面板：MFD 按钮（内置+插件）、PWR/SEL/MNU、MFD 关闭时的大电源按钮、底部导航按钮（自稳/水平/顺行/逆行/法向+/法向-/定高）、引擎状态块 RCS 按钮（关闭/旋转/平移，自绘适配背景纹理按钮框、激活高亮、坐标静态缓存）。
- 模式选择页：特判保留官方图集渲染（箭头/缩写），并调用官方 `RepaintMFDButtons(id)` 恢复标签顶点位置。
- HUD：模式标题（轨道/地表/对接）、参考天体中文名、读数标签（高度/半径/地速/轨速/真速）、NAV→接收机、空速刹车提示（减速板）；字号/位置按用户反馈微调。
- 按钮文字统一右下移 1px、颜色统一。

### 2.5 缺陷修复

- 切换语言/重开场景（图形设备重建）后打开场景崩溃：设备销毁回调中释放 HUD 中文字体缓存（`ZhHudReleaseFonts`），按需重建。
- DXVK 引擎变体下通用相机 MFD 崩溃：`gcCore.h`/`gcGUI.h` 依次查找 `D3D9Client.dll` / `D3D9ClientDXVK.dll`。
- 面板按钮字体过小（降档策略错误）、RCS 按钮消失（坐标被隐藏操作污染）、RCS 按钮双框（背景纹理自带按钮框）、模式选择页箭头错位（覆盖了官方 `RepaintMFDButtons` 的位置）。

### 2.6 对外接口与文档

- 新增开发者 API（`Orbitersdk/include/OrbiterAPI.h` + `OrbiterAPI.cpp`）：`oapiZhHudText()`、`oapiZhActive()`（`oapiTr()` 此前已有）。
- README_ZH.md / README_EN.md 更新：v1.1 版本历史；新增"开发者接口与开发规范"章节；按用户要求删除"术语统一""分发包"章节。
- v1.1 已提交推送：commit `d728b954`（main + zh-cn + tag `v1.1`），48 个文件变更（+2272/−1120）。

---

## 3. 我没做啥

- **通用相机画面四角亮度异常**：用户已明确交给其它开发者处理，本轮未处理（与汉化代码无关）。
- `HANDOVER_ZH.md`（本文档）未纳入 git 提交，保持工作区文档；需要时再提交。

---

## 4. 要注意啥

### 4.1 操作规则

1. **编译/安装前必须确认游戏已关闭**（`Get-Process Orbiter` 为空），游戏运行时不要构建或安装。
2. **构建与安装是一体操作**：要么都做，要么都不做。
3. **不要擅自关闭用户的游戏**——除非用户明确授权（如"帮我关掉并构建安装"）。关闭时先 `CloseMainWindow()`，等待后再 `Stop-Process` 兜底。
4. 打开游戏（启动器）后**不要操作/测试/关闭**，测试由用户进行。启动用 WMI：
   `Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{ CommandLine = "`"<install64>\Orbiter.exe`""; CurrentDirectory = "<install64>" }`
   （不要用 `Start-Process` / `cmd /c start`，会被工具 job 清理杀掉。）
5. **不要改用户配置**（`Orbiter.cfg`、`D3D9Client.cfg`、`Localization.cfg` 等），除非用户明确要求。
6. 构建日志写仓库根，错误过滤：`error C|error LNK|error MSB|Build FAILED`。

### 4.2 构建（多核并行）

```powershell
$vsroot = 'D:\Microsoft Visual Studio\2022\Community'
$vcvars = Join-Path $vsroot 'VC\Auxiliary\Build\vcvarsall.bat'
$cmake  = Join-Path $vsroot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
# 构建（多核并行，已验证可用）
cmd /c "call `"$vcvars`" x64 >nul && cd /d D:\AI_projet\orbiter\orbiter && `"$cmake`" --build build64 --config Release --parallel 8 > build.log 2>&1"
# 安装
cmd /c "`"$cmake`" --install `"D:\AI_projet\orbiter\orbiter\build64`" --config Release > install.log 2>&1"
```

- **用多核并行**（`--parallel 8` 或按机器核数）。`--parallel 8` 已实测构建成功（`BUILD EXIT: 0`）。
- **如果构建失败且确认是权限/沙箱问题**（临时文件被拒、路径不可写、进程被拦截等）：**向用户申请更高权限**（`danger-full-access` / workspace-write）后重试；不要用 `--parallel 1` 降级，也不要改工程配置绕过。
- 成功标志：`BUILD EXIT: 0` / `INSTALL EXIT: 0`；安装后校验 `build64\Release\Orbiter.exe` 与安装版 SHA256 一致。

### 4.3 常见坑

1. **inc include 位置**：`InstrumentZh.inc`（→`MfdZh.inc`→`ZhCommon.inc`）必须在 `Defpanel.cpp` 的 `#include "Pane.h"`、`using std::min/max;`、`Vessel.h` 之后；`hud.cpp` 中在 `#include "hud.h"` 之后。
2. **汉化绘制函数必须是 `DefaultPanel` 成员函数**（要访问 `pane->mfd`/`pane->H`/`mesh`/`btnw` 等私有成员）。
3. **编码**：新增 `.inc` 与 `ZhCommon.inc` 等为 **UTF-8 无 BOM**；官方老文件（如 `Defpanel.h`）是 CP1252，不要写中文注释。
4. **PowerShell 改源码优先按行操作**（行数组定位/替换），here-string 整段替换易踩空白/换行不匹配；改完检查括号结构并用 `git diff` 核对；写回用 `UTF8Encoding($false)`。
5. **翻译表**：精确匹配；后加入条目覆盖先前；别名键（`MFD_*`/`PANEL_*`/`TRX_*` 等）不参与反向映射；仅大小写不同且译文不同的键会被标记歧义（严格匹配）。
6. **宏段/TU 隔离**：`ZH_PANEL`、`ZH_HUD`、`DG_ZH_EDITOR`、`DG_ZH_CONFIG`、`ZH_D3D9_*`、`LAUNCHPAD_ZH_*` 各自只在对应 cpp 中定义；同一数据文件被多个 DLL 包含时必须用宏段分隔。
7. **顶点坐标只能读一次**：隐藏按钮标签是把顶点 x 改成 `-10000`（持久修改）。隐藏后还要绘制的东西必须**先缓存坐标**（参考 RCS 按钮），否则下一帧读到 -10000，表现为按钮/文字消失。
8. **模式选择页**：不要自己保存/恢复按钮标签顶点——官方位置由 `RepaintMFDButtons()` 按"按钮中心+字符宽"计算；中文绘制特判时调用官方 `RepaintMFDButtons(id)` 即可。
9. **字体缓存按 TU 各一份**：面板字体在 `CreateZhMfdButtonFonts`/`ReleaseZhMfdButtonFonts`（Defpanel.cpp TU）；HUD 字体在 hud.cpp TU，由 `ZhHudReleaseFonts`（设备销毁回调）释放；设备重建后使用旧字体对象会崩溃。
10. **英文模式硬约束**：所有汉化都在 `IsChinese()` 分支内；`Tr()` 在英文模式返回原文；布局偏移/字体绘制只在中文模式生效。
11. **验证场景**：`Scenarios/Test_zh_mfd1.scn`（左 Orbit / 右 Docking）；带 `Playback` 行会进入回放（无 2D 面板），需删除该行。

---

## 5. 开发规范

### 5.1 翻译表与运行时 API（`Src/Orbiter/Localization.cpp/.h`）

| API | 用途 |
|---|---|
| `Tr(const char*)` | 英文→当前语言；无翻译返回原指针（英文模式=原文） |
| `Tr2(const char*)` | 别名键翻译（同英文键不同语义时隔离） |
| `IsChinese()` / `GetLanguage()` / `SetLanguage()` | 语言状态 |
| `Utf8ToWideString()` / `WideToUtf8String()` | UTF-8 ↔ UTF-16 |
| `ComboAddStringUtf8()` / `ListBoxAddStringUtf8()` | 控件添加 UTF-8 文本（必须用） |
| `SetWindowTextUtf8()` | 设置窗口标题/控件文字（必须用） |
| `LocalizeWindowTree(HWND)` | 递归汉化 Win32 窗口树 |
| `RetranslateCurrent()` | 运行时重译动态控件 |

- `BuildTables()` 先遍历 `g_translations`，再合并 `DialogZh.inc`（`g_zhDialogEntries`）与 `LaunchpadZh.inc` 段 A（`g_zhLaunchpadEntries`）。
- 切换语言：`SetLanguage()` + `LocalizeWindowTree()`，注意 `IsRelocalizing()` 防递归。

### 5.2 对外开发者 API（`Orbitersdk/include/OrbiterAPI.h`）

| API | 说明 |
|---|---|
| `const char *oapiTr(const char *english)` | 插件查翻译表（中文模式译文，英文模式原文） |
| `int oapiZhActive()` | 是否中文模式（1/0），用于插件 UI 偏置/文字替换 |
| `void oapiZhHudText(int x, int y, int size, const char *utf8)` | 登记玻璃座舱 HUD 中文文字（中文模式生效，由主程序字体绘制） |

### 5.3 汉化代码组织规范（核心）

- **逻辑唯一放 `ZhCommon.inc`**：字体、文本宽度估算、居中绘制、`DrawButtonLabel`（排版/降档）、MFD 排版算法、`ZH_PANEL`/`ZH_HUD` 流程、对外接口实现。
- **数据文件只放数据/参数**：`MfdZh.inc`（MFD 按钮映射 + 布局偏置 `zhOff_*`）、`InstrumentZh.inc`（面板字号档位 `g_zhPanelFontSpec`、按钮文本、HUD 数据）、`DialogZh.inc`、`LaunchpadZh.inc`。
- **包含关系单向**：数据 → `MfdZh` → `ZhCommon`；官方源码只留 `#include` + 少量条件调用。
- 新增文本：先查重 → 加翻译表条目/对应窗口数据文件 → 必要时用别名键 → 验证英文模式。
- 新增数据文件时在文件头注释写清楚：用途、被哪个 cpp/DLL 包含、是否需要宏。

### 5.4 飞船 / 插件 / 窗口数据规范

- 每艘飞船最多两个文件：`<Ship>Zh.inc`（本体）+ `<Ship>ToolsZh.inc`（游戏外：启动器载具配置/场景编辑器载具页）；不同 DLL 包含同一文件用宏段。
- 每个编译目标（DLL）的对话框数据放该 DLL 目录下一个 `*Zh.inc`（如 `D3D9ClientZh.inc`、`EditorZh.inc`、`AtmConfigZh.inc`、`MeshdebugZh.inc`）。
- 外部 MFD：插件目录内 `<Plugin>Zh.inc` 存标签数据，`ButtonLabel()` 中：
  ```cpp
  if (bt < 0 || bt >= N) return 0;
  if (oapiZhActive()) return (char*)zh[bt];   // 中文模式
  return (char*)en[bt];                        // 英文模式
  ```
  按钮绘制由主程序统一处理，插件无需关心。
- 船载 HUD 中文标注：`oapiZhHudText(x, y, size, oapiTr("KEY"))`（参考 `DeltaGliderZh.inc` 的 AIRBRK→减速板）。

### 5.5 玻璃座舱面板 / HUD 渲染规则

- 中文模式：所有 MFD 按钮统一字体渲染（隐藏图集 → 字体绘制 `ButtonLabel()`）；英文模式：原图集渲染。
- 模式选择页：保留官方图集 + `RepaintMFDButtons(id)` 重绘。
- 按钮文字统一右下移 1px（`DrawButtonLabel` 内 `+1,+1`）。
- 字号档位按面板高度 clamp（`g_zhPanelFontSpec`，档0 1-2字/档1 3字/档2 4字/档3 小按钮）；`DrawButtonLabel` 支持 `fiOverride` 强制档位、`maxW` 自动降档。
- 颜色格式 `0xBBGGRR`（高字节可带 alpha）；面板按钮色 `{0x77DC77,0x7777DC,0x77DCDC,0xDC7777}`，电源按钮红 `0xFF8888FF`。
- RCS 按钮：背景纹理自带按钮框 → 非激活不画填充；激活仅在框内（内缩 2px）铺亮色底；按钮位置静态缓存。
- HUD：`ZhHud::AddBand/AddReadoutLabel/AddDockNav` 登记，`Pane::Render` 中 `ZhHudDrawFrame()` 绘制；读数标签字号 `max(16, scal*17)`，右侧标签右移 4*scale。

### 5.6 构建与交付流程

1. 确认游戏关闭 → 2. 多核构建（`--parallel 8`）→ 3. `cmake --install` → 4. 校验 SHA256 → 5. WMI 打开启动器交用户验证。
2. 用户确认后更新 README 版本历史（`README_ZH.md`/`README_EN.md`）→ 提交（提交信息用文件方式 `git commit -F`，避免 PowerShell 参数解析问题）→ tag（`git tag vX.Y`）→ 推送 `origin main`、`origin main:zh-cn`、`origin vX.Y`。

---

## 6. 关键文件索引

| 文件 | 作用 |
|---|---|
| `Src/Orbiter/ZhCommon.inc` | 汉化逻辑（字体/绘制/排版/面板与 HUD 流程/接口实现） |
| `Src/Orbiter/MfdZh.inc` | MFD 数据（按钮文字映射、布局偏置） |
| `Src/Orbiter/InstrumentZh.inc` | 玻璃座舱面板 + HUD 数据 |
| `Src/Orbiter/DialogZh.inc` | 主程序 ImGui 对话框文本 |
| `Src/Orbiter/LaunchpadZh.inc` | 启动器窗口文本（段 A）+ Extra 页补充（段 B） |
| `Src/Orbiter/Localization.cpp/.h` | 翻译表 + 运行时 API + `BuildTables` 合并 |
| `Src/Orbiter/Defpanel.cpp/.h` | 2D 面板渲染（官方）+ 汉化调用点 |
| `Src/Orbiter/hud.cpp` | HUD 绘制（官方）+ 汉化登记调用 |
| `Src/Orbiter/Pane.cpp` | HUD 渲染后叠加中文（1 行调用） |
| `Src/Orbiter/OrbiterAPI.cpp` / `Orbitersdk/include/OrbiterAPI.h` | 新增 API 实现/声明 |
| `Src/Orbiter/Mfd*.cpp` | 内置 MFD（布局 + 按钮标签） |
| `Src/Vessel/DeltaGlider/DeltaGliderZh.inc`、`DeltaGliderToolsZh.inc` | DG 本体 / 游戏外 |
| `Src/Vessel/Atlantis/AtlantisToolsZh.inc` | Atlantis 游戏外（配置工具） |
| `Src/Vessel/ShuttleA/ShuttleAToolsZh.inc` | ShuttleA 游戏外（载具页） |
| `Src/Plugin/TransX/TransXZh.inc`、`Src/Plugin/LuaMFD/LuaMfdZh.inc`、`Orbitersdk/samples/AscentMFD/AscentMFDZh.inc`、`OVP/D3D9Client/samples/GenericCamera/GenericCameraZh.inc`、`Src/Module/LuaScript/LuaInterpreter/LuaMfdZh.inc` | 外部 MFD 中文标签 |
| `OVP/D3D9Client/D3D9ClientZh.inc` | D3D9Client 三个设置对话框文本 |
| `OVP/D3D9Client/gcCore.h`、`gcGUI.h` | DXVK 引擎变体兼容 |
| `README_ZH.md` / `README_EN.md` | 使用说明/版本历史/开发者规范 |
