# 物理/主场景渲染双线程 —— 交接文档

> 交接日期：2026-11-20
> 分支：`revert/official-shadow`（HEAD `ac6d0735`）
> 目标：物理求解（相对论/刚体/子系统）与主场景 3D 渲染分线程并行；仪表/MFD/对话框/HUD 保持主线程。当前**全部实验代码已合入但宏默认关闭**，稳定版可用。

---

## 1. 为什么要做（实测数据，`PerfLog=2/3` 日志）

| 场景 | FPS | 物理 `upd` | 渲染 `rnd` | 备注 |
|---|---|---|---|---|
| `_perf_space`（轨道 DG） | 31.7 | **21.4ms** | 10.0ms | `prop/grav` 19~20ms（相对论传播） |
| `_perf_ground`（地面） | 40.5 | 1.5~19ms | **23.1ms** | 渲染大头 |
| `_perf_night_orbit`（ISS） | 27.0 | **27.3ms** | 9.4ms | `subs=364` 子系统 |
| ISS 旋转/自动稳定观察 | 10~20 → 30~40 | — | — | **旋转状态物理 ≈ 25ms+** |

- **相对论/传播器是功能刚需（黑洞/中子星/近光速），不可优化**；
- 轨道/深空类场景**物理 > 渲染** → 并行后帧率 ≈ `1/max(物理,渲染)` → **理论 +50%~2 倍**；
- 帧率 14-16fps 的版本已修复（夜面简化删除 + 合并 main 优化后稳定 37-40fps）。

---

## 2. 已完成的工作（提交）

| 提交 | 内容 |
|---|---|
| `1e2a0433` | 设计文档 `docs/handover/dual-thread-design.md` + 线程框架骨架（`DUAL_THREAD_EXPERIMENT` 宏默认 0） |
| `84b08512` | 渲染快照 `struct RenderSnapshot`（时间+相机核心字段）+ 物理侧每帧发布 `PublishRenderSnapshot()`（UpdateWorld 末尾） |
| `fdce6805` | 线程编排（物理线程/渲染线程/主线程协调、Launch、退出清理）+ `Log.cpp` 线程安全（`LogOutVA` 全局锁）+ 原子成员初始化 |
| `c751cc7d` | `Scene::RenderMainScene` 拆分为 3D（`RenderMainScene`）+ 2D（`RenderMainScene2D`：HUD/2D 覆盖/EndScene） |
| `ac6d0735` | SDK `GraphicsClient::clbkRenderScene3D()/clbkRenderScene2D()`（默认向后兼容）+ D3D9Client 覆写拆分 + Orbiter 主线程 `Render2DEnvironment()`（ImGui+2D+Present） |

宏开关：`Src/Orbiter/Orbiter.cpp` 顶部 `#define DUAL_THREAD_EXPERIMENT 0`（0=稳定单线程，1=双线程实验）。

---

## 3. 已解决/踩坑记录（必读）

1. **`std::atomic` 成员必须初始化**（`{false}`/`{0}`）：否则线程读到垃圾值直接退出，表现为"线程启动日志后无后续"。已在 `Orbiter.h` 修复；
2. **宏=1 分支替换了原主循环** → 原 `Launch(...)` 不会执行 → 场景不加载。宏块内已补 `Launch` 调用；
3. **主线程等待渲染完成时必须泵消息**（等待循环内 `PeekMessage/DispatchMessage`），否则渲染线程内 `SendMessage` 会死锁；
4. **`LogOutVA` 无线程锁** → 多线程并发写日志会崩溃/交错。已加 `std::mutex g_logMutex`（在 `Src/Orbiter/Log.cpp`，**修复已保留**，安全无害）；
5. **渲染线程直接调完整 `Render3DEnvironment` 会卡死**（内部混 ImGui/2D/窗口逻辑）→ 已拆 3D/2D；拆分后**渲染线程能完成第 1 帧 3D**，但**第 2 帧起停滞**（当前卡点）。

---

## 4. 当前卡点与排查建议

**现象**（宏=1，`_perf_ground`）：
- `[DUAL] physics thread started` / `[DUAL] render thread started` 均出现；
- `[DUAL] render frame begin=1 / end=1` → **3D 渲染线程只完成 1 帧**；
- 主线程 `[DUAL] render wait timeout`（5s 超时）；
- 加过场景阶段日志（后已回退），主线程 2D 路径（`Render2DOverlay frame=0`）能执行，3D 子阶段日志未命中。

**判断**：停滞点在渲染线程**第二次进入 D3D9Client 3D 渲染路径**时，性质是 **D3D9 设备/窗口消息/资源锁的线程归属问题**（第一帧能过说明设备可跨线程部分使用，第二帧涉及资源状态/纹理上传/设备查询时被主线程依赖卡住）。

**推荐排查手段（按效率）**：
1. **挂调试器看线程栈（首选）**：卡住时 VS → Attach to Process → Threads 窗口看渲染线程调用栈（`WaitForSingleObject`/`EnterCriticalSection`/`SendMessage`/纹理锁/`Present`）——直接定位；
2. **带线程 ID 的阶段日志二分**：在 `Scene::RenderMainScene` 各大段前后打 `[stage] tid=%lu frame=%d`，找最后执行的阶段，再对子阶段二分（临时关闭阴影/地形/飞船/云/大气/星空看是否恢复）；
3. **嫌疑清单**：`SendMessage`、D3D9 设备在主线程创建/渲染线程使用、纹理上传线程 `TileBuffer::HoldThread`、`PresentScene`、`ImGui` 共享状态、窗口消息依赖。

---

## 5. 两条路线（建议选 A）

### 路线 A：只把物理移出主线程（推荐，收益相同、风险低、不碰 D3D9Client）
- **渲染留在主线程**（D3D9Client 完全不动 → 无跨线程设备/窗口问题）；
- 物理线程循环 `BeginTimeStep → UpdateWorld → EndTimeStep`（相对论 20~28ms 刚需）；
- 物理线程每步发布**快照**（已有 `RenderSnapshot` 雏形，需补：行星位置/旋转、vessel 位置/姿态/可见性、时间、光照、事件队列）；
- 渲染前（主线程）从快照更新**渲染侧缓存**：
  - `vVessel` 的 `gpos/grot` 等渲染缓存（`D3D9Client/VVessel` 更新一处即可，`vVessel::Render` 读缓存）；
  - D3D9Client 相机（从快照构造 `Scene::CAMERA`）；
  - 行星 `gpos/grot`（`vPlanet` 渲染缓存）；
- 需审计：`clbkPreStep/clbkPostStep`（物理线程调用）内不得访问渲染状态；物理查高程走 `ElevMgr`（不碰渲染地形树）；事件用无锁队列；`RenderSnapshot` 双缓冲原子发布。

### 路线 B：渲染线程化（原方向，需继续啃停滞问题）
- 排查第 4 节卡点 → 处理 D3D9 线程假设 → 3D 渲染线程 + 主线程 2D/Present 帧级设备交替；
- 工作量与风险都大于 A。

---

## 6. 关键代码位置

- 宏/线程编排：`Src/Orbiter/Orbiter.cpp`（`#define DUAL_THREAD_EXPERIMENT`、`RenderThreadProc`、`PhysicsThreadProc`、`Render2DEnvironment`、`Run()` 宏块）；
- 线程成员/快照结构：`Src/Orbiter/Orbiter.h`（`RenderSnapshot`、`m_renderSnap[2]`、`m_dual*` 原子、`m_dualMutex`）；
- 快照发布：`Orbiter::PublishRenderSnapshot()`（`UpdateWorld` 末尾调用，读 `g_camera`/`td`）；
- SDK 扩展：`Orbitersdk/.../GraphicsAPI.h`（`clbkRenderScene3D/2D` 默认实现）；
- D3D9Client 拆分：`D3D9Client.h/.cpp`（`clbkRenderScene3D/2D`）、`Scene.cpp`（`RenderMainScene` 3D / `RenderMainScene2D` 2D）、`Scene.h` 声明；
- 日志锁：`Src/Orbiter/Log.cpp`（`g_logMutex`）；
- 设计文档：`docs/handover/dual-thread-design.md`（线程模型/快照/同步/审计/风险/验收）。

---

## 7. 性能测量方法

- 配置：`install64/Orbiter/D3D9Client.cfg`：`PerfLog=2`（或 3）；`DebugLvl=0`；
- 日志：`install64/Orbiter/Orbiter.log` 中 `PERF CORE:`（fps/upd/rnd/scene…）与 `PERF UPD:`（物理细分：`ves/rb/grav/prop/subs/maxsub/dyn`）、`PERF fps=`（渲染 CPU 阶段：`sky/tDraw/atmo/plPost/...`）；
- 测试场景：`_perf_space`（轨道 DG）、`_perf_ground`（地面）、`_perf_night_orbit`（ISS 夜轨道）、`_perf_night_high`；
- 启动：`Orbiter.exe --scenariox="场景名"`（用独立进程测试，不碰用户的启动器/场景）。

---

## 8. 用户偏好（重要）
- 中文交流；先调查/定位再动手；不要自己截图；
- 部署/测试前确认游戏关闭，**不杀用户的启动器**；
- 不要动用户已调好的接触参数（`install64/Orbiter/Orbiter.cfg` 的 Contact* 等）；
- 构建 `D3D9Client`/`Orbiter` **必须 `--parallel 1`**；改 shader 后注意 **UTF-8 无 BOM**（BOM 会导致 HLSL 编译崩溃：`Illegal character in shader file`）；
- 构建环境偶发 `NO_PROXY/no_proxy` 环境变量大小写冲突导致 MSBuild 崩溃（`已添加项...NO_PROXY`）——处理：在同一 PowerShell 命令里用 `[Environment]::SetEnvironmentVariable('NO_PROXY',$null)` 等清理后再编译；
- 本地提交，不 push；改动先自测（启动不崩溃 + 日志无错 + 帧率）再交给用户实测。

---

## 9. 验收标准（双线程）
- ISS 旋转场景：10~20fps → **≥25fps**；
- 稳定/地面场景：不退化（≥30fps）；
- 仪表/MFD/对话框/HUD 正常；
- 无崩溃、无画面异常；
- 回归：地形黑线/阴影/残骸/汉化正常。
