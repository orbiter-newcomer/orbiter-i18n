# 重构交接（死代码清理 + 官方文件剥离）

> 日期：2026-11-21
> 仓库：`D:\AI_projet\orbiter\orbiter`
> 分支：`refactor/deadcode-strip`（HEAD `6ee7367b`，未推送远端）
> 基线：`revert/official-shadow`（`d11e1a4f`，重构前）
> 状态：7 个重构提交完成，构建/冒烟通过；**有一个阴影渲染回退 bug 待排查**（见 §3）。

---

## 0. 当前状态一句话

死代码清理 + 官方文件剥离已完成（官方文件新增行 +3,290 → +1,660，约 −49.5%）；构建通过、current state 冒烟 PASS、`install64` 已部署重构完成版；发现“飞船阴影没有投在正确平面”的渲染 bug，已排除部分原因，移交给下一条鱼排查。

## 1. 环境与常用命令

```powershell
cd D:\AI_projet\orbiter\orbiter
# 构建（必须 --parallel 1；先清理 HTTP_PROXY/http_proxy 等大小写重复环境变量）
$cmake = 'D:/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
& $cmake --build build64 --config Release --target Orbiter D3D9Client --parallel 1
# 部署
Copy-Item build64\Release\Orbiter.exe install64\Orbiter\Orbiter.exe -Force
Copy-Item build64\Modules\Plugin\D3D9Client.dll install64\Orbiter\Modules\Plugin\D3D9Client.dll -Force
Copy-Item OVP\D3D9Client\shaders\*.hlsl install64\Orbiter\Modules\D3D9Client\ -Force   # shader 是运行时从安装目录加载！
# 冒烟（current state 场景，自动判 PASS/FAIL，约 25s）
powershell -ExecutionPolicy Bypass -File .\tools\smoke_test.ps1 -KillExisting
```

开发规则要点（必须遵守）：中文思考/回复；不删除未明确授权的文件；保存必须用 git，本地提交可以自便，**不准推送远端**；官方文件修改最小化并每次报告改动量；测试只测 current state 场景（正常启动 + 截图确认无恶性渲染 bug），新场景测试先给用户方法由用户测；游戏启动 ≤30s，不要长期跑后台任务。

## 2. 已完成的重构（7 个本地提交）

| 提交 | 内容 | 官方文件改动 |
|---|---|---|
| `2c5e36a4` | 阶段1：删死代码（路线B残骸、UpdateEffectShadows、eLVP2组句柄、夜面简化死链、死配置/成员、PerfCore+PERF日志、死SDK声明、BuildEquilibrium、WreckDestroyVessel、RenderFadeTiles 等） | 16 文件 −461 行 |
| `6db25ca6` | 热控：17 个函数从官方 `ThermalSubsys.cpp` 搬入 **`ThermalModel.inc`** | −436 行 |
| `d34e1b3b` | i18n 布局/对话框 API：实现→`i18n/core/DlgLayoutApi.inc`，声明→`Orbitersdk/include/I18nAPI.h` | OrbiterAPI.cpp −337、.h −116 |
| `46c3ba5f` | 地形：morph/fade/LOD/高程跳过/加载等待等 → **`OVP/D3D9Client/TerrainRender.inc`**（PART 分段）+ **`shaders/TerrainRender.inc.hlsl`**；删除独立 `TerrainLoadWait.inc` | Surfmgr2 −114、Tilemgr2 −65、NewPlanet.hlsl −67、D3D9Client −1 |
| `f306344f` | Vessel 并行更新函数→`Src/Orbiter/VesselUpdate.inc`；`Scene::RenderMainScene2D`→`OVP/D3D9Client/SceneRender2D.inc` | Vessel.cpp −331、Scene.cpp −222 |
| `9981b9f2` | Batch2：头文件成员声明搬入类内 include 的成员块（Orbiter/Vessel/Scene/D3D9Client/D3D9Config/热控/项目配置），共 9 个 `*Members_*.inc` | 6 个官方头文件 −221 |
| `6ee7367b` | 接触模型对话框资源→`ContactDialogRes.rc`/`ContactDialogLang.rc`/`ContactResources.h`；D3D9Client 成员块 | Orbiter.rc/resource*.h −52、D3D9Client.h −21 |

重构后官方文件基本只保留 `#define xxx` + `#include "模块.inc"` 钩子；地形只剩 `TerrainRender.inc`（1218 行）+ `TerrainRender.inc.hlsl`（78 行）两个文件，热控只剩 `ThermalModel.inc`（1173 行）。

> 说明：`temp/` 目录是本次重构的临时 Python 脚本（未跟踪、未提交，可删）。

## 3. ⚠ 待排查 bug：阴影没有投在正确平面

**现象**（用户截图）：两架 DG 悬停时，阴影被拉长/投到远处地面，而不是机腹正下方的地面。用户反馈“此前是可以的”。

**已排除**：在重构完成版基础上恢复阶段1删除的阴影参数代码后仍然异常（恢复项：`Scene::UpdateEffectShadows`、`eLVP2/eShadowMap2/eSHD2/eSHDT2`、G1→近场参数写入块、`RenderSceneShadow` 末尾调用）→ 不是/不仅是那段代码。恢复已撤销，工作区=纯重构完成版。

**可疑提交（按怀疑度）**：
1. `46c3ba5f` 地形收敛（改动最大：TerrainRender.inc 结构 +517、Surfmgr2/Tilemgr2 函数搬移、NewPlanet.hlsl shader include 搬移）— **首选**
2. `f306344f` Vessel/Scene 搬移
3. `9981b9f2` Batch2 成员块（Vessel.h/Scene.h/D3D9Config.h 成员位置变化）
4. `2c5e36a4` 死代码删除（已部分排除）

**定位方法**：
```powershell
# 基线（重构前）
git checkout revert/official-shadow          # = d11e1a4f
# 或逐个提交试验（不产生提交）：
git revert --no-commit <hash> && 构建+部署 && 用户测阴影
git checkout -- .                             # 撤销试验
```
或用 `git bisect start; git bisect bad refactor/deadcode-strip; git bisect good revert/official-shadow`（7 个提交，3 次构建可定位）。

**阴影系统速查（重构后位置）**：
- G1 场景阴影（地形自阴影/投影）：`TerrainRender.inc` SCENE 段（`Scene::RenderSceneShadow`、`ShadowSystem` 等）；接收端 shader 在 `NewPlanet.hlsl` 的 TerrainPS 内嵌块（未搬）+ `TerrainRender.inc.hlsl`（`SampleShadowsT`、caster `ShadowVS/PS`、`TerrainTileFade`）；`Surfmgr2.cpp` 中 G1 接收块直接写 `PObjectParams`（mLVP2/vSHD2/vSHDT）+ `bShadowsT`。
- G0 近场阴影（飞船/基地）：官方 `Scene::RenderShadowMap`（Scene.cpp）+ `VPlanet.cpp:977-987` 参数设置；`NewPlanet.hlsl` 顶部 `tShadowMap` 后新增了 `#include "TerrainRender.inc.hlsl"`（重点检查该 include 位置/编译期 effect include 路径）。
- 飞船旧投影地面阴影（用户截图最像这条）：`TerrainRender.inc` VESSEL 段（`vVessel::UpdateShadowPlane` / `RenderGroundShadow`，参考平面 = 实际渲染表面，含 2s/1m 缓存）+ `VVessel.cpp:21-22` include 钩子。
- 已知隐患（未修）：`Surfmgr2.cpp` 里 `bShadows=true`（近场）与 `bShadowsT=true`（G1）同时置位、且唯一 `tShadowMap` 采样器被覆盖绑定到 G1 贴图 → 地形可能用近场矩阵采样 G1 图。
- 也可能不是重构引起（v1.3.0 就有的参考平面缓存/悬停低太阳角问题），先做基线对比。

## 4. 第二轮待办（清过一波后还剩 12 项）

确认重复 5 项：R1 G1 阴影管线合并官方 `RenderShadowMap`；R2 地形接收端第二套 uniform/采样合并（先修 bShadows 双置位隐患）；R3 caster shader 复用官方 `ShdMapVS/ShdMapPS`；R4 `RelativityConfig.inc` 自写精确键解析→官方 `Config::GetItem*`；P7 Wreck `rand()/srand`→官方 `oapiRand()`（`srand(GetTickCount())` 破坏官方 `srand(12345)` 确定性）。
部分重复 7 项：P1 PerfDiag 101 处埋点/裸 QPC 收口；P2 瓦片计数三套 vs `D3D9Stats.TilesRendered`；P3 地平系法线旋转手写两遍（其中 `WreckDebris.inc:610 RelTerrainNormal` 还是第一轮漏删的死函数）；P4 `oapiSurfaceElevationEx` 未用 `InitTileCache`；P5 编码转换三份；P6 `EstimateWidth` vs 官方 `GetTextWidth`；P8 PerfStats 与 `_D3D9Stats` 语义重叠。
其他遗留：死 API `oapiZhFitControlText/Row/Height`、`oapiZhLayoutDialogButtons`、`oapiGetLayoutFloat` 无调用者（保留/删需用户定）；死特性链（内伤禁用/延迟销毁/飞行中等待/数值故障队列）未接线；临时诊断默认开启无开关（PerfDiag、DUAL_THREAD_DIAG、[ELEV]、PhysicsThread [PT] 日志）；aap.lua 整定与 VideoTab inline 汉化策略待用户定。

## 5. 沟通注意

- 用户偏好：中文、直接、少啰嗦；先给结论/数据，不要长篇探索过程。
- 不做未授权的事；改官方文件后要报告改动量；本地提交不推送。
- 用户会帮做人工验证（测试规范限制），需要复现步骤时说清楚。
