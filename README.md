![Orbiter logo](./Src/Orbiter/Bitmaps/banner.png)

# Orbiter 中文汉化 + 相对论扩展

本仓库是 [Orbiter Space Flight Simulator](https://github.com/orbitersim/orbiter)（开源航天飞行模拟器）的 **64 位分支**，在官方最新代码基础上提供：

1. **全面中文汉化**（界面 / MFD / 场景编辑器 / 帮助文本；英文模式与官方原版完全一致）
2. **相对论扩展**（v1.2 系列）：1PN / Kerr 引力场、参考系拖曳、相对论质量修正、时间膨胀、引力透镜、多普勒 / 光行差 / 引力红移、黑洞与中子星渲染、超光速与强场保护
3. 官方代码修复与渲染 / 窗口 / ImGui 修复

- 汉化与相对论扩展仓库：https://github.com/orbiter-newcomer/orbiter-i18n
- 当前版本：**v1.2.4**

## 相对论扩展

### 物理

- **引力场模型**：1PN（一阶后牛顿）与 Kerr（强场/自旋）引力求解器，含参考系拖曳
- **相对论质量修正**、**速度时间膨胀**、**引力时间膨胀**（时间加速与物理积分一致）
- **视角锁定时间加速**（观察者所在参考系的时间膨胀倍率）
- **超光速保护（FTL guard）**、**强场保护**（进入 10 rs 强场区域自动回退牛顿解 / 删除受影响船只，避免数值爆炸）
- 牛顿模型（关闭相对论视觉，仅保留经典物理）

### 视觉

- **相对论多普勒效应**（红/蓝移、亮度修正）
- **光行差**：速度光行差（背景星空 / 天体）+ 引力光行差（FrameInverse，静止观察者处于势井中）
- **引力透镜（"挖洞"法）**：
  - 扭曲大的区域用透镜 cubemap 渲染（`LensCubemapSize` 可配 512/1024/2048）
  - 扭曲小的区域用原版清晰星空（含小偏折透镜 + 光行差，边界平滑无接缝）
  - 阈值 / 亮度可配：`LensBlendStart`、`LensBlendEnd`、`LensBrightness`
- **引力红移**
- **黑洞 / 中子星渲染**：紧凑天体发光球（黑洞橙红、中子星蓝白）
- **相对论时间膨胀显示**与 Lua 接口

### 配置与调试

`D3D9Client.cfg` 中与透镜相关的配置项：

| 配置项 | 含义 | 默认 |
|---|---|---|
| `LensCubemapSize` | 透镜 cubemap 分辨率（512/1024/2048） | 1024 |
| `LensBlendStart` | 低于此偏移角 [度] 用原版星空 | 0.2 |
| `LensBlendEnd` | 高于此偏移角 [度] 用透镜 cubemap | 1.0 |
| `LensBrightness` | 透镜 cubemap 亮度增益 | 1.0 |
| `LensBrightness = -1` | 调试：全屏显示原始 cubemap | |
| `LensBrightness = -2` | 调试：挖洞权重图（红=洞内） | |
| `LensBrightness = -3` | 调试：正常画面 + 绿色挖洞边界线 | |

启动器 **Extra → 调试渲染选项** 中可开关相对论调试功能（透镜线框、天体位置标记等）。

### 已知限制

- 天体引力透镜只做**位置偏移 / 星空透镜**，不做天体图像的多重像 / 爱因斯坦环形变（远景下天体本身近似点，不展开图像扭曲）
- 观察者在极强场中时，为清晰度优先，部分引力效应由透镜 cubemap 承担（分辨率有限）
- 测试场景 / 自定义天体配置属于安装目录运行时数据，不包含在本仓库中

## 构建

需求：

- Windows 10/11 x64
- Visual Studio 2022（含 C++ 桌面开发）
- CMake（VS 自带即可）
- DirectX SDK（编译 D3D9Client shader 用，可选）

```powershell
# 在仓库根目录
cmake -S . -B build64 -G "Visual Studio 17 2022" -A x64
cmake --build build64 --config Release --parallel 1
cmake --install build64 --config Release
```

> 注意：D3D9Client 的 shader 是**运行时从安装目录 `Modules/D3D9Client/*.hlsl` 加载**的。
> 如果修改了 `OVP/D3D9Client/shaders/` 下的 shader，需要把改动后的 shader 复制到安装目录的 `Modules/D3D9Client/` 下才会生效。

## 运行

1. 使用官方 Orbiter 2024 安装（需要行星纹理等资源），或使用本项目的 `install64` 安装目录；
2. 将构建产物（`Orbiter.exe`、`Modules/Plugin/D3D9Client.dll` 等）覆盖到安装目录；
3. 启动 `Orbiter.exe` 进入 Launchpad，选择场景开始。

语言切换：Launchpad → **Options → UI → Language** 选择中文（简体）即可，或在 `Localization.cfg` 中设置 `Language=zh` / `Language=en`。

## 版本历史

| 版本 | 主要改动 |
|---|---|
| v1.2.0 | 相对论物理扩展：1PN/Kerr 引力场、参考系拖曳、相对论质量修正、视角锁定时间加速、超光速保护、相对论测试工具 |
| v1.2.1 | 相对论物理/视觉修复 + 启动器模块页修复 |
| v1.2.2 | 相对论代码 10 文件重构（官方文件钩子化）+ 汉化补全修复 + 引力模型新增牛顿模型 |
| v1.2.3 | 天体引力透镜"挖洞"混合 + 中子星蓝白渲染 + 引力光行差修复 + ImGui 视口修复 |
| v1.2.4 | 天空盒挖洞法完善：洞边界接缝修复（原版星空引力光行差 + 小偏折透镜）+ 挖洞调试工具 |

更早的汉化版本历史见 [README_ZH.md](./README_ZH.md)。

## 许可证

- Orbiter 本体遵循 [MIT 许可证](./LICENSE)
- D3D9Client 图形引擎遵循 LGPL（见 [LGPL](./OVP/D3D9Client/LGPL.txt)）

## 相关链接

- 官方 Orbiter：https://github.com/orbitersim/orbiter
- 中文汉化详细说明：[README_ZH.md](./README_ZH.md)
- English readme: [README_EN.md](./README_EN.md)
