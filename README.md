![Orbiter logo](./Src/Orbiter/Bitmaps/banner.png)

# Orbiter 中文汉化 + 相对论扩展

## 一、简介

本仓库是 [Orbiter](https://github.com/orbitersim/orbiter)（开源航天飞行模拟器）的 **64 位分支**，在官方游戏基础上提供：

- **中文汉化**：界面、MFD、场景编辑器、载具、玻璃座舱面板与 HUD、帮助及场景文本；完善的国际化 API（翻译查询、排版数据、对话框布局自适应）
- **相对论扩展**：新增相对论引力模型、黑洞/中子星天体与相对论视觉效应
- **地形渲染优化**：新增地形自投影与地形预加载系统
- **性能优化**：物理/渲染并行、多核计算、渲染优化
- **残骸与撞击系统**：新增损毁事件、残骸模型生成与损毁物理
- **新的接触求解器**：新增求解方式、设置页与接触参数整定
- **参数整定**：自动驾驶、悬停高度保持与热控系统参数整定
- **热控系统优化**：新增质量模型、时间积分等热控优化

- 仓库：https://github.com/orbiter-newcomer/orbiter-i18n
- 当前版本：**v1.3.2**（地形渲染优化、性能优化、新的接触求解器、残骸与撞击系统、参数整定、热控系统优化、中文排版完善、国际化 API）
- English readme: [README_EN.md](./README_EN.md)

---

## 二、操作方法

### 1. 安装与运行

1. 使用官方 Orbiter 2024 安装（需要行星纹理等资源），或使用本项目的 `install64` 安装目录；
2. 将构建产物（`Orbiter.exe`、`Modules/Plugin/D3D9Client.dll` 等）覆盖到安装目录；
3. 启动 `Orbiter.exe` 进入 Launchpad，选择场景开始。

### 2. 切换语言

**方法一：游戏内切换（推荐）**

1. 启动 `Orbiter.exe` 进入 **Launchpad**（启动器）
2. 点击 **Options**（选项）→ **UI** 页面
3. 找到 **Language / 语言** 下拉框，选择 **中文（简体）**（立即生效并自动保存）

**方法二：编辑配置文件**

编辑游戏根目录下的 `Localization.cfg`：

```ini
Language=zh   ; 中文模式
Language=en   ; 英文模式（默认）
```

**英文版说明**：英文模式保持官方表现；汉化内容仅在中文模式生效。

### 3. 相对论设置

1. **Options → Physics**：相对论物理总开关（关闭后相对论物理与视效均不生效）；
2. **Options → Visual**：相对论视觉开关（多普勒、参考系效应、引力引起的光线偏折等）；
3. **Extra → Relativity effects**：引力模型（牛顿 / 1PN / Kerr）、电荷引力开关；
4. **Extra → 调试渲染选项**：透镜线框、天体位置标记；
5. 相对论测试场景：`Scenarios\Relativity\` 下 `NeutronStar`、`StellarBlackHole`、`SupermassiveBlackHole`。

### 4. 配置项

本节列出**启动器界面中可设置**的配置项及其对应的配置文件键。无图形界面的配置项见 [6.4 配置参数（开发者）](#4-配置参数开发者)。

> **生效方式**：这些配置项都需要**重启启动器**（重启启动器 = 重启整个 Orbiter），**重启模拟场景不会生效**；`i18n/layout/zh_CN/layout.ini`、`i18n/locales/zh_CN/*.po` 切换一次语言即可生效。配置文件在 Orbiter 退出时会被回写，建议关闭后再修改。
>
> `Orbiter.cfg` 的键必须写在**行首**、布尔值只认 `TRUE`/`FALSE`（写成 `1` 会被忽略）；`D3D9Client.cfg` 允许行首缩进、键名需完整精确。

| 文件 | 作用 |
|---|---|
| `Orbiter.cfg` | 主程序：视觉、物理、损伤模拟、接触模型、相对论等参数 |
| `D3D9Client.cfg` | D3D9Client 渲染器：地形、引力透镜等渲染参数 |
| `Localization.cfg` | 界面语言 |

#### 4.1 语言（Options → UI）

| 键 | 默认 | 说明 |
|---|---|---|
| `Language` | en | 界面语言：`zh` = 中文、`en` = 英文 |

#### 4.2 视觉（Options → Visual）

| 键 | 默认 | 说明 |
|---|---|---|
| `EnableTerrainHorizonShadow` | TRUE | 地形自投影开关（"Planetary effects / 行星效果"分组内，需该天体有地形高程数据） |
| `RelativityDoppler` | FALSE | 多普勒效应 |
| `RelativityAberration` | FALSE | 参考系效应（光行差） |
| `RelativityLensing` | FALSE | 引力引起的光线偏折（引力透镜） |
| `RelativityExposure` | 2.0 | 多普勒曝光白点（0 ~ 8） |

> 引力红移（`RelativityGravRedshift`）随多普勒效应开关联动。

#### 4.3 物理与相对论（Options → Physics / Extra → Relativity effects / Extra → 调试渲染选项）

| 键 | 默认 | 说明 |
|---|---|---|
| `Relativity` | FALSE | 相对论物理总开关 |
| `RelativityGravityMethod` | 0 | 引力模型：0 = 1PN（默认）、1 = Kerr、2 = 牛顿 |
| `RelativityCharge` | FALSE | 电荷引力效应（Reissner–Nordström） |
| `RelativityLensWireframe` | FALSE | 引力透镜线框（调试显示） |
| `RelativityBodyMarkers` | FALSE | 相对论修正的天体位置标记（调试显示） |

#### 4.4 损伤与故障模拟（Options → Vessel）

| 键 | 默认 | 说明 |
|---|---|---|
| `DamageModel` | 0 | 0 = 关闭（不会产生残骸）、1 = 开启 |

#### 4.5 接触模型（Extra → Contact model）

| 键 | 默认 | 说明 |
|---|---|---|
| `ContactSolver` | 0 | 接触求解方式：0 = 官方默认、1 = 隐式阻尼、2 = 全隐式 |
| `ContactGearStiffness` | 0 | 起落架接触刚度 [N/m]；0 = 飞船原始值 |
| `ContactGearDampingRatio` | 0 | 起落架阻尼比；0 = 原始阻尼 |
| `ContactGearFriction` | 0 | 起落架摩擦系数；0 = 原始值 |
| `ContactHullStiffness` | 0 | 船身/机体接触刚度 [N/m]；0 = 原始值 |
| `ContactHullDampingRatio` | 0 | 船身/机体阻尼比；0 = 原始阻尼 |
| `ContactHullFriction` | 0 | 船身/机体摩擦系数；0 = 原始值 |

> 接触点分配：前 3 个为起落架（机轮），其余为船身/机体。

#### 4.6 修改建议

- 按需修改；无关键保持默认，未知行保持不动。
- 地形自投影影响帧率；低端 GPU 可在启动器中取消勾选。

---

## 三、新特性

### 1. 相对论物理

- 新增引力模型：加入了 1PN（弱场近似）、Kerr（含参考系拖曳）两种相对论引力模型
- 新增相对论动力学：加入了相对论质量与时间膨胀（通过时间加速倍率体现）
- 新增天体：加入了黑洞、中子星
- 新增相对论测试场景：加入了 `NeutronStar`、`StellarBlackHole`、`SupermassiveBlackHole` 测试场景（`Scenarios\Relativity\`）
- 新增电荷引力：加入了 Reissner–Nordström 电荷修正
- 新增保护机制：加入了超光速、强场区与数值异常保护（触发时飞船损毁并提示；载入超光速场景时自动限速）
- 新增设置界面：Options → Physics（总开关）、Extra → Relativity effects（引力模型 / 电荷引力）

### 2. 相对论视觉

- 新增多普勒效应：加入了速度和引力对光波波长和亮度的影响
- 新增参考系效应：加入了速度和引力引起的观察者参考系效应
- 新增引力引起的光线偏折：加入了引力造成的光线偏折效应（主要表现为引力透镜）
- 新增设置界面：Options → Visual，各项可独立开关

### 3. 中文汉化

- 新增中文界面：加入了 Launchpad 全部页面与选项页、内置及常见第三方 MFD、场景编辑器、载具编辑页、玻璃座舱面板与 HUD、帮助文本、场景/教程说明、天体/发射基地/星座名称的中文翻译
- 新增中文排版适配：加入了按钮、标签、对话框的中文布局适配（不压字、不重叠）
- 新增天体名格式：加入了"中文名（英文名）"格式，如"土卫六（泰坦）"
- 新增国际化 API：加入了翻译查询（`oapiTr`、`oapiTrContext`、`oapiTrName`）、排版数据读取（`oapiGetLayoutInt/Float/String`）、中文对话框布局自适应（`oapiZhFitControlText/Row/Height`、`oapiZhOffsetControls/Window`、`oapiZhLayoutDialogButtons`、`oapiZhFitDialogButtons`）；译文与排版数据可在安装目录直接修改，无需重新编译

### 4. 地形渲染优化

- 新增地形自投影：加入了地形对阳光的遮挡阴影（山体在背光坡面与谷地投影，随太阳方位与高度变化；仅对相机所在天体近地表生效，需该天体有高程数据）
- 新增地形预加载系统：加入了后续细节层级地形数据的提前加载，减少进入场景/切换视角时的低清与卡顿
- 新增地形加载与切换优化：加入了可选细节层级过渡、加载等待、加载线程与高空高程跳过
- 新增设置项：Options → Visual → Planetary effects → Terrain self-shadowing；其余参数见 6.4

### 5. 性能优化

- 新增物理/渲染并行：加入了物理计算与渲染的并行执行，物理计算量大的场景帧率提升约 25%~30%
- 新增多核计算：加入了多飞船场景下互不影响飞船的并行更新
- 新增渲染优化：加入了白天星空与引力透镜的按需跳过，以及引力透镜结果的缓存复用

### 6. 新的接触求解器

- 新增接触求解方式：加入了隐式阻尼、全隐式两种接触求解方式（官方显式仍为默认）
- 新增设置页：加入了 Extra → Contact model 设置页，可选择求解方式并调整起落架/船身接触参数（刚度、阻尼比、摩擦系数；0 = 飞船原始值）
- 新增接触参数整定：加入了 DeltaGlider 起落架/船身接触参数重新整定

### 7. 残骸与撞击系统

- 新增损毁事件：加入了船间碰撞（相对速度超过阈值）、坠地/水面撞击、再入大气动压过高烧毁，以及相对论保护触发的直接损毁
- 新增残骸模型生成：加入了按飞船外形切分的真实船体残骸模型（最多 8 块）
- 新增损毁物理：加入了残骸的重力、碰撞与地形物理模拟；落地停稳后停止积分，约 30 s 后或静置约 10 s 后清理
- 新增设置项：Options → Vessel → "损伤与故障模拟"（残骸开关）；碰撞阈值等参数见 6.4

### 8. 参数整定

- 新增自动驾驶参数整定：加入了 DeltaGlider AAP 速度/高度/坡度/航向保持控制参数整定
- 新增悬停高度保持参数整定：加入了 HOLD ALT 悬停高度保持参数整定
- 新增热控参数整定：加入了飞船热控系统各项参数调整，使各舱温度合理化

### 9. 热控系统优化

- 新增真实空船质量模型：加入了按真实空船质量推导各舱结构等效热质量；修复了油箱/座舱质量为零导致的温度异常
- 新增混合显式/半隐式时间积分：加入了大步长（高时间加速）下的半隐式温度解算，提升求解稳定性
- 修复太阳辐照/阴影方向计算：加入了按实际太阳位置计算辐照与阴影方向；修复了太阳位置假设错误导致的光照/阴影异常
- 修复冷却回路连接：修正了右翼热交换器接入的回路
- 新增温度下限与温度钳制：加入了宇宙微波背景温度下限与温度钳制，消除负温度与 NaN
- 新增数值失败上报：加入了热控数值异常记录与上报

---

## 四、官方 Bug 修复

- 修复了官方 TransX MFD 打开即崩溃的 bug（太阳系初始化时父天体指针未初始化）
- 修复了官方 TransX 近点纬度显示 NaN 的 bug
- 修复了官方 TransX 发射方位角无解时仍输出错误航向的 bug
- 修复了官方 TransX 栈缓冲区溢出的 bug
- 修复了官方场景编辑器天体下拉框排序后参考天体选错的 bug（按未排序索引设置选中项）
- 修复了官方基地建筑在高分辨率非平地地形下悬空的 bug（按占地范围内最低地形点放置，v1.3.0 起）
- 同步了官方上游的其他 bug 修复（ImGui 视口、距离显示等）

---

## 五、版本历史

| 版本 | 主要改动 |
|---|---|
| v0.9 ~ v1.0.6 | 汉化框架、核心界面/MFD/场景编辑器/载具汉化、TransX 修复 |
| v1.0.7 | 新增 D3D9ClientDXVK（DXVK/Vulkan）渲染引擎变体 |
| v1.1 | 玻璃座舱面板/HUD 汉化、汉化范围补全、开发者 API |
| v1.2.0 | 相对论物理扩展（1PN/Kerr、参考系拖曳、相对论质量、时间膨胀、超光速保护） |
| v1.2.1 | 相对论视效（多普勒、光行差、引力透镜） |
| v1.2.2 | 汉化补全、牛顿引力模型、调试选项调整 |
| v1.2.3 | 引力透镜效果完善、中子星蓝白渲染、光行差修正 |
| v1.2.4 | 天空盒引力透镜衔接完善 |
| v1.2.5 | 电荷引力效应（Reissner–Nordström）、与官方 Orbiter 2024 最新版本同步 |
| v1.3.0 | 残骸与撞击系统（撞击销毁、残骸消息/相机、大气烧毁）、基地建筑贴地、动态强场判据、翻译补全 |
| v1.3.1 | 排版数据外置（排版可编辑、无需重新编译）、翻译补全 |
| v1.3.2 | 地形渲染优化（地形自投影、地形预加载、加载与切换优化）；性能优化（物理/渲染并行、多核计算、渲染优化）；新的接触求解器；残骸与撞击系统（损毁事件、残骸模型生成、损毁物理）；参数整定（自动驾驶、悬停高度保持、热控参数）；热控系统优化（质量模型、时间积分、数值稳定性）；中文排版完善；国际化 API |

---

## 六、开发者相关

### 1. 构建

**环境要求**：Visual Studio 2022（MSVC 14.44+）、DirectX SDK（June 2010）、CMake 3.31+

```powershell
# 配置（VS 2022 生成器，x64）
cmake -S . -B build64 -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_INSTALL_PREFIX="<源码目录>/install64" `
  -DDXSDK_DIR="D:/Microsoft DirectX SDK (June 2010)"

# Release 构建
cmake --build build64 --config Release --parallel 6

# 安装到 install64
cmake --install build64 --config Release
```

> **注意**：D3D9Client 的 shader 是**运行时从安装目录 `Modules/D3D9Client/*.hlsl` 加载**的。
> 修改 `OVP/D3D9Client/shaders/` 下的 shader 后，必须复制到安装目录的 `Modules/D3D9Client/` 才会生效。

### 2. 代码组织（官方文件钩子化）

本扩展的代码集中在独立 `.inc`/资源文件，官方源码只保留 `#include` 与少量条件调用：

- **相对论**：
  - 物理：`Src/Orbiter/RelativityPhysics.inc`
  - 配置：`Src/Orbiter/RelativityConfig.inc`（主程序）、`OVP/D3D9Client/RelativityConfig.inc`（渲染器）
  - 渲染：`OVP/D3D9Client/Relativity.inc`
  - UI：`Src/Orbiter/RelativityUi.cpp` / `RelativityUi.h` / `OrbiterRelativity.rc` / `RelativityResources.h`
  - 共享物理库：`OVP/D3D9Client/shaders/RelativityMapping.hlsl`（光行差、引力光行差、参考系拖曳、透镜偏折的单一实现来源）
- **汉化**：
  - 核心实现：`i18n/core/`（Localization、MfdZh/InstrumentZh/ZhCommon 布局算法、`DlgLayoutApi.inc` 布局/对话框 API 实现、`OptionsVisualZh.inc`）
  - 数据：`i18n/locales/zh_CN/*.po`（运行时加载）、`i18n/layout/zh_CN/layout.ini`（排版数值）
  - 各模块：`i18n/d3d9client/`、`i18n/scneditor/`、`i18n/vessels/`、`i18n/plugins/`
  - SDK 声明：`Orbitersdk/include/I18nAPI.h`（布局/对话框 API）
- **地形**：`OVP/D3D9Client/TerrainRender.inc`（按 `TERRAINRENDER_SURF/TILES/LOADWAIT/HORIZON` 分段挂入官方 `Surfmgr2.cpp`/`Tilemgr2.cpp`/`D3D9Client.cpp`）、`TerrainRenderMembers_D3D9Config.inc`（配置成员）、`shaders/NewPlanet.hlsl`（地形自投影蒙版段）
- **性能**：`Src/Orbiter/ParallelUpdate.inc` + `ParallelUpdateConfig.inc`（多核逐船计算）、`PhysicsThread.inc`（物理线程 + 主线程渲染）、`VesselUpdate.inc`、`OVP/D3D9Client/SceneRender2D.inc`、`PerfStats.h` / `PerfStats.inc`（帧性能分析）
- **接触模型**：`Src/Orbiter/ContactConfig.inc`（配置）、`TabExtraContact.inc`（启动器页面）、`ContactDialogRes.rc` / `ContactDialogLang.rc` / `ContactResources.h`（对话框资源）
- **残骸**：`Src/Wreck/WreckDebris.inc`（生成/物理/清理）、`WreckApi.inc`、`WreckConfig.inc`、`WreckDebris.h`
- **热控（DeltaGlider）**：`Src/Vessel/DeltaGlider/ThermalModel.inc` + `ThermalModelMembers_CoolantLoop.inc` / `ThermalModelMembers_ThermalSubsystem.inc`
- **类成员声明集中块**：`*Members_*.inc`（如 `OrbiterMembers_Orbiter.inc`、`VesselMembers_Vessel.inc`、`ClientMembers_D3D9Client.inc`、`SceneMembers_Scene.inc`、`ProjectConfigMembers_*.inc` 等）——新增成员声明集中于此，官方头文件仅增加一行 include
- **工具**：`tools/smoke_test.ps1`（自动冒烟测试）、`tools/run_game.cmd`（命令行启动本地构建）

> 官方源码保持"只增钩子"的最小改动原则：热控、地形、i18n 布局、接触对话框资源与类成员声明均放在独立文件中；官方阴影逻辑保持原样，地形自投影为独立叠加功能。

### 3. 开发者接口

| API | 说明 |
|---|---|
| `const char *oapiTr(const char *english)` | 查询翻译表：中文模式返回译文（UTF-8），英文模式返回原文 |
| `const char *oapiTrContext(const char *context, const char *english)` | 带上下文的翻译查询（同一英文在不同上下文有不同译文时使用） |
| `const char *oapiTrName(const char *english)` | 名称类翻译（天体名等，带规范化） |
| `int oapiZhActive()` | 当前是否为中文模式（1/0） |
| `void oapiZhHudText(int x, int y, int size, const char *utf8)` | 登记玻璃座舱 HUD 中文文字（中文模式生效） |
| `int oapiGetLayoutInt/Float/String(section, key, def)` | 查询 `i18n/layout/zh_CN/layout.ini` 中的排版数据（英文模式返回默认值） |
| `oapiZhFitControlText/Row/Height`、`oapiZhOffsetControls/Window`、`oapiZhLayoutDialogButtons`、`oapiZhFitDialogButtons` | 中文对话框布局工具：按中文文本自适应控件宽高、按钮行重排、整体偏移（"官方布局 + 偏置"规则，英文模式不改变布局） |
| `bool oapiGetRelativitySpin(OBJHANDLE hObj, double *aM, VECTOR3 *axis)` | 查询天体的相对论自旋参数（a/M 与自旋轴） |
| `double oapiGetRelativityTimeDilation()` | 焦点船处的相对论时间膨胀（总/引力/速度） |
| `oapiGetRelativityGravTimeFactor()` / `oapiGetRelativityVelTimeFactor()` | 引力/速度时间膨胀因子 |
| `bool oapiGetWreckMessageEx(char *buf, int n, double *age_ms, int *isFocus, char *ship, int shipN)` | 获取当前残骸消息（UTF-8，含是否焦点船与船名） |
| `OBJHANDLE oapiGetWreckAnchorVessel()` | 残骸观察锚点（保留接口；当前版本锚点机制未启用，恒返回 0） |
| `void oapiWreckReportNumericFailure(OBJHANDLE hVessel, double maxTemp, int compartment)` | 向损毁系统上报数值失败（供船模块使用；当前仅排队/记录日志，损毁策略为预留接口，不直接删船） |

**配置参数（SDK 宏/CFGPRM）**

| 头文件 | 内容 |
|---|---|
| `Orbitersdk/include/GraphicsAPI.h` | 新增 `CFGPRM_TERRAINHORIZONSHADOW`（地形自投影总开关，bool） |
| `Orbitersdk/include/ContactAPI.h` | `CFGPRM_CONTACTSOLVER`（接触求解方式）；原实验摩擦模型 `CFGPRM_CONTACTFRICTION` 已删除 |
| `Orbitersdk/include/WreckAPI.h` | `CFGPRM_WRECKS` / `CFGPRM_RELCOLLISIONDV` / `CFGPRM_RELGASGIANT` |
| `Orbitersdk/include/RelativityAPI.h` | `CFGPRM_REL*` 相对论参数开关（不变） |
| `Orbitersdk/include/I18nAPI.h` | 布局/对话框 API 声明（从 `OrbiterAPI.h` 移入的新公开头） |

**已删除/变更的 API（破坏性变更）**

- 旧 2D 碎片渲染 API：`oapiGetWreckDebrisCount` / `oapiGetWreckDebris`（残骸改真实船体后不再需要）；
- 旧场景阴影模式 API：`oapiSetVesselShadowMode` / `oapiGetVesselShadowMode` 及 `SHADOWMODE_*`（已删除，官方阴影路径保持原样）；
- `oapiGetLayout*` / `oapiZhFit*` / `oapiZhOffset*` 等布局 API 声明迁至 `I18nAPI.h`（函数本身不变）。

**外部 MFD 汉化**：中文模式下按钮统一由主程序字体渲染，插件只需在中文模式返回中文标签（`ButtonLabel()` 中用 `oapiZhActive()` 判断），参考 `i18n/plugins/TransXZh.inc` 等。

**Lua**：`oapi.get_time_dilation()` 等相对论接口（不变）。

**双线程/多核开关**：`Src/Orbiter/Orbiter.cpp` 顶部宏 `DUAL_THREAD_EXPERIMENT`（1 = 物理线程 + 主线程渲染（默认）、0 = 官方单线程循环，需重新编译）；多核逐船计算用 `VesselParallelUpdate` 配置项控制。

**验证流程**：构建 → 安装 → 启动游戏验证中文模式各界面 → 切换到 English 回归 → 再切回中文（设备重建）确认无崩溃；相对论场景验证视效；地表场景验证地形自投影与接触模型。

### 4. 配置参数（开发者）

本节列出**无图形界面**的配置项（配置文件位于游戏根目录）。生效方式同 2.4：需**重启启动器**，重启模拟场景不会生效；应在 Orbiter 关闭后再修改（退出时会回写）。

**Orbiter.cfg（主程序）**

| 键 | 默认 | 范围 | 说明 |
|---|---|---|---|
| `VesselParallelUpdate` | TRUE | TRUE / FALSE | 多核计算开关：多飞船场景下并行更新互不影响的飞船 |
| `VesselParallelRange` | 50000 | 1000 ~ 1e7 | 与其它飞船距离小于该值 [m] 的飞船改为顺序更新（可能发生交互） |
| `Wrecks` | TRUE | TRUE / FALSE | 残骸系统附加开关（需 `DamageModel` 开启） |
| `RelativityCollisionDVThreshold` | 10.0 | 1 ~ 1e6 | 飞船间碰撞损毁的相对速度阈值 [m/s] |
| `RelativityGasGiantFactor` | 0.8 | 0.1 ~ 1.0 | 气态巨行星压毁半径系数 |
| `RelativityChargeRatio` | 0.5 | 0 ~ 1 | 电荷比 q = r_Q/r_s |
| `RelativityGravRedshift` | TRUE | TRUE / FALSE | 背景天空引力红移（Options → Visual 中随多普勒开关联动） |

**D3D9Client.cfg（渲染器）— 地形自投影**

| 键 | 默认 | 范围 | 说明 |
|---|---|---|---|
| `TerrainHorizonShadow` | 1 | 0 / 1 | 渲染器侧开关（与 `Orbiter.cfg` 总开关同时开启才生效） |
| `TerrainHorizonFieldSize` | 32 | 16 ~ 128 | 阴影精细度；越高越精细、开销越大 |
| `TerrainHorizonDirs` | 16 | 8 ~ 32（偶数） | 太阳方位插值的平滑度；越高越平滑、开销越大 |
| `TerrainHorizonRangeKm` | 300 | 50 ~ 500 | 最大阴影搜索距离 [km] |
| `TerrainHorizonStrength` | 75 | 0 ~ 100 | 阴影强度 [%] |
| `TerrainHorizonThreads` | 1 | 1 ~ 3 | 阴影计算线程数 |
| `TerrainHorizonCacheMB` | 64 | 8 ~ 512 | 阴影数据缓存预算 [MB] |
| `TerrainHorizonMaxAlt` | 50000 | 0 ~ 1e6 | 相机高度高于该值时关闭自投影 [m] |
| `TerrainHorizonMinLevel` | 4 | 0 ~ 20 | 只对达到该细节层级或更细的地形生效 |
| `TerrainHorizonUpdate` | 4 | 1 ~ 16 | 阴影更新检查间隔 [帧] |
| `TerrainHorizonDebug` | 0 | 0 ~ 3 | 调试视图（0 = 正常阴影；1/2/3 语义见 6.6） |
| `TerrainHorizonContour` | 200 | 0 ~ 5000 | 调试视图等高线间距 [m]（0 = 关闭） |
| `TerrainHorizonShadowFloor` | 40 | 0 ~ 90 | 阴影内保留的最低亮度 [%]（近似天空/环境光） |
| `TerrainHorizonSoftness` | 27 | 1 ~ 300 | 软阴影边缘宽度（0.01 度单位）；27 = 0.27°，即太阳圆盘半径 |

**D3D9Client.cfg（渲染器）— 地形加载 / 细节过渡**

| 键 | 默认 | 范围 | 说明 |
|---|---|---|---|
| `WaitTerrainLoad` | 1 | 0 / 1 | 进入模拟时等待初始地形加载完成 |
| `TerrainLoadWaitTimeout` | 30 | 1 ~ 120 | 地形加载等待上限 [s] |
| `TerrainLoadWaitMinMs` | 600 | 0 ~ 5000 | 地形加载等待最短时长 [ms]（瓦片已缓存时仍保留该等待） |
| `WaitTerrainLoadCamera` | 1 | 0 / 1 | 相机目标切换时重新执行地形加载等待 |
| `TerrainPreloadLevels` | 1 | 0 ~ 2 | 提前预加载的细节层级数 |
| `TerrainTransitionMs` | 0 | 0 ~ 2000 | 地形细节切换过渡时长 [ms]；0 = 关闭（默认），建议 200 ~ 350 获得平滑切换 |
| `TerrainElevMinLevel` | 10 | 0 ~ 20 | 高程跳过：相机高度超过 `TerrainElevSkipAlt` 时，低于该细节层级的瓦片不再读取高程文件 |
| `TerrainElevSkipAlt` | 50000 | 0 ~ 1e6 | 高程跳过的相机高度阈值 [m]（0 = 总是生效） |
| `TerrainLoadThreads` | 1 | 1 ~ 4 | 地形瓦片加载线程数 |

**D3D9Client.cfg（渲染器）— 相对论视觉 / 引力透镜**

| 键 | 默认 | 范围 | 说明 |
|---|---|---|---|
| `LensCubemapSize` | 1024 | 512 / 1024 / 2048 | 引力透镜效果分辨率（越高边界越平滑，显存与开销也越高） |
| `LensBlendStart` | 0.2 | 0 ~ 45 | 低于此偏移角 [度] 用原版清晰星空 |
| `LensBlendEnd` | 1.0 | 0 ~ 90 | 高于此偏移角 [度] 用透镜效果 |
| `LensBrightness` | 1.0 | -10 ~ 4 | 透镜效果亮度增益（负值为调试模式，见 6.6） |
| `RelativityRenderCache` | 1 | 0 / 1 | 透镜渲染缓存：参数不变时跳过重绘；强场/高速时自动禁用 |

**i18n/layout/zh_CN/layout.ini（中文排版数据）**

中文模式的排版数值（控件加宽/加高、偏移、字号、按钮间距等）在运行时读取（UTF-8 INI），修改后无需重新编译，切换一次语言即可生效；缺失的段/键自动回退到内置默认值，英文模式不读取该文件。

| 段 | 内容 |
|---|---|
| `[mfd.*]` / `[panel.*]` / `[hud.*]` | MFD、玻璃座舱面板、HUD 的中文字号/坐标偏置 |
| `[dialog.*]` | Win32 对话框控件偏置、按钮重排参数 |
| `[dialog.visual]`（v1.3.2 新增） | 启动器 Options → Visual 页中文布局偏置：`groupboxExtraWidth`（分组框加宽，默认 12 DLU）、`planetaryExtraHeight`（行星效果分组框加高，默认 8 DLU）、`lowerBlockYOffset`（下方内容整体下移，默认 9 DLU） |

**实现细节备注**

- 接触模型的阻尼比 ζ 在运行时换算为阻尼系数 `c = 2ζ√(k·m)`（k 为有效刚度，m 为飞船质量）；求解方式 1/2 分别将阻尼/弹簧力放到步末计算，以提高大步长下的稳定性。
- 配置参数通过 `CFGPRM_*` 宏暴露给图形客户端（`Orbitersdk/include/*API.h`）：新增图形/物理参数时，同时更新对应 API 头与两侧配置读写（主程序 `Config`、渲染器 `D3D9Config`/`RelativityConfig.inc`），并保持"官方文件最小化"（配置读写实现放到独立 `.inc`）。
- 调试/诊断相关配置（地形自投影调试视图、引力透镜调试、帧性能日志等）见 6.6。

### 5. 自动化测试

- `tools/smoke_test.ps1`：以 `CreateProcess` 启动指定场景（默认 `Delta-glider\DG and DG-S`，可用 `-Scenario` 指定；绕过 Windows 未签名 exe 的安全提示），轮询日志并带超时/卡死看门狗，自动截图并判定 PASS/FAIL（约 25 s）；
- `tools/run_game.cmd`：命令行启动本地构建的 `Orbiter.exe`。

### 6. 调试视图与性能日志

**地形自投影调试视图**（`D3D9Client.cfg`）：

| 键 | 默认 | 说明 |
|---|---|---|
| `TerrainHorizonDebug` | 0 | 0 = 正常阴影；1 = 太阳可见度红（阴影）/黄（受光）+ 等高线；2 = 地平线角场 + 等高线；3 = 正常着色 + 等高线。1/2 为覆盖式调试视图，正式游玩请保持 0 |
| `TerrainHorizonContour` | 200 | 调试视图等高线间距 [m]（0 = 关闭） |

**引力透镜调试**：`LensBrightness` 取负值：`-1` 原始 cubemap、`-2` 挖洞权重图、`-3` 挖洞边界绿线；另外 `Extra → 调试渲染选项` 中可开透镜线框、相对论天体位置标记。

**帧性能日志**（`D3D9Client.cfg`）：

| 键 | 默认 | 说明 |
|---|---|---|
| `PerfLog` | 0 | 0 = 关闭（零开销）；1 = 输出 CPU 各渲染阶段耗时到 `Orbiter.log`（每 2 秒一行 `PERF:`）；2 = 额外做 GPU 计时（会阻塞管线，被分析帧的帧率不具代表性）；3 目前与 2 等价 |

---

## 七、协议和贡献者

### 许可

- Orbiter 本体遵循原项目 [MIT 许可](https://github.com/orbitersim/orbiter/blob/main/LICENSE)
- D3D9Client 图形引擎遵循 LGPL（见 [LGPL](./OVP/D3D9Client/LGPL.txt)）

### 贡献者

- 汉化、相对论扩展与测试：[@orbiter-newcomer](https://github.com/orbiter-newcomer)
- AI 辅助开发：DeepSeek（翻译、代码修改、缺陷排查、文档）
