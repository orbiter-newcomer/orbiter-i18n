# 重构交接（死代码清理 / 官方文件剥离 / 阴影回归官方）

> 更新日期：2026-11-21
> 仓库：`D:\AI_projet\orbiter\orbiter`
> 分支：`refactor/deadcode-strip`（HEAD `447d63ba`，**未推送远端**）
> 基线：`revert/official-shadow`（`d11e1a4f`，重构前）
> 状态：构建通过、current state 冒烟 PASS、`install64` 已部署。**阴影逻辑已全部回归官方，自研场景阴影(G1)已删除。**

---

## 0. 一句话现状

死代码清理 + 官方文件剥离 + 阴影回归官方已完成（官方文件新增行相对 v1.3.0 从 +3,290 降到 **+1,394（−57.6%）**）；地形投影（地形阴影）将作为**新独立功能**开发，开发入口与注意事项见 §4。

## 1. 环境与常用命令

```powershell
cd D:\AI_projet\orbiter\orbiter
# 构建（必须 --parallel 1；先清理 HTTP_PROXY/http_proxy 等大小写重复环境变量）
$cmake = 'D:/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
& $cmake --build build64 --config Release --target Orbiter D3D9Client --parallel 1
# 部署
Copy-Item build64\Release\Orbiter.exe install64\Orbiter\Orbiter.exe -Force
Copy-Item build64\Modules\Plugin\D3D9Client.dll install64\Orbiter\Modules\Plugin\D3D9Client.dll -Force
Copy-Item OVP\D3D9Client\shaders\*.hlsl install64\Orbiter\Modules\D3D9Client\ -Force   # shader 运行时从安装目录加载
# 冒烟（current state 场景，自动判 PASS/FAIL，约 25s）
powershell -ExecutionPolicy Bypass -File .\tools\smoke_test.ps1 -KillExisting
```

开发规则要点：中文；不动未授权的文件/不删未授权文件；git 本地提交可以自便，**禁止推送远端**；官方文件修改最小化并在每次改动后报告改动量；测试只测 current state 场景（正常启动+截图无恶性渲染 bug），新场景/新功能测试方法先告知用户由用户测；游戏启动 ≤30s，后台任务不要长跑。

## 2. 已完成的工作（14 个本地提交，`d11e1a4f..HEAD`）

| 提交 | 内容 |
|---|---|
| `2c5e36a4` | 阶段1 删死代码（路线B残骸、UpdateEffectShadows、eLVP2 组句柄、夜面简化死链、死配置/成员、PerfCore+PERF 日志、死 SDK 声明、BuildEquilibrium、WreckDestroyVessel、RenderFadeTiles 等，官方 −461 行） |
| `6db25ca6` | 热控 17 个函数从官方 `ThermalSubsys.cpp` 搬入 **`ThermalModel.inc`**（官方 −436 行） |
| `d34e1b3b` | i18n 布局/对话框 API 实现→**`i18n/core/DlgLayoutApi.inc`**，声明→**`Orbitersdk/include/I18nAPI.h`** |
| `46c3ba5f` | 地形实现→**`OVP/D3D9Client/TerrainRender.inc`**（PART 分段）+ 当时新增的 shader 文件；独立 `TerrainLoadWait.inc` 删除 |
| `f306344f` | Vessel 并行更新→`Src/Orbiter/VesselUpdate.inc`；`Scene::RenderMainScene2D`→`OVP/D3D9Client/SceneRender2D.inc` |
| `9981b9f2` | 头文件成员声明搬入类内 include 成员块（9 个 `*Members_*.inc`） |
| `6ee7367b` | 接触模型对话框资源→`ContactDialog{Res,Lang}.rc`+`ContactResources.h`；D3D9Client 成员块 |
| `ef97f044` | 第一版重构交接文档（已被本文件取代） |
| `fb43470b` | 外部修复：投影地面阴影/近场阴影路径回归官方 |
| `eb70593c` | **删除自研 G1 场景阴影系统 + 阴影模式 API**（−1,336 行）：`TerrainRender.inc` SCENE 段、`TerrainRender.inc.hlsl` 整文件、NewPlanet.hlsl 的 G1 接收/投射、Scene 的 smapT/ShadowSystem、D3D9Effect G1 句柄、8 个 G1 配置、`oapiSet/GetVesselShadowMode`+`SHADOWMODE_*` |
| `0af3ac3c` | 删死函数 `RelTerrainNormal`；R4 改用官方 `Config::GetItemBool`；P7 改用官方 `oapiRand()` 并删除 `srand(GetTickCount())` |
| `e7c83eab` | 删除未接线死特性链（内伤禁用/延迟销毁），Orbiter 输入处理恢复官方 |
| `77f985f3` | 删除临时诊断：`PerfDiag.h`（101 处埋点）、`DUAL_THREAD_DIAG`、`[PT]` 性能日志（−511 行） |
| `447d63ba` | 删除过时阴影 handover 文档，README/README_EN 阴影条目同步 |

**官方文件修改程度（vs v1.3.0）**：79 个官方文件 **+1,394 / −1,650**；总 diff 165 文件 +10,077/−2,177。

## 3. 当前代码结构（阴影回归官方后的地形/渲染相关）

- `OVP/D3D9Client/TerrainRender.inc`（514 行）：**只剩非阴影**地形功能 PART：
  - `TERRAINRENDER_SURF`（Surfmgr2.cpp）：瓦片 LOD morph / 淡入 / **地形 LOD 动态计数**等
  - `TERRAINRENDER_TILES`（Tilemgr2.cpp）：`UpdateTileCountSmooth`、fade 记账
  - `TERRAINRENDER_LOADWAIT`（D3D9Client.cpp）：地形加载等待状态机
  - 采用在官方 .cpp 末尾 `#define TERRAINRENDER_xxx` + `#include "TerrainRender.inc"` 的钩子模式。
- `OVP/D3D9Client/shaders/NewPlanet.hlsl`：官方地形 shader（含官方近场阴影接收 `_SHDMAP`/`SampleShadows`/`bShadows`；已无任何 G1 代码）。
- **官方阴影路径（必须保持官方，不要再改）**：
  - 近场阴影：`Scene::RenderShadowMap`（Scene.cpp）、`SHADOWMAPPARAM smap`（Scene.h）、参数设置 `VPlanet.cpp` 约 977-987、`eLVP/eShadowMap/eSHD/eShadowToggle`（D3D9Effect）
  - 官方投影地面阴影：`vVessel::RenderGroundShadow`（VVessel.cpp）、`vBase::RenderGroundShadow`（VBase.cpp）
  - 地形接收：`NewPlanet.hlsl` 的 `_SHDMAP` 分支；飞船接收：Vessel.fx/PBR.fx + Common.hlsl
  - 配置：`ShadowMapMode` / `ShadowMapSize` / `TerrainShadowing`（官方项）
- 其余项目模块：`ThermalModel.inc`（热控）、`i18n/core/DlgLayoutApi.inc` + `Orbitersdk/include/I18nAPI.h`（汉化布局）、`Src/Orbiter/VesselUpdate.inc`、`OVP/D3D9Client/SceneRender2D.inc`、9 个 `*Members_*.inc` 成员块、`ContactDialog*.rc`/`ContactResources.h`。

## 4. 给"地形投影（地形阴影）"开发者的交接

### 目标
把地形投影/地形阴影作为**新独立功能**开发，不恢复也不修改已删除的 G1 系统；官方阴影逻辑保持原样。

### 可参考的已删除实现（git 历史里能找到）
```powershell
# 删除前（6ee7367b）的完整 G1 实现：
git show 6ee7367b:OVP/D3D9Client/TerrainRender.inc          # 原 SCENE 段（ShadowSystem、RenderSceneShadow、地形 caster）
git show 6ee7367b:OVP/D3D9Client/shaders/TerrainRender.inc.hlsl  # G1 shader（SampleShadowsT / caster VS/PS / fade uniform）
git show 6ee7367b:OVP/D3D9Client/shaders/NewPlanet.hlsl      # 当时的 G1 接收块
git show 6ee7367b:OVP/D3D9Client/SceneMembers_Scene.inc      # smapT/pShadows/RenderSceneShadow 声明
git show 6ee7367b:OVP/D3D9Client/D3D9Effect.cpp              # eLVP2/eShadowMap2/eSHD2/eSHDT2 查找
```
已知的旧 G1 坑（当时审计发现、未修）：`Surfmgr2.cpp` 中 `bShadows`（官方近场）与 `bShadowsT`（G1）双置位、且唯一 `tShadowMap` 采样器被覆盖绑定 → 地形可能用近场矩阵采样 G1 贴图；新实现建议**独立 sampler 或独立接收路径**，并注意 ps_3_0 采样器上限（16）。

### 建议入口（按官方文件最小化原则）
1. 新建独立模块：`OVP/D3D9Client/TerrainShadow.inc`（C++）+ `OVP/D3D9Client/shaders/TerrainShadow.inc.hlsl`（shader），在官方文件用钩子 include（参考 `TerrainRender.inc` 的 selector 模式）。
2. 阴影贴图/投影矩阵等基础设施可复用官方 `SHADOWMAPPARAM` 结构和 `D3D9Effect` effect 句柄机制；新增的 map/RT 在模块内自管（旧 G1 的 `ShadowSystem` 可作参考）。
3. 地形接收在 `NewPlanet.hlsl` 的 `TerrainPS` 中插入调用（旧 G1 是在此加第二套 uniform/采样，注意上面坑）。
4. caster 可参考官方 `NewMesh.hlsl ShdMapVS/ShdMapPS`。
5. 改动完成后按规则报告官方文件改动量；构建+冒烟+（新功能效果）用户实测。

### 关键入口速查（当前代码）
- 地形渲染主流程：`Surfmgr2.cpp TileManager2<SurfTile>::Render` / `SurfTile::Render` / `RenderNode`
- 地形 shader：`NewPlanet.hlsl`（`TerrainVS` / `TerrainPS`）
- 官方近场阴影生成：`Scene::RenderShadowMap`（Scene.cpp）、`Scene::RenderShadowMap` 调用点
- D3D9Effect 句柄：`D3D9Effect.cpp`（官方 eLVP/eShadowMap/eSHD/eShadowToggle）
- 配置读写：`D3D9Config.cpp`（官方 Read/WriteParams，新增地形投影配置加在这里，注意官方文件最小化——可放独立 .inc 钩子）

## 5. 剩余待办 / 已决定不动

| 项 | 状态 |
|---|---|
| 死 API（`oapiZhFitControlText/Row/Height`、`oapiZhLayoutDialogButtons`、`oapiGetLayoutFloat`、`oapiGetWreckAnchorVessel`） | **保留不动**（用户决定） |
| P2 瓦片计数收口（自维护 vs 官方 `D3D9Stats.TilesRendered`） | **不做**（用途不同、收益低，用户决定） |
| P8 PerfStats 自累计计数器收口 | 未做（PerfLog 默认 0，低优先） |
| P5 编码转换统一（`TextCodec`→核心 API） | 未做（可选小改） |
| P6 `EstimateWidth`→官方 `GetTextWidth` | 建议保留手估（性能取舍） |
| `[ELEV]` 临时诊断清理 | 未做（试过会破坏 elevmgr.h，已回滚，需精细处理） |
| aap.lua 整定 / VideoTab inline 汉化 | 策略待定 |

## 6. 其他说明

- `temp/` 是本次整理的临时 Python 脚本（未跟踪、未提交，可删）。
- 基线分支 `revert/official-shadow`（`d11e1a4f`）保留，可随时对比重构前状态。
- 分支未推送远端；提测/发布前需用户决定推送与打 tag。
