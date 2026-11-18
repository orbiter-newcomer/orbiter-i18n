# Orbiter 性能优化交接文档

> 交接日期：2026-11-19
> 下一位接手人的任务：**渲染/整体帧率优化**（当前 14~16 fps @ 低空，用户只有核显）。

---

## 1. 当前状态（已完成、可用的部分）

### 1.1 地形 LOD 平滑过渡（morph）——已解决搏动问题
- 方案：**CPU 顶点插值**（不动顶点结构、不动 shader，避免云/球/旧地形 shader 不匹配崩溃）。
- 关键实现（`OVP/D3D9Client/Surfmgr2.cpp`）：
  - `SurfTile::PrepareMorph()`：**只在真实 LOD 切换时**（父瓦片从 ForRender 变 Active）调用；注意 **`morphHoldLevel` 粘滞机制**——每块瓦片记录"已 morph 过的最深层级"，同层级小抖动（LOD 判据在边界反复）**不再重复 morph**，这是"搏动"问题的最终解（用户已确认解决）；
  - 另外有 3 秒冷却作为第二道保险；
  - `SurfTile::UpdateMorph()`：首次调用时用**瓦片自身顶点的邻域低通**生成低清几何（`morphLow`），每帧插值到高清位置 `MapVertices` 上传，过渡结束后恢复高清并释放缓冲；
  - **只改瓦片内部顶点**（`1..res-1`），边缘顶点保持高清——避免瓦片边界撕裂；morph 期间 `IsMorphing()` 为 true，`ProcessNode` 里跳过 `MatchEdges`（否则两者抢顶点 → 抽搐/闪烁）。
- 配置：`TerrainTransitionMs`（毫秒，350 = 开启；**0 = 关闭 morph**）。
- 已知残留：**进场景加载阶段**偶发边缘撕裂（morph 与正在加载的瓦片交互，加载完成后消失，用户已接受）。

### 1.2 瓦片边缘匹配优化（效果有限）
- `SurfTile::MatchEdges()`（`Surfmgr2.cpp`）开头新增**邻居 LOD 签名**（`Tile::neighborSig`，`Tilemgr2.h`）：8 个邻居的层级打包成 64 位签名，签名不变则跳过匹配。
- `ProcessNode` 里**删除了每帧的 `tile->edgeok = false`**（`Tilemgr2_imp.hpp`），只在预加载/隐藏恢复分支（166/183 行附近）重置。
- 用户实测：**帧率提升不明显**（约 14→? 基本无感），说明大头不在这里。

### 1.3 地形加载优化（已生效）
文件：`Tilemgr2_imp.hpp::ProcessNode`、`TerrainLoadWait.inc`、`D3D9Config.cpp`。
- `TerrainPreloadLevels`（默认 1）：渲染需求前多预加载 1~2 级（含视锥外 Invisible 瓦片）；
- 视锥外子树保留：Invisible 且 idle>10s 才 `DelChildren`；
- LOD 滞回：`hystUp=0.55` / `hystDown=0.35`；
- 高程跳过：`TerrainElevMinLevel`（10）/`TerrainElevSkipAlt`（50000m）；
- 飞行兜底：`TerrainLoadWaitFlying`（1）/`TerrainLoadWaitPendingLimit`（12）；
- 加载等待：`TerrainLoadWait` / `TerrainLoadWaitTimeout` / `TerrainLoadWaitMinMs`；
- **并行加载已实现**（原版 `TileLoader` 多线程 + `TerrainLoadThreads` 配置 1~4，默认 1）。注意：多线程加载只加快**加载**，**不影响运行帧率**（渲染线程不等待加载）。

### 1.4 场景阴影（G1）
- `TerrainSelfShadow`（1）、`TerrainShadowing`（2=Projected）、`TerrainShadowMaxAlt`（100000m）、`TerrainShadowUpdate`（1=每帧）、`TerrainShadowSize/Range/Strength` 等配置见 `D3D9Config.h`（77~86 行）。
- 阴影每帧都会做一次地形树遍历 + 阴影贴图渲染（**可能是渲染 CPU 的大头之一，待测**）。

---

## 2. 性能现状（用户实测数据）

| 项目 | 数据 |
|---|---|
| 当前帧率 | **14~16 fps**（低空看地形，核显，3D 利用率没跑满） |
| 物理时步 0.1 → 0.2 | 帧率 **14 → 16**（约 +14%） |
| 推算 | **物理 ≈ 28% 帧时间（~20 ms）**，**渲染 ≈ 72%（~50 ms/帧）** |
| 瓶颈 | **渲染主线程单核**（物理+渲染在同一主线程顺序执行） |
| GPU | 未跑满 → GPU 不是瓶颈 |

**结论：渲染是大头，先优化渲染。物理/渲染分离在 Orbiter 里代价极高（见第 5 节），不建议。**

---

## 3. 下一步任务清单（按顺序做）

### 3.1 先定位渲染瓶颈（配置测试，用户配合测帧率）
修改 `install64/Orbiter/D3D9Client.cfg`，每项单独测（重启游戏、固定视角对比）：

| 配置 | 测试值 | 目的 |
|---|---|---|
| `TerrainShadowUpdate` | `2`（当前 1） | 阴影隔帧更新 |
| `TerrainSelfShadow` | `0` | 关地形自阴影 |
| `TerrainShadowing` | `0` | 关全部地形阴影（对照） |
| `TerrainShadowMaxAlt` | `30000`（当前 100000） | 高空不渲阴影 |
| `MaxTiles` | 调低（0.5~0.7） | 减少同时渲染瓦片数 |

各项帧率数字发给用户/记录后，判断大头是**阴影**还是**瓦片/Draw call**。

### 3.2 加内部计时（建议接手人做，最直接）
在主渲染循环里统计每帧 **Update（物理）** 与 **Render（渲染）** 的耗时：
- 位置：`Src/Orbiter/Orbiter.cpp::Run()` / `Render()`（`Simulation::Update` 与 `Scene::Render` 调用处）；
- 用 `oapiGetSysTime()` 或 `QueryPerformanceCounter`，每 2 秒 `oapiWriteLogV` 输出平均值（`ms Update / ms Render / fps`）；
- 注意：游戏内 FPS 计量器在暂停时不刷新，用户已试过暂停法不可用。

### 3.3 按定位结果优化（候选方向）
- **阴影大头**：把 `TerrainShadowUpdate` 默认改为 2~3 帧；或阴影覆盖范围（`TerrainShadowRange`）按高度自适应；或阴影 pass 减少投射物数量。
- **地形瓦片/Draw call 大头**：降低默认 `MaxTiles`、调整 `MaxPlanetLOD`/视距配置；较大的工程是地形合批（风险高，需谨慎）。
- **物理大头（当前证据 28%，不优先）**：物理时步已经是用户可调（0.1→0.2 有收益），不建议改代码。

---

## 4. 构建 / 部署 / 测试流程（必须遵守）

### 4.1 构建
```powershell
cmd /c "call ""D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"" x64 >nul 2>&1 && cd /d D:\AI_projet\orbiter\orbiter && cmake --build build64 --config Release --target D3D9Client --parallel 1"
```
- **必须 `--parallel 1`**。
- 改 shader 后必须用 fxc 全 entry 验证：`D:\Windows Kits\10\bin\10.0.22621.0\x64\fxc.exe`（参考 `NewPlanet.hlsl` 的 entry 组合）。

### 4.2 部署（**部署前必须确认游戏和启动器都已关闭**；启动器也会锁 DLL，**不要杀用户的启动器**）
- DLL：`orbiter\build64\Modules\Plugin\D3D9Client.dll` → `install64\Orbiter\Modules\Plugin\`
- Shader：`orbiter\OVP\D3D9Client\shaders\*` → `install64\Orbiter\Modules\D3D9Client\` **和** `build64\Modules\D3D9Client\`
- 配置：`install64\Orbiter\D3D9Client.cfg`

### 4.3 自动测试（**不要手动进场景/开游戏**，用户明确要求过：会破坏他自己的场景）
```powershell
powershell -ExecutionPolicy Bypass -File "D:\AI_projet\orbiter\temp\capture_shadow_test.ps1" -Scenario '_terraintest_ksc' -OutFile "D:\AI_projet\orbiter\temp\test.png" -WaitAfterTerrainMs 6000 -WantShots 2 -ShotIntervalMs 3000 -TimeoutSec 45
```
- 脚本带**看门狗**：进程卡死（模拟开始后 10s 无响应）或超时（默认 40s）→ **自动 kill 游戏 + FAIL**；
- 正常加载约 15~25 s（KSC/Alps），看门狗不会误报；
- 场景：`_terraintest_ksc`（KSC 海岸）、`_terraintest_alps`（Alps 山脉）、`_terraintest_high`（高空）。
- 测试完用 `read_image` 看截图确认地形正常（无消失/条带/撕裂）。

---

## 5. 专项分析结论（不用重复做）

### 5.1 物理/渲染分离（用户提议过，**不做**）
- Orbiter 的物理（`Simulation::Update`）和渲染（`Scene::Render`）在**同一主线程**顺序执行；
- 渲染模块/插件（D3D9Client 等）**直接读 `VESSEL` 等共享对象**（不是快照），物理线程化 → 数据竞争；加锁 = 顺序执行无收益；改快照架构 = 重写全部模块渲染接口；
- 即使成功，物理只占 28%，理论帧率上限约 20 fps（+40%），**投入产出比极低**。

### 5.2 任务级并行（阴影/morph/边缘匹配分核，**暂不做**）
- D3D 调用（`MapVertices`、阴影 Draw、状态设置）必须留在渲染线程（D3D9 设备非线程安全）；
- 工作线程只能做纯 CPU 计算，收益取决于计算量占比，而边缘匹配优化已证明"不是大头"；
- 如后续确认阴影遍历是大头，可先尝试**只读的阴影收集**放到工作线程（风险相对低）。

---

## 6. 当前部署状态（交接时）

- `install64/Orbiter/Modules/Plugin/D3D9Client.dll`：2026-11-19 05:05 构建版本（含 morph 死区 + 边缘匹配签名优化 + 阴影/加载全部改动）；
- `D3D9Client.cfg` 关键值：`TerrainTransitionMs=350`（morph 开）、`TerrainPreloadLevels=1`、`TerrainShadowMaxAlt=100000`、`TerrainShadowUpdate=1`、`TerrainSelfShadow=1`、`TerrainLoadThreads=1`；
- 用户已确认：**搏动问题解决**（morph 死区生效）、撕裂只在加载阶段短暂出现。

## 7. 用户偏好（重要）
- 中文交流；先调查/定位再动手；
- **不要自己启动游戏进场景**（用 `capture_shadow_test.ps1` 自动测试并截图）；
- 部署前确认游戏/启动器已关闭，**不杀用户的启动器**；
- 构建 `--parallel 1`；改 shader 必须 fxc 验证；
- 改动先离线验证、自测通过再交给用户上线（用户会一轮轮实测反馈）。
