# Orbiter 相对论视觉 —— 交接文档 (HANDOVER)

日期: 2026-11-12
基线与版本: v1.1.1 (commit d9f069f7) + 相对论视觉 v1.2
备份: `D:\AI_projet\orbiter\backup\v1.2-relvis\`（changes.patch / newfiles / config / README_zh.md）

---

## 0. 任务来源

用户要求为 Orbiter 2024 fork 的**相对论物理**（上一条鱼已完成物理侧：1PN + Kerr 测地线积分）
补上**相对论视觉特效**。用户的硬性约定：

- **严格范围纪律**：没让干的不要干
- 代码**尽量单独剥离**（新逻辑集中在 `Relativity.inc` / `Relativity.hlsl` / `StarsLens.hlsl`，
  官方文件只保留调用点）
- **不改正确的物理**；发现用户的物理错误要大胆指出
- **不要擅自关游戏**（每次测试结束先问）
- 回复用**中文**
- 每次改完要能跑起来，诊断优先用可视化（线框/方向编码）而不是猜

---

## 1. 当前完成状态

| 效果 | 状态 | 实现位置 |
|---|---|---|
| **Doppler 红蓝移 + 曝光适应** | ✅ 完成 | `shaders/Relativity.hlsl`（后处理） |
| **光行差（星空）** | ✅ 完成 | cubemap 源系渲染 + 合成 pass `AberrateInverse` |
| **光行差（天体位置）** | ✅ 完成 | `Scene::ApplyRelativityToBodies` + `vObject::RepositionFromCamera` |
| **引力透镜（星空）** | ✅ 完成 | cubemap 源系渲染 + `StarsLens.hlsl` 反解采样 |
| **强弱场模型** | ✅ 完成 | 弱场 PN 展开 + Bozza 强场对数项 + smoothstep 混合 |
| **Kerr 自旋修正（一阶）** | ✅ 完成 | 主程序 `oapiGetRelativitySpin` + shader 一阶项 |
| **近场保护** | ✅ 完成 | `RelNearGuard`（默认 500 m） |
| **太阳眩光相对论修正** | ✅ 完成 | `ComputeRelSunVisual` + VisibilityPS |
| **设置 UI** | ✅ 完成 | 主程序 Options → 视觉设置组（3 复选框 + 曝光滑条） |
| **线框调试选项** | ✅ 完成 | 启动器 Extra 页 → Relativity → Debug 分组 |

**渲染管线（星空）**：
```
每帧:
  RenderLensCubemap()            // 6 个 90° 面，源系星空（无光行差/透镜），异步瓦片
  m_celSphere->Render(..., bOverlaysOnly=true)   // 主画面只画网格/标签/天球点
  CompositeStarField()           // 全屏 quad：屏幕方向 -> AberrateInverse -> LensInverse -> texCUBE -> 加色混合
```

**关键顺序**：背景渲染（cubemap）必须在合成之前；主画面背景层要跳过（否则覆盖合成结果）。

---

## 2. 文件清单

### 新增文件（未跟踪，需备份）

| 文件 | 说明 |
|---|---|
| `OVP/D3D9Client/Relativity.inc` | 主逻辑：Doppler/光行差/透镜/cubemap/合成/自旋数据收集 |
| `OVP/D3D9Client/shaders/Relativity.hlsl` | Doppler 后处理（红蓝移 + 曝光） |
| `OVP/D3D9Client/shaders/StarsLens.hlsl` | 透镜合成：光行差反解 + 透镜反解 + cubemap 采样 |
| `install64/Orbiter/Config/BlackHole.cfg` | 10 太阳质量黑洞测试天体（SPICE SUN 位置） |
| `install64/Orbiter/Config/BlackHoleSystem.cfg` | 黑洞测试星系 |
| `install64/Orbiter/Scenarios/BlackHole_Test.scn` | 测试场景（DG 绕黑洞 r=1000km, 0.12c） |

### 官方文件调用点（改动最小化）

| 文件 | 调用内容 |
|---|---|
| `OVP/D3D9Client/Scene.h` | `LENSBODY` 结构、cubemap 成员、方法声明、`REL_MAX_LENS=12` |
| `OVP/D3D9Client/Scene.cpp` | 构造/析构 `Init/ExitLensCubemap`；`RenderMainScene` 的 cubemap→合成顺序 |
| `OVP/D3D9Client/CelSphere.h/.cpp` | `RenderBackground`（源系 cubemap 渲染）、`RenderBackgroundPano`（残留未用） |
| `OVP/D3D9Client/CSphereMgr.h/.cpp` | `CelDataStruct` 扩展字段、`Render` 参数（bNoRel/bWireframe/bPano）、剔除 `m_bNoCull` |
| `OVP/D3D9Client/shaders/CelSphere.hlsl` | CelVS 光行差/透镜/全景模式；CelPS 白线框模式 |
| `OVP/D3D9Client/VStar.cpp` | billboard `relScale`（光行差视大小） |
| `OVP/D3D9Client/VObject.h/.cpp` | `RepositionFromCamera`、`relScale` |
| `OVP/D3D9Client/D3D9Config.h/.cpp` | RelDoppler/RelAberration/RelLensing/RelGravityMethod/RelInternalMode/RelExposureWhite/RelNearGuard/LensCubemapSize/RelLensWireframe |
| `Src/Orbiter/Config.h/.cpp` | `CFG_VISUALPRM` 加 bRelDoppler/bRelAberration/bRelLensing/bRelLensWireframe/RelExposureWhite；CFGPRM 读写 |
| `Src/Orbiter/GraphicsAPI.h`（Orbitersdk/include） | `CFGPRM_RELLENSWIREFRAME 0x002D` |
| `Src/Orbiter/OptionsPages.cpp` | Options 视觉设置组（Doppler/光行差/透镜 + 曝光） |
| `Src/Orbiter/Orbiter.rc` / `resource.h` | 设置组对话框 + Extra 页面板框（IDC 1200-1204, 1409） |
| `Src/Orbiter/TabExtra.cpp` | Extra → Relativity 对话框（重力模型 + 线框 debug） |
| `Src/Orbiter/OrbiterAPI.cpp` + `Orbitersdk/include/OrbiterAPI.h` | **新 API `oapiGetRelativitySpin`**（a/M + 自旋轴，与物理侧同源） |
| `Src/Orbiter/Psys.cpp` | （上一条鱼）物理侧相对论积分；视觉侧只读其数据 |
| `install64/Orbiter/Config/Sun.cfg` | 已恢复原样（Mass/Size 等） |

---

## 3. 关键实现

### 3.1 光行差（星空）

- 源系星空渲染进 cubemap（不变量：采样方向与观察方向解耦，无"屏幕外数据"问题）
- 合成 pass：屏幕方向 `dw` → `AberrateInverse(dw)`（真实方向 → 静止系方向，注意是**逆变换**：
  观察者运动把源拉向运动方向，所以反解是把观察方向**背离**运动方向旋转）
- 天体位置：`ApplyRelativityToBodies` 直接用 `RepositionFromCamera` 位移 camPos
- 天体光行差用**相对该天体的观察者速度**（站行星上不自移）

### 3.2 引力透镜

**公式**（`StarsLens.hlsl`）：

```
b = 相机到天体距离 × sin(方向与天体夹角)
弱场 (x = b/b_c > 1.8):  α = 4μ/b + 15πμ²/4b² + 128μ³/3b³      (μ = GM/c²)
强场 (x → 1):            α = -ln(x-1) - 0.4016 + 0.825(x-1)      (Bozza 强场极限)
混合: 1.2 < x < 1.8 smoothstep 平滑
clamp: α ≤ 0.8 rad
```

**反解**（观察方向 → 源方向）：Rodrigues 球面旋转（绕 `normalize(cross(bodyDir, d))` 轴
旋转 α，远离天体），单次映射 + 0.75 阻尼（避免二次迭代过冲撕裂），NaN 防护。

**Kerr 一阶自旋修正**（`oapiGetRelativitySpin` 提供 a/M 与自旋轴）：

```
α += -4·(a/M)·μ/b² · dot(自旋轴, (天体方向×光线方向)归一化)
```
- 主程序 API 与物理侧 `SingleGacc_kerr` 同源：`J = InertiaFactor·M·R²·RotOmega·RotAxis`，`a/M = J·c/(G·M²)`
- 只在物理模型 = Kerr 时生效（跟随 `RelativityGravityMethod`）

### 3.3 cubemap 关键坑（已解决的历史 bug）

1. **剔除**：`CSphereManager` 用主相机视锥剔除会漏瓦片 → cubemap 渲染时跳过（`m_bNoCull`）
2. **背面剔除**：从天球内部看瓦片是背面 → `D3DCULL_NONE`
3. **渲染顺序**：背景层必须在合成前画完，否则主背景覆盖合成结果（曾出现大黑块）
4. **源系渲染**：cubemap 渲染时必须 `bNoRel=true`（否则瓦片先被透镜扭曲一次再合成 → 双重扭曲）
5. **合成 quad**：用 NDC 坐标（-1..1）而非 `viewW/viewH`，并显式 SetViewport 全屏
   （否则窗口变化/设备丢失后 quad 缩到左上角）
6. **D3D9 设备丢失**（`D3DERR_DEVICENOTRESET`）：由驱动/窗口切换引起，重启恢复

---

## 4. 构建 / 安装 / 测试

```powershell
# 构建（VS2022 x64, CMake -> build64）
call "<VS>\VC\Auxiliary\Build\vcvarsall.bat" x64
cmake --build D:\AI_projet\orbiter\orbiter\build64 --config Release --parallel 8

# 安装（复制插件+shader；注意还要复制 Orbiter.exe！）
powershell -NoProfile -ExecutionPolicy Bypass -File D:\AI_projet\orbiter\install64_fast.ps1
copy build64\Release\Orbiter.exe install64\Orbiter\Orbiter.exe   # 主程序也要复制！

# 全量重编译（改 CSphereMgr/Scene 等结构时，避免陈旧 obj 崩溃）
Remove-Item build64\OVP\D3D9Client\D3D9Client.dir\Release\*.obj -Recurse
```

**shader 改完只需复制文件 + 重启游戏**（运行时编译，不用重编 DLL）。
**fxc 验证**：`"D:\Microsoft DirectX SDK (June 2010)\Utilities\bin\x64\fxc.exe" /T ps_3_0 /E PSMain StarsLens.hlsl`

### 测试流程

1. 启动 `install64\Orbiter\Orbiter.exe`
2. 启动器：Extra → Relativity（Kerr/1PN、线框 debug）→ OK → Launch
3. 场景：`Scenarios\BlackHole_Test`
4. **物理 → Relativity 必须开启**（否则视觉效果全部强制关闭）
5. 诊断日志：`install64\Orbiter\RelativityTrace.log`（独立文件，可运行中读取）、
   `Modules\D3D9Client\D3D9ClientLog.html`（运行时独占）、`Orbiter.log`

---

## 5. 已知问题 / 限制

1. **cubemap 面边界/极点轻微伪影**：D3D9 cubemap 无 seamless filtering。已有
   `SampleCubeSmooth` 边界柔化缓解。极点区域（相机朝向面中心）可能出现放射/条纹状伪影，
   这是 cubemap 方案固有问题（用户已知晓，暂接受）。
2. **Kerr 自旋修正是一阶弱场近似**（`-4aμ/b²·...` 项），非精确 Kerr 光偏折公式；
   在极端近光子球自旋场景下不精确。
3. **星点（dots）**：源系 cubemap 里渲染了星点（RenderStars）+ 背景图（银河/星图），
   但合成后星点亮度需确认与主渲染一致。
4. **性能**：cubemap 每帧 6 面重渲染（`LensCubemapSize=2048`，显存 ~100MB）；
   有 `relLensCubeBrightness` 缓存机制但**被临时禁用**（每帧重渲染），可恢复懒更新。
5. **残余诊断代码**：见下节待清理清单。
6. **配置**：`install64\Orbiter\D3D9Client.cfg` 的 `LensCubemapSize = 2048`；
   `Orbiter.cfg` 中 `Relativity = TRUE`、三个视觉开关 TRUE。
7. **黑洞测试场景**依赖 system `BlackHoleSystem` 与 `BlackHole.cfg`（10 太阳质量，
   放在 SPICE SUN 位置），测试前需要 config 存在。

---

## 6. 待清理清单（交付前）

1. **`Relativity.inc`**：
   - `RelLog("LensCube: face %d start/done")` 每帧 6 条日志（调试噪音，可删）
   - `RelLog("Composite: start/done")`（可删或降频）
   - `dbgLens`/`dbgState` 等诊断计数器（保留少量关键日志即可）
2. **`CSphereMgr.cpp`**：`m_nRendered` 计数 + `LogAlw("CelRender: ...")` 瓦片数诊断（可删）
3. **`CSphereMgr.h`**：`m_nRendered` 成员（随上删除）
4. **全景残留**（未使用功能，可整块删除）：
   - `CelSphere.h/.cpp`：`RenderBackgroundPano`
   - `CelSphere.hlsl`：`fPanoMode`/`fPanoCenterU` 分支
   - `CSphereMgr.h/.cpp`：`bPano` 参数、`m_bPano`、`fPanoCenterU` 设置
   - `CelDataStruct`：`fPanoMode`/`fPanoCenterU`/fPad10/11
5. **`Scene.cpp`**：`RelLog("Scene: cel sphere lenscube=...")` 每帧日志（可删）
6. **`VStar.cpp`**：确认 `relScale` 相关代码干净（无临时 return）
7. **`D3D9Config`**：`RelLensWireframe` 保留（debug 选项）；确认默认 false
8. 保留：`RelLog` 机制本身（运行诊断有用，但可只保留关键行）

---

## 7. 下一步建议（按优先级）

1. **清理诊断代码**（上节清单）→ 全量重编译 → 验证正常星空效果
2. **恢复 cubemap 懒更新**（星图瓦片加载完成后停止每帧重渲染，降低 GPU 负载）
3. cubemap 极点伪影改善（可选：极点区域跳过瓦片 / 面边界羽化增强）
4. 精确 Kerr 光偏折公式（若用户要求；需要更强的物理公式与验证）
5. 写真正常星空效果截图，确认 Dopper/光行差/透镜三者叠加无残影
6. 更新 README / 版本历史

---

## 8. 调试工具与经验

- **可视化诊断优先**：线框模式（瓦片网格）、六色面（cubemap 面方向）、
  方向编码（采样方向 `s*0.5+0.5` 打色）——比读代码猜有效得多
- **日志**：`RelLog` → `RelativityTrace.log`（独立文件，可运行中读）；
  `LogAlw` → `D3D9ClientLog.html`（运行时独占，退出后读）
- **崩溃**：`%LOCALAPPDATA%\CrashDumps\Orbiter.exe.*.dmp`；`temp\dumptool.exe`/`symtool.cpp`（PDB 加载失败）
- **测试纪律**：每次改动后 构建→安装→重启→用户确认；不要擅自关游戏
- **备份**：`backup/v1.2-relvis/`（changes.patch 可 git apply 恢复；newfiles 含新文件）

---

## 9. 相关背景（上一条鱼的物理侧）

- 物理侧在 `Src/Orbiter/Psys.cpp`：1PN（`SingleGacc_pn`）与 Kerr（`SingleGacc_kerr`）测地线积分，
  `RelativityGravityMethod` 选择求解器（0=1PN, 1=Kerr）
- 视觉侧**不修改物理**，只读取：天体质量/方向/距离（oapiGetMass/GlobalPos）、
  自旋参数（新 API `oapiGetRelativitySpin`，与 Kerr 求解器同源公式）
- 物理开关 `Relativity` 在 `Orbiter.cfg` 的 physics 段；**关闭时视觉全部强制关闭**
