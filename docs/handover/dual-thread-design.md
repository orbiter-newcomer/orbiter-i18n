# 物理/主场景渲染双线程改造设计

> 目标：把 **物理求解（Simulation::Update，含相对论传播/刚体/重力）** 与 **主场景 3D 渲染** 拆到不同线程并行执行。
> 仪表 / MFD / 对话框 / HUD（2D 部分）保持原样（主线程）。
> 依据实测：物理 10~28ms（旋转/复杂姿态时 20~28ms），渲染 4~22ms → 并行后理论 **+50%~2 倍**。

---

## 1. 线程模型

| 线程 | 职责 | 禁止 |
|---|---|---|
| **物理线程** | `Orbiter::UpdateWorld()`：`Simulation::Update`、相对论传播、刚体、子系统、`clbkPreStep/clbkPostStep`、`vessel->PostStep` 等；**每步导出主场景快照** | 不碰 D3D 设备、不调渲染 API |
| **渲染线程** | 主场景 3D：`Scene::RenderMainScene` 中的 3D 部分（行星/地形/飞船/云/大气/星空/阴影/后处理） | 不写物理对象，只读快照 |
| **主线程**（原线程） | 输入、UI、仪表/MFD/对话框、2D HUD/覆盖、**Present**、帧节拍协调 | 与渲染线程交替使用 D3D9 设备（帧级串行） |

### 帧节拍
```
物理线程：  UpdateWorld()  [快照A]  →  UpdateWorld()  [快照B]  → ...
渲染线程：  读快照A → 渲染3D（BeginScene..EndScene）→ 通知主线程
主线程：    2D/UI 绘制 + Present（同一设备，帧级交替）
```
- 快照双缓冲（物理写 A、渲染读 B，交换），**渲染滞后物理 1 帧**（可接受）；
- 物理线程可按固定步长连续跑（不受渲染帧率限制）；渲染线程每次取最新完整快照。

---

## 2. 快照结构（阶段 1 最小集）

`src/Orbiter/RenderSnapshot.h`（新增）：

```cpp
struct VesselSnapshot {
    OBJHANDLE hVessel;
    VECTOR3   gpos;        // 全局位置
    MATRIX3   grot;        // 全局旋转
    VECTOR3   vrot;        // 角速度（渲染插值可选）
    double    alt;         // 高度
    bool      visible;     // 可见/激活
    // 阶段2: 引擎/灯光/动画状态
};

struct RenderSnapshot {
    double  simt, simdt, mjd;
    CameraSnapshot cam;                 // 相机（pos/朝向/投影参数/lng/lat/elev）
    std::vector<VesselSnapshot> vessels;
    // 阶段2: 光照（sun dir/亮度）、事件队列（爆炸/残骸/切换）
};
```

- 物理线程每步填充新快照（写操作仅完成对象内部与快照，不发布）；
- 通过 **原子指针/双缓冲 + 互斥量** 发布；渲染线程读取当前最新快照。
- 渲染线程的 `vVessel` 渲染路径：**位置/姿态从快照取**（对象本身只用于网格/材质等只读数据）。
- 阶段 1 只快照相机 + vessel 位置/姿态/可见性；其余渲染数据（地形/云/大气/阴影）归渲染线程私有。

---

## 3. D3D9 设备跨线程

- D3D9 设备**非线程安全**，但主线程 2D（仪表/MFD/HUD/对话框）和渲染线程 3D 共用同一个设备。
- 方案：**帧级交替**（安全且简单）：
  - 渲染线程：`BeginScene → 3D 渲染 → EndScene（不 Present）`；
  - 主线程：获得设备锁 → 2D/UI 绘制 → `Present` → 释放锁；
  - 2D 耗时 <1ms（实测 `out2d=0.00 imgui=0.5ms`），对整体影响小。
- 设备锁：`std::mutex`；锁粒度 = 一次 3D 帧 / 一次 2D+Present。

---

## 4. 改动文件清单（预估）

| 文件 | 改动 |
|---|---|
| `Src/Orbiter/Orbiter.cpp` | `Run()`：启动物理线程与渲染线程；原主循环变为主线程（2D+Present）；`UpdateWorld`/`Render` 拆分 |
| `Src/Orbiter/RenderSnapshot.h/.inc`（新增） | 快照结构与双缓冲发布 |
| `Src/Orbiter/Simulation.cpp/.h` | 物理线程边界、快照导出钩子（`UpdateWorld` 末尾） |
| `OVP/D3D9Client/Scene.cpp` | `Scene::RenderMainScene` 拆分为 3D（渲染线程）与 2D（主线程）；相机读取走快照 |
| `OVP/D3D9Client/D3D9Client.cpp` | 设备锁、Present、clbk 回调归属调整 |
| `OVP/D3D9Client/VVessel.cpp` 等 | vessel 渲染位置/姿态从快照读取 |
| `OVP/D3D9Client/VPlanet.cpp` | 相机/时间从快照读取（地形/云内部逻辑不变） |

---

## 5. 审计清单（线程安全）

| 项目 | 说明 |
|---|---|
| `clbkPreStep` / `clbkPostStep`（物理线程调用） | 检查所有模块（尤其 D3D9Client 的 clbk）回调内不访问 D3D 设备/渲染状态；把"渲染相关"工作移到渲染线程 |
| `vessel->PostStep` / 废船/残骸更新 | 保持物理线程；渲染只读快照 |
| `Scene::GetCamera*` / `Camera` | 渲染线程读快照，不直接读 `Scene::Camera`（物理/相机模块会写） |
| `oapi*` API | 渲染线程只允许只读查询（快照已提供所需数据）；物理线程不调渲染 API |
| 地形 `SurfMgr2` / 云 `CloudMgr2` | 渲染线程私有（加载线程已有自己的锁）；物理查高程走 `ElevMgr`，不碰渲染地形树 |
| 日志输出 | `oapiWriteLogV` 内部加锁（官方已线程安全），保持 |
| 事件（爆炸/残骸/网格切换） | 物理线程 push 到无锁队列，渲染线程消费 |

---

## 6. 分阶段计划

### 阶段 1：最小闭环（先验证帧率收益）
1. 新增快照结构（相机 + vessel 位置/姿态/可见性）；
2. `Orbiter::Run`：物理线程跑 `UpdateWorld`（导出快照）、渲染线程跑 3D `Scene::RenderMainScene`；
3. 主线程：2D/UI/Present + 设备锁；
4. **验证目标**：ISS 旋转场景 10~20fps → **≥25fps**；稳定场景不退化（30~40fps）；
5. 自测：不崩溃、画面正常、仪表/MFD 正常、回归测试（地形/阴影/残骸）。

### 阶段 2：快照扩展
- 光照/太阳方向、事件队列（爆炸/残骸/切换）、引擎/动画状态、插值（1 帧延迟的平滑）。

### 阶段 3：2D 数据快照
- 仪表/MFD 读取的数据也走快照（彻底消除 UI 与物理的竞争）。

---

## 7. 风险与回退

| 风险 | 缓解 |
|---|---|
| 竞态崩溃（难查） | 阶段 1 先加"全局大锁"跑通（正确性优先），再按模块缩小锁范围；开启 Debug 构建断言 |
| D3D9 设备状态错乱 | 帧级交替 + `RecallDefaultState`；每线程只用自己的渲染阶段 |
| 第三方插件不兼容 | 本 fork 自用；文档记录 |
| 性能收益不及预期 | 阶段 1 达成即验证；否则回退（保留分支） |
| 渲染线程与加载线程竞争 | 地形加载线程保持不变（已有锁）；渲染线程等待加载就绪（沿用 `TerrainLoadWait` 逻辑） |

## 8. 验收标准
- ISS 旋转场景：fps 从 10~20 → **≥25**；
- 稳定场景/地面：不退化（30~40fps）；
- 仪表/MFD/对话框/HUD 功能正常；
- 无崩溃、无画面异常；
- 回归：地形黑线/阴影/残骸/汉化均正常。
