![Orbiter logo](./Src/Orbiter/Bitmaps/banner.png)

# Orbiter 中文汉化 + 相对论扩展

## 一、简介

本仓库是 [Orbiter](https://github.com/orbitersim/orbiter)（开源航天飞行模拟器）的 **64 位分支**，在官方代码基础上提供：

- **全面中文汉化**：界面 / MFD / 场景编辑器 / 载具 / 玻璃座舱 HUD / 帮助文本；英文模式与官方原版完全一致
- **相对论物理与视觉扩展**：1PN/Kerr 引力场、参考系拖曳、相对论质量、时间膨胀、多普勒红蓝移、光行差、引力透镜（挖洞法）、引力红移、黑洞/中子星渲染、超光速与强场保护、电荷引力效应

- 仓库：https://github.com/orbiter-newcomer/orbiter-i18n
- 当前版本：**v1.2.5**（与官方 Orbiter 2024 最新上游版本同步）
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

**英文版说明**：英文模式与官方原版完全一致——所有汉化改动仅在中文模式生效，英文模式下界面、MFD、文本均保持官方原始行为。

### 3. 相对论设置

1. **Options → Physics**：相对论物理**总开关**（关闭后所有相对论物理与视效都不生效）；
2. **Options → Visual**：相对论视觉开关——多普勒效应、光行差、引力透镜、引力红移等；
3. **Extra → Relativity effects**：相对论引力模型三选一（**牛顿 / 1PN / Kerr**）、开关电荷引力效应；
4. **Extra → 调试渲染选项**：透镜线框、天体位置标记等调试功能；
5. 内置相对论测试场景：`Scenarios\Relativity\` 下的**黑洞 / 超大质量黑洞 / 中子星**场景。

### 4. 引力透镜（挖洞法）参数

`D3D9Client.cfg`（安装目录根下）：

| 配置项 | 含义 | 默认 |
|---|---|---|
| `LensCubemapSize` | 透镜 cubemap 分辨率（512/1024/2048） | 2048 |
| `LensBlendStart` | 低于此偏移角 [度] 用原版星空 | 0.2 |
| `LensBlendEnd` | 高于此偏移角 [度] 用透镜 cubemap | 1.0 |
| `LensBrightness` | 透镜 cubemap 亮度增益 | 1.0 |

调试模式（`LensBrightness`）：`-1` 原始 cubemap、`-2` 挖洞权重图、`-3` 挖洞边界绿线。

> 原版星空顶点位移限幅自动绑定挖洞参数（`min(2°, LensBlendEnd)`），调参不会产生网格拉伸伪影。

---

## 三、新特性

### 1. 相对论物理

1. **以全局仿真时间为基准的洛伦兹变换**，其中时间膨胀通过时间加速体现（速度洛伦兹因子 × 引力时间膨胀因子）；
2. **可选的相对论引力模型**（三选一）：**牛顿**（经典）、**1PN**（一阶后牛顿，弱场）、**Kerr**（精确强场，含参考系拖曳）；
3. **可选的电荷引力效应**：紧凑天体的 Reissner–Nordström 电荷修正（引力加速度排斥项、时间膨胀/红移修正），仅对相对论天体生效；
4. **新增相对论天体**：黑洞、中子星；
5. **超光速与强引力场保护**：受限速度处理、强场区域自动回退/保护，防止游戏崩溃与数值发散。

### 2. 相对论视觉

1. **多普勒效应**：速度与引力引起的红/蓝移、亮度修正；
2. **参考系效应**：速度和引力引起的观察者参考系效应（以下简称**速度光行差**和**引力光行差**）；
3. **引力引起的时空弯曲**：引力透镜"挖洞"法——大偏折区域用透镜 cubemap、小偏折区域用原版清晰星空（边界平滑）；
4. **电荷对引力视效的修正**：透镜偏折与引力光行差的电荷修正。

### 3. 中文汉化

- **核心界面**：Launchpad 全部页面、选项页、物体信息、MFD 菜单/按钮、菜单栏、Win32 对话框、主程序 ImGui 对话框
- **MFD**：内置 MFD（Orbit/Surface/Map/HSI/VOR/Docking/Align/Sync/Transfer/COM-NAV）+ 第三方 MFD（Terminal/Generic Camera/Ascent/AscentAP/Attitude RCS/TransX）
- **场景编辑器**：完整界面汉化
- **载具编辑页与配置**：DeltaGlider / ShuttleA / 启动器载具配置
- **2D 玻璃座舱面板与 HUD**：MFD 按钮、面板按钮（电源/选择/说明）、导航按钮、RCS 按钮、HUD 读数标签与中文标注
- **天文与标签**：88 星座中文名、天体名、39 个发射基地名、地物标签保持英文
- **字体**：Noto Sans CJK SC；运行时语言切换字体自动重建

### 4. 调试与开发者工具

- 挖洞调试（LensBrightness 负值模式）、透镜线框、天体位置标记
- 共享物理库 `OVP/D3D9Client/shaders/RelativityMapping.hlsl`（相对论方向映射单一来源）
- 开发者 API：`oapiTr` / `oapiZhActive` / `oapiZhHudText`、Lua 时间膨胀接口等（见"开发者相关"）

---

## 四、Bug 修复

### 1. 汉化与稳定性修复

| 组件 | 问题 | 修复 |
|---|---|---|
| TransX | **打开 MFD 即崩溃（0xc0000005）**：太阳系初始化时父天体指针未初始化 | 初始化指针、排除 NaN 距离、找不到父体时兜底挂到太阳 |
| TransX | 近点纬度显示 NaN | 改用位置向量模长归一化 |
| TransX | 发射方位角无解时输出无意义航向 | 标准公式 + 无解显示 N/A |
| TransX | 栈缓冲区溢出 / 中文 UTF-8 截断乱码 | 扩大缓冲区、规范格式化 |
| 地物数据 | 20 个 `.mkr` 文件 UTF-8 特殊字符乱码 | 规范化为 ASCII |
| D3D9Client | 语言运行时切换后 ImGui 字体乱码 | CJK 字体始终合并进字体图集 |
| 核心 | 中文文本经 ANSI API 显示乱码 | 统一改用 W API（UTF-8 → UTF-16） |
| 核心 / HUD | 切换语言或图形设备重建后 HUD 中文字体缓存失效 → 崩溃 | 设备销毁回调中同步释放字体缓存，按需重建 |
| D3D9Client / GenericCamera | DXVK 引擎变体下通用相机 MFD 打不开 | 依次尝试 `D3D9Client.dll` / `D3D9ClientDXVK.dll` |
| 面板按钮 | 中文按钮字号降档错误、RCS 按钮消失 | 修正降档策略；按钮坐标静态缓存 |

### 2. 相对论相关修复

- 引力透镜背向偏折 bug（body 背后方向出现虚假大偏折）
- 天空盒挖洞法边界接缝（原版星空引力光行差 + 小偏折透镜，与 cubemap 合成精确互逆）
- 挖洞边界硬切换接缝、阴影区过绘、原版银河被错误弯折
- 交接区原版网格拉伸伪影（新增顶点位移兜底判据 + 顶点限幅与挖洞参数绑定）
- 官方 bug 修复：ImGui 视口修复、距离显示改进等（随官方同步）

---

## 五、版本历史

| 版本 | 主要改动 |
|---|---|
| v0.9 ~ v1.0.6 | 汉化框架、核心界面/MFD/场景编辑器/载具汉化、乱码修复、TransX 崩溃修复、汉化代码重构（一/二期） |
| v1.0.7 | 新增 D3D9ClientDXVK（DXVK/Vulkan）渲染引擎变体 |
| v1.1 | 汉化架构三期重组（逻辑/数据分离）、玻璃座舱面板/HUD 汉化、开发者 API |
| v1.2.0 | 相对论物理扩展（1PN/Kerr、参考系拖曳、相对论质量、时间膨胀、超光速保护） |
| v1.2.1 | 新增相对论视效（多普勒、光行差、引力透镜） |
| v1.2.2 | 相对论代码钩子化重构、汉化补全修复、牛顿模型、调试选项移位 |
| v1.2.3 | 引力透镜"挖洞"混合、中子星蓝白渲染、引力光行差/背向偏折修复 |
| v1.2.4 | 天空盒挖洞法完善（边界接缝修复 + 挖洞调试工具） |
| v1.2.5 | 电荷引力效应（Reissner-Nordström）、顶点限幅与挖洞参数绑定、顶点位移兜底判据、与官方 2024 最新版本同步 |

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

- 本扩展所有相对论/汉化代码集中在独立 `.inc` 文件，官方源码只保留 `#include` 与少量条件调用：
  - 相对论物理：`Src/Orbiter/RelativityPhysics.inc`
  - 相对论配置：`Src/Orbiter/RelativityConfig.inc`（主程序）、`OVP/D3D9Client/RelativityConfig.inc`（渲染器）
  - 相对论渲染：`OVP/D3D9Client/Relativity.inc`
  - 相对论 UI：`Src/Orbiter/RelativityUi.cpp` / `RelativityUi.h` / `OrbiterRelativity.rc` / `RelativityResources.h`
  - 汉化逻辑：`Src/Orbiter/ZhCommon.inc`；数据：`MfdZh.inc` / `InstrumentZh.inc` / `DialogZh.inc` / `LaunchpadZh.inc` 等
- **共享物理库**：`OVP/D3D9Client/shaders/RelativityMapping.hlsl`——光行差、引力光行差、参考系拖曳、引力透镜偏折的单一实现来源；顶点路径与 cubemap 合成路径都调用它，改物理只改这一个文件
- **渲染设置**：相对论视觉开关通过 `CFGPRM_REL*`（`Orbitersdk/include/RelativityAPI.h`）由主程序传给渲染器；物理与视觉共用同一开关和数据

### 3. 开发者接口

| API | 说明 |
|---|---|
| `const char *oapiTr(const char *english)` | 查询翻译表：中文模式返回译文（UTF-8），英文模式返回原文 |
| `int oapiZhActive()` | 当前是否为中文模式（1/0） |
| `void oapiZhHudText(int x, int y, int size, const char *utf8)` | 登记玻璃座舱 HUD 中文文字（中文模式生效） |
| `bool oapiGetRelativitySpin(OBJHANDLE hObj, double *aM, VECTOR3 *axis)` | 查询天体的相对论自旋参数（a/M 与自旋轴） |
| `double oapiGetRelativityTimeDilation()` | 焦点船处的相对论时间膨胀（总/引力/速度） |
| `oapiGetRelativityGravTimeFactor()` / `oapiGetRelativityVelTimeFactor()` | 引力/速度时间膨胀因子 |

- **外部 MFD 汉化**：中文模式下按钮统一由主程序字体渲染，插件只需在中文模式返回中文标签（`ButtonLabel()` 中用 `oapiZhActive()` 判断），参考 `Src/Plugin/TransX/TransXZh.inc` 等；
- **Lua**：`oapi.get_time_dilation()` 等相对论接口；
- **验证流程**：构建 → 安装 → 启动游戏验证 中文模式各界面 → 切换到 English 回归 → 再切回中文（设备重建）确认无崩溃。

### 4. 相对论配置项（开发者）

- 物理：`Orbiter.cfg` 的 `Relativity` / `RelativityGravityMethod` / `RelativityCharge` / `RelativityChargeRatio` 等；
- 视觉：`D3D9Client.cfg` 的 `LensCubemapSize` / `LensBlendStart` / `LensBlendEnd` / `LensBrightness` 等；
- 配置参数通过 `CFGPRM_REL*` 暴露给图形客户端，扩展新效应时同时更新 `RelativityAPI.h` 与两侧配置读写。

---

## 七、协议和贡献者

### 许可

- Orbiter 本体遵循原项目 [MIT 许可](https://github.com/orbitersim/orbiter/blob/main/LICENSE)
- D3D9Client 图形引擎遵循 LGPL（见 [LGPL](./OVP/D3D9Client/LGPL.txt)）

### 贡献者

- 汉化、相对论扩展与测试：[@orbiter-newcomer](https://github.com/orbiter-newcomer)
- AI 辅助开发：DeepSeek（翻译、代码修改、缺陷排查）
