# 物理/渲染双线程 —— 路线 A 实现交接

> 日期：2026-11-20
> 分支：`revert/official-shadow`（工作区 `D:\AI_projet\orbiter\orbiter`）
> 目标：把**物理求解**（`PlanetarySystem::Update`：相对论传播/刚体/重力/子系统）放到独立物理线程，D3D9Client 的 3D/2D 渲染、仪表/MFD/对话框/HUD 全部留在主线程。
> 开关：`Src/Orbiter/Orbiter.cpp` 顶部 `#define DUAL_THREAD_EXPERIMENT`（1=双线程，0=原单线程循环）。

---

## 1. 结论

- 双线程模式已跑通，核心场景（物理重、渲染中等）**帧率 +25%~30%**；渲染瓶颈场景不退化。
- 物理线程只跑 `g_psys->Update()`；所有会碰 UI / 图形客户端 / 创建删除仿真对象的代码（插件回调、对话框、残骸创建、船删除、`EndTimeStep` 视觉更新）都留在主线程的物理暂停窗口里，**没有跨线程创建/删除 D3D 资源**。
- 官方源码改动最小化：主要逻辑放在新增 `Src/Orbiter/PhysicsThread.inc`；官方文件仅加少量分支/钩子（见 §4）。

---

## 2. 为什么选路线 A（而不是原路线 B「渲染线程化」）

原路线 B（物理主线程 + 3D 渲染次线程）在交接文档中卡在"D3D9 渲染线程第 2 帧停滞"。评估结论：
1. D3D9 设备/资源/Present/纹理上传线程的线程归属是系统性重构，不是单点 bug；
2. B1（3D 次线程、物理+2D+Present 主线程）的帧时间 = `max(物理+2D+Present, 3D)`，**比 A 的 `max(物理, 渲染)` 更差**；只有 B2（全部渲染进次线程）收益才与 A 持平，但风险最高；
3. A 的状态分离（视觉缓存更新）将来做 B2 时仍可复用。

---

## 3. 线程模型与帧流水线

| 线程 | 职责 |
|---|---|
| **物理线程** | `g_bStateUpdate=true` → `g_psys->Update(g_bForceUpdate)`（不再做其它事） |
| **主线程 StageB**（物理线程停在同步点） | `WreckDebrisUpdate`（相机部分）、`pDlgMgr->UpdateDialogs`、`ModulePostStep`（插件/船模块回调）、`g_bStateUpdate=false`、`WreckProcessPendingSafe`、`KillVessels`、`PublishRenderSnapshot`、`EndTimeStep`（`FinaliseUpdate`、`td.EndStep`、`g_camera->Update`、`g_pane->Update`、`gclient->clbkUpdate`、`SessionLimitReached`） |
| **主线程 StageA**（物理线程停在同步点） | `UpdateRelativityWarp`、`BeginTimeStep`（含暂停广播、`td.BeginStep`）、`UserInput`、`CaptureVideoFrame`、控制台命令、`ModulePreStep`、向物理线程发布 `bRunning/g_bForceUpdate/bPlayback` 镜像 |
| **主线程 渲染**（与物理线程并行） | `Render3DEnvironment()`：3D/2D/ImGui/Present |

帧流水线（1 帧延迟）：

```
主线程: Wait(物理帧 N-1) → StageB(N-1) → StageA(N) → Release(物理帧 N) → Render(N-1)
物理线程:                               [---------- 物理帧 N ----------]
```

- 同步：`std::mutex + std::condition_variable`（`m_dualMutex/m_dualCv`），主线程等待期间泵 Windows 消息；
- `td`（时间）只由主线程写（`BeginTimeStep`/`EndTimeStep`），物理线程只读；渲染读 `oapiGetSimTime()` = `td.SimT0`，与渲染使用的稳定状态 `s0` 一致；
- 1 帧延迟对相机/仪表可忽略（同一帧内 StageB 已更新相机与视觉缓存）。

---

## 4. 代码结构与官方源码修改清单

### 新增文件（非官方）
| 文件 | 说明 |
|---|---|
| `Src/Orbiter/PhysicsThread.inc` | 线程编排全部实现：物理线程循环、StageA/StageB、等待/放行、性能日志、`RunDualThread()` 主循环 |

### 官方文件修改（`Src/Orbiter`）
| 文件 | 改动 | 量 |
|---|---|---|
| `Orbiter.cpp` | ① 宏注释；② 全局线程辅助（`g_mainThreadId`/`g_bDualThreadMode`/`OrbiterIsMainThread()`）；③ 删除旧路线 B 实验代码（`RenderThreadProc`/旧 `PhysicsThreadProc`/`Render2DEnvironment`/Run 里两段旧宏块）；④ 宏=1 时 include `PhysicsThread.inc` 且 `Run()` 转发 `RunDualThread()`；⑤ `PreCloseSession()` 开头停物理线程 | 净减少（删旧实验代码），新增 ~30 行 |
| `Orbiter.h` | 旧路线 B 成员替换为路线 A 同步成员/方法声明；`<condition_variable>` | ~48 行 |
| `Rigidbody.cpp` | `Els()`：双线程模式主线程固定用 `SimT0`（稳定状态），物理线程按 `g_bStateUpdate` | 9 行 |
| `elevmgr.h/.cpp` | `ElevationManager::Elevation()` 加 `std::recursive_mutex`（物理线程与渲染主线程并发查高程：可变 tile 缓存、共享 `FILE*`、`clbkFilterElevation` 回调图形客户端） | 11 行 |
| `PerfDiag.h` | `g_suspended` 改 `std::atomic<bool>`（双线程下增强健壮性） | 9 行 |

### 官方文件修改（`OVP/D3D9Client`）
| 文件 | 改动 | 量 |
|---|---|---|
| `VPlanet.cpp` | 高程平坦化全局表（`g_ShapeStore`/`g_ElevationFlatteningShapes`/`g_ShapesLoaded`）加 `std::recursive_mutex`：物理线程经 `clbkFilterElevation` 与瓦片加载线程 `FilterElevationGraphics` 会并发访问 | 10 行 |

> 未改动 SDK / `GraphicsAPI.h` / `Scene::RenderMainScene`。之前路线 B 加入的 `clbkRenderScene3D/2D` 拆分保持原样、默认不启用（不额外增加本轮改动）。

---

## 5. 线程边界审计结论（依据两份只读审计报告）

### 已在主线程（物理暂停窗口）执行的高风险项
- `pDlgMgr->UpdateDialogs()`、插件 `clbkPreStep/clbkPostStep`、船模块 `clbkPreStep/clbkPostStep`（可能删船/建船/开对话框/阻塞 Lua）→ StageA/StageB；
- `KillVessels()`（焦点/相机/插件/对话框/MFD/控制台/D3D 资源销毁）→ StageB；
- `WreckProcessPendingSafe()`（`oapiCreateVesselEx` + `clbkNewVessel` + D3D 资源创建）→ StageB；
- `gclient->clbkUpdate()`（D3D9Client `Scene::Update`：视觉对象增删、mesh/纹理上传）→ StageB；
- `g_camera->Update()` / `g_pane->Update()` / `SessionLimitReached()` → StageB（`EndTimeStep`）。

### 留给物理线程的残余风险（已知、待实测观察）
1. `Vessel::UpdateAnimate()`（`clbkAnimate`，在 `g_psys->Update` 的并行更新尾部）仍由物理线程调用；插件动画回调可能改 mesh/动画状态，与渲染期 `Scene::UpdateAnimations` 存在非原子竞争（可能造成视觉抖动，一般不崩溃）。
2. 物理链内的残骸触发（`WreckSurfaceImpact`/`WreckAtmosphericBurnup`）：写全局消息缓冲、`WreckSpawnHull` 运行时写 MSH 文件、`oapiSetVesselShadowMode`；`ResolveCollisionWith` 里 `SetWarpFactor(1.0)`（td + 插件回调）也在物理线程。
3. 首次进入低空时的懒加载：`CheckBaseCollisions` 的基地碰撞网格加载、部分 mesh 加载仍在物理线程。
4. `Timejump`/菜单等主线程操作在 `WaitPhysicsFrame` 的消息泵中执行时，物理线程可能正在跑帧（未加互斥）。测试场景未触发；后续若要支持"运行中时间跳转"需加保护。
5. 随机数：MSVC `rand()` 为线程局部序列，物理线程化后残骸/风场等随机序列改变（行为变化，不可复现，非崩溃）。

---

## 6. 实测数据（1920×1080 窗口模式，本机；`PerfLog=3`）

| 场景 | 单线程基线 | 路线 A | 说明 |
|---|---|---|---|
| `_perf_space`（深空 DG，物理重/渲染轻） | fps≈38.5–39.3，upd=22.5ms，rnd=2.6ms | fps≈39.2，phys=24.7ms，rnd=3.1ms，wait=0 | 持平；渲染太轻，按 `max(物理,渲染)` 无从收益；物理变慢约 2ms（CPU 竞争） |
| `_perf_night_orbit`（ISS 夜轨道，物理重） | fps≈23–35（正常段 upd=30–34ms，rnd=3–12ms） | **fps≈30–32**（phys=29–33ms，rnd=5–26ms，wait=5–27ms） | **约 +25%~30%** |
| `_perf_ground`（地面，渲染重/物理轻） | fps 35–46（前期）/ 24–26（后期） | fps 33–42，rnd=19–28ms，wait=0 | 不退化（渲染瓶颈，符合预期） |

补充：
- 窗口模式下渲染耗时只有 3–28ms（交接文档全屏实测 rnd 约 10ms 量级），**用户实际全屏 2048×1152 下渲染更重，收益应高于上表**。
- 测试中曾出现一次 `Present` 阻塞（pres≈40ms、GPU frame 间隔 50–88ms）的异常，未复现，怀疑为测试环境瞬时因素（同帧日志有 `GPU query polling failed`，PerfLog=3 的 GPU profiling 触发）；需用户在实测中留意。
- **130s 长跑对照**：长跑约 30s 时出现既有相对论数值保护触发（`Wreck: numerical failure`/`non-finite vessel state`，多个 vessel 被移除、物理变慢至 70–90ms）。用**单线程基线（宏=0）复跑同一 130s 场景，完全相同的数值失败在同一时间点出现**，故该问题与本轮双线程改造无关（属 fork 既有相对论/热模型数值问题，另案处理）。


---

## 7. 开关、回退与配置

- 关闭双线程（回到稳定单线程）：`Src/Orbiter/Orbiter.cpp` 顶部 `#define DUAL_THREAD_EXPERIMENT 0`，重编 `Orbiter` 目标即可（`PhysicsThread.inc` 不参与编译）。
- 构建（x64 Release，必须 `--parallel 1`；构建前清理 `HTTP_PROXY/http_proxy` 等大小写重复环境变量，否则 MSBuild 报 "已添加项"）：
  ```
  cmake --build orbiter\build64 --config Release --target Orbiter D3D9Client --parallel 1
  ```
- 部署：`build64\Release\Orbiter.exe` → `install64\Orbiter\Orbiter.exe`；`build64\Modules\Plugin\D3D9Client.dll` → `install64\Orbiter\Modules\Plugin\D3D9Client.dll`。
- 日志：`Orbiter.log` 中 `[PT] ...` 为双线程诊断（`[PT] PERF: fps/phys/sA/sB/wait/rnd`、`[PT] UPD: 物理细分明细`）。

---

## 8. 验收清单（需用户实测）

- [ ] ISS 旋转/自动稳定观察场景：10~20fps → ≥25fps；
- [ ] 稳定/地面场景：不退化（≥30fps）；
- [ ] 仪表/MFD/对话框/HUD 正常（含 ImGui 对话框、MFD 交互）；
- [ ] 相机操作（旋转/切换视角/时间加速/暂停）正常；
- [ ] 无崩溃、无画面异常（尤其：镜头快速旋转、进出大气、残骸/爆炸、对接分离、Alt-Tab/全屏切换、场景退出回 Launchpad）；
- [ ] 回归：地形/阴影/残骸/汉化正常。
