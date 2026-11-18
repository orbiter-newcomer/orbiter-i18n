# 船身阴影交接文档（Handover：Vessel Shadows）

> 交接对象：**下一条鱼**（接下来要大改地形渲染的会话）。
> 目的：把当前"船身阴影 / 地形接收阴影"的实现、依赖、已知限制和易踩的坑交接清楚，
> 避免大改地形渲染时把阴影系统、地形加载等待等已有功能弄坏。
> 编写时间：2026-11-17。仓库：`D:\AI_projet\orbiter\orbiter`（分支 `main`）。

---

## 0. 仓库与构建状态（先看这一节）

### 0.1 Git 状态（重要）
- 远端 `origin/main` 当前包含 `fe782555`（残骸开关绑定）。
- **本地有 2 个未推送提交**（用户明确要求：**未经许可不要推送**）：
  - `31b5956c` 地形加载暂停：新场景创建时确定性重置等待状态
  - `5c0c044f` 地形加载暂停稳定可见：已暂停不跳过 + 最小等待时长（`TerrainLoadWaitMinMs`）
- 以后所有改动**只做本地提交**；推送前必须得到用户明确许可。

### 0.2 构建
- VS2022 x64，CMake 工程目录 `build64`，Release。
- 必须 `--parallel 1`（否则 C1060 堆内存错误）：
  ```
  call "<VS>\VC\Auxiliary\Build\vcvarsall.bat" x64
  cd /d D:\AI_projet\orbiter\orbiter
  cmake --build build64 --config Release --target D3D9Client --parallel 1
  cmake --build build64 --config Release --target Orbiter --parallel 1
  ```
- 安装目录：`install64\Orbiter\`（D3D9Client 在 `Modules\Plugin\D3D9Client.dll`）。
  改 D3D9Client 后要手动复制 DLL 到 install64，否则测不到新代码。

### 0.3 项目原则（务必遵守）
- **官方源码尽量少改**：功能放到 `.inc` 钩子文件里（D3D9Client 里已有
  `TerrainRender.inc`、`TerrainLoadWait.inc`、`TextCodec.inc` 等），官方 cpp 只留 hook 点。
- 汉化/i18n 不要随意改官方 cpp；SDK API 优先。
- 本地只提交、不推送（见 0.1）。

---

## 1. 船身阴影系统总览

当前是**两套并存的阴影体系**，由 D3D9Client 的配置项控制：

| 体系 | 配置项 | 说明 |
|---|---|---|
| 旧式投影/模板阴影（legacy） | `TerrainShadowing`（0=None / 1=Stencil / 2=Projected） | 把飞船轮廓投影到地面，**参考面**用实际渲染地形/基地平面（我们改过，见 §3.1） |
| Shadow Map 自阴影（新式） | `ShadowMapMode`（0=None / 1=Focus+payload / 2=Near by objects / 3=All visible）、`ShadowMapSize`、`ShadowFilter` | 把飞船渲染进深度阴影贴图，飞船/地形/基地接收 |

**船身阴影（ship hull shadow）主要指第二套**：vessel 网格 → shadow map → 地形/飞船接收。
`TerrainShadowing == 2`（Projected，默认值）时，地形通过 shadow map 接收船身阴影。

---

## 2. 关键文件与调用链（精确位置）

### 2.1 Shadow Map 的初始化
`OVP/D3D9Client/Scene.cpp`
- **212–236 行**：`vScene` 初始化时创建 shadow map 资源：
  - `SHM_LOD_COUNT = 5`（Scene.h:43）级 LOD 的深度模板 surface（`psShmDS[]`）与 R32F 渲染目标（`ptShmRT[]`/`psShmRT[]`）
  - 尺寸从 `Config->ShadowMapSize` 逐级减半
  - `smap.pShadowMap = ptShmRT[0]`

### 2.2 每帧：生成 shadow map
`OVP/D3D9Client/Scene.cpp`
- **1534–1545**：构建 `Casters` 列表（**只收 `OBJTP_VESSEL`**）。
- **1549–1630**：为焦点船（`vFocus`）提前渲染 shadow map（供地表基地/行星渲染使用）：
  - `bouble_rad = 10.0f`（地形阴影覆盖半径，米）
  - **wreck 残骸**作为焦点时覆盖半径放宽到 `200.0f`（残骸散落范围大，见注释 1556–1559）
  - 把 `vFocus->bStencilShadow = false`（避免和旧式投影阴影重复）
  - 附近小物体也加入 `SmapRenderList`（`bs_rad > 80` 跳过、距离 > 1e3 跳过等筛选）
- **1843–1867**：先渲染"在阴影里"的飞船（`IsInsideShadows()`）。
- **1871–1915**：`ShadowMapMode >= 2` 时再为附近物体逐个渲染 shadow map。
- **`Scene::RenderShadowMap`（2536–2629）**：核心函数
  - 入参 `pos`（焦点船包围球中心）、`ld`（太阳方向）、`rad`（包围球半径）
  - 计算正交投影 `mProj`（覆盖 `[-rad, rad]`）、`mView`、`mViewProj`
  - 根据 `rsmax` 计算 LOD（`Config->ShadowMapSize` / 1.5 系数，`smap.lod` 限制在 `SHM_LOD_COUNT-1`）
  - `RENDERPASS_SHADOWMAP` pass 下**只渲染 `SmapRenderList`（vessel）**
  - **注意：地形不在这里的投射者列表里** → 地形自身不投影（§5.1）

### 2.3 vessel 网格渲染进 shadow map
`OVP/D3D9Client/VVessel.cpp`
- **789 行**：`RENDERPASS_SHADOWMAP` 时 `mesh->RenderShadowMap(pWT, pLVP, 0)`
- **790–793**：`RENDERPASS_NORMAL_DEPTH` 时 `RenderShadowMap(pWT, pVP, 1)`（法线深度缓冲）

`OVP/D3D9Client/Mesh.cpp`
- **2733–2800+**：`D3D9Mesh::RenderShadowMap(pW, pVP, opt)`
  - `opt==1` → `SHADER_NORMAL_DEPTH`（屏幕空间法线深度）
  - 否则 → `SHADER_SHADOWMAP`（普通自阴影）或 `SHADER_SHADOWMAP_OIT`（带 OIT 的组，`Grp.UsrFlag & 0x20`）
  - 顶点缓冲：`pGB`（`D3DXVECTOR4` 位置）或 `pSB`（SMVERTEX）
  - 跳过 `UsrFlag & 0x1/0x2` 的组（无阴影/特殊组）

阴影 map 顶点/像素着色器：`OVP/D3D9Client/shaders/NewMesh.hlsl`
- `ShdMapVS` / `ShdMapPS`（约 102–123 行）
- 技术名 `SHADOWMAP` 宏（`D3D9Effect.cpp` 327 行附近 `macro[3].Name = "SHDMAP"`）

### 2.4 地形接收船身阴影
`OVP/D3D9Client/VPlanet.cpp`
- **924**：`const Scene::SHADOWMAPPARAM *shd = scn->GetSMapData();`
- **964–973**：`shd->pShadowMap && RENDERPASS_MAINSCENE && TerrainShadowing == 2` 时
  - `eLVP`（光照视图投影矩阵）、`eShadowMap`（阴影贴图）、`eSHD`（`(size, 1/size, 1/shadowmapsize, 0)`）
  - `eShadowToggle = true`

`OVP/D3D9Client/Surfmgr2.cpp`
- **995–1017**：`SurfTile::Render` 中同样把 `shd->pShadowMap` 传到 `PlanetShader` 的 `tShadowMap`
- 条件：`ShadowMapMode != 0 && TerrainShadowing == 2`

`OVP/D3D9Client/shaders/NewPlanet.hlsl`（**地形渲染主体，下一条鱼要大改的就是这个**）
- `TileVS`（42–51）：`#if defined(_SHDMAP)` 时增加 `float4 shdH : TEXCOORD3;`
- `TerrainVS`（321–371）：输出 `shdH`（`#if defined(_SHDMAP)`，行 362–364）
- `SampleShadows`（**144–170**）：9 次 `tex2D(tShadowMap, ...)` 采样，返回 0..1
- `TerrainPS`（389 起）中 432–442 行：
  ```hlsl
  float fShadow = 1.0f;
  #if defined(_SHDMAP)
  if (Flow.bShadows) { ... fShadow = 1.0f - SampleShadows(sp, pd); }
  #endif
  ```
- 最终光照（`_NO_ATMOSPHERE` 分支 583–610 / 大气分支 611–700+）里 `fShadow` 参与漫反射合成

### 2.5 飞船接收阴影（自阴影/其他船）
- `OVP/D3D9Client/shaders/Vessel.fx`、`Mesh.fx`、`NewMesh.hlsl` 中均有 `SHDMAP` 相关分支
  （`SampleShadows`/`ComputeShadow` 在 `shaders/Common.hlsl` 352–385 行，多档过滤 MSAA 采样）
- `Scene::RenderMesh`（Scene.cpp 3050 起）给 mesh 设置 `eLVP/eShadowMap/eSHD/eShadowToggle`

### 2.6 旧式投影/模板地面阴影（TerrainShadowing 0/1/2 的 legacy 部分）
`OVP/D3D9Client/Scene.cpp`
- **3026–3045**：`Scene::RenderVesselShadows(hPlanet, depth)` → 遍历所有 vessel 调用 `vVessel::RenderGroundShadow`，然后渲染粒子阴影。

`OVP/D3D9Client/VVessel.cpp`
- **21 行**：`#include "TerrainRender.inc"`（我们项目的功能钩子）
- **1164–1168**：注释说明投影阴影实现已移到 `TerrainRender.inc`

`OVP/D3D9Client/TerrainRender.inc`（**我们项目文件，重要**）
- `TerrainRender_SkipGroundShadow()`：`oapiGetVesselShadowMode()` 为 `SHADOWMODE_NONE` 或无该阴影时跳过
- `vVessel::UpdateShadowPlane(hPlanet)`：计算投影阴影参考面
  - **参考面来自"实际渲染的表面"**：在基地上方用基地平面（跑道/停机坪），否则用**渲染器地形高程**（`vPlanet::GetElevation`）+ 官方法线 API（`oapiSurfaceElevationEx`）
  - 每船缓存（位置/时间失效重算）
- `vVessel::RenderGroundShadow(dev, hPlanet, alpha)`：真正投影渲染

---

## 3. 地形渲染当前结构（大改对象）

> 下一条鱼要"大改地形渲染"——本节说明现有地形渲染入口，方便替换/改造。

### 3.1 地形渲染入口
- `SurfTile::Render()`：`OVP/D3D9Client/Surfmgr2.cpp` **761 行起**（主力入口）
  - 取 `PlanetShader *pShader = mgr->GetShader();`、`ShaderParams *sp = vPlanet->GetTerrainParams();`
  - 设置 `sp->vMSc[]`（microtexture 范围）、`sp->vCloudOff` 等参数
  - 处理 `has_specular/has_lights/has_shadows/has_microtex/has_atmosphere/has_ripples`
  - 绑定 shadow map（995–1017，见 §2.4）
  - 调用 `PlanetShader` 绘制
- `PlanetShader` / `ShaderParams`：`OVP/D3D9Client/VPlanet.h`
  - `class PlanetShader : public ShaderClass`（42 行）
  - `struct ShaderParams`（81 行）——地形着色器参数结构
- 着色器：`OVP/D3D9Client/shaders/NewPlanet.hlsl`
  - 地形 technique 用 `TerrainVS` / `TerrainPS`（`VPlanetAtmo.cpp` 148/153/168 行分别编译 Earth/Mars/Moon）
  - 其他行星类型用 `GiantVS/GiantPS`、`CloudVS/CloudPS` 等
- 宏定义：`D3D9Effect.cpp` 的宏表（`SHDMAP`、`_MICROTEX`、`_CLOUDSHD`、`_WATER`、`_NIGHTLIGHTS`、`_LOCALLIGHTS`、`_NO_ATMOSPHERE` 等，`ShaderClass` 按 flags 组合编译）

### 3.2 地形顶点数据
- `Tile::CreateMesh_quadpatch`：`OVP/D3D9Client/Tilemgr2.cpp` **420 行起**
  - 顶点结构 `VERTEX_2TEX`：位置 + **法线 nx/ny/nz**（从高程计算，行 502–504）+ `e`（高程）+ 纹理坐标
  - 所以地形**逐顶点法线是现成的**（这也是地形坡面光照/自投影的数据基础）
- 地形高程查询：`vPlanet::GetElevation(lng, lat, &elev, ...)`（Surfmgr2.cpp 1591、Surfmgr2.h 35）
  - `TerrainRender.inc` 的投影阴影参考面依赖它 → **大改时保留该 API**

### 3.3 地形加载等待（我们最近改过的功能，容易被弄坏）
- `OVP/D3D9Client/TerrainLoadWait.inc`（D3D9Client.cpp:30 include；1265–1267 每帧调用）
  - `UpdateTerrainLoadWait()`：进入模拟时暂停直到相机周围地形瓦片加载完
  - `DrawTerrainLoadWaitLabel()`：屏幕显示"Loading terrain..."
  - 依赖（**大改地形渲染时不要破坏**）：
    - `TileManager2<SurfTile>::TileTree(0/1)`（Tilemgr2.h:457 的 1 行 getter）
    - `Tile::State()`（`Tile::InQueue` / `Tile::Loading`，Tilemgr2.h）
    - `vPlanet::SurfMgr2()`、`scene->GetCameraProxyVisual()`
  - 最近修复（本地提交 `31b5956c`、`5c0c044f`）：
    - `D3D9Client::ResetTerrainLoadWait()` 在 `clbkPostCreation()` 中重置状态机（每次新场景确定性触发）
    - 已暂停时不再跳过等待（仅不接管暂停状态）
    - 新增 `TerrainLoadWaitMinMs`（默认 600ms，cfg 键 `WaitTerrainLoadMinMs`）：缓存地形也保证暂停可见
  - 诊断日志：`TerrainLoadWait: waiting for terrain...` / `done after N s`

### 3.4 地形相关配置（D3D9Client.cfg）
| 键 | 默认 | 范围 | 说明 |
|---|---|---|---|
| `ShadowMapMode` | 1 | 0–3 | 自阴影模式（None / Focus+payload / Near by objects / All visible） |
| `ShadowMapSize` | 2048 | 512–4096 | 阴影贴图尺寸 |
| `ShadowMapFilter` | — | 0–5 | 阴影过滤（9/27/27s dither 等） |
| `TerrainShadowing` | 2 | 0–2 | None / Stencil / Projected（Projected 时地形接收 shadow map） |
| `WaitTerrainLoad` | 1 | 0–1 | 地形加载等待开关 |
| `TerrainLoadWaitTimeout` | 30 | 1–120 | 等待超时（秒） |
| `TerrainLoadWaitMinMs` | 600 | 0–5000 | 最小等待时长（毫秒，新增） |
| `WaitTerrainLoadCamera` | 1 | 0–1 | 相机切换时是否重新等待 |

配置读写：`OVP/D3D9Client/D3D9Config.cpp`（默认值 ~40–60 行；读取 ~145–165 行；写入 ~245–265 行）；UI 在 `VideoTab.cpp`（Self shadows / Shadow filter / Terrain shadowing 下拉框，714–730、821–823、897–899）。

---

## 4. 大改地形渲染时的"必须保留清单"

1. **阴影接收链路**：`_SHDMAP` 宏、`TileVS.shdH`、`TerrainPS` 中 `SampleShadows` 分支、`eLVP/eShadowMap/eSHD/eShadowToggle` 参数绑定（`Surfmgr2.cpp` 995–1017 / `VPlanet.cpp` 964–973）。
2. **地形高程/法线 API**：`vPlanet::GetElevation(...)`、`VERTEX_2TEX` 的逐顶点法线——`TerrainRender.inc`（投影阴影参考面）依赖它们。
3. **地形加载等待依赖**：`TileManager2::TileTree()`、`Tile::State()`、`vPlanet::SurfMgr2()`——`TerrainLoadWait.inc` 依赖。
4. **旧式投影阴影**：`TerrainRender.inc` 的 `vVessel::RenderGroundShadow/UpdateShadowPlane`、`oapiGetVesselShadowMode`、
   `SHADOWMODE_NONE/SHADOWMODE_SHADOWMAP`（残骸走 shadow map、不投地面阴影）。
5. **相对论地形着色**：`RelativityShaders.hlsl` / `Relativity.hlsl` / `NewPlanet.hlsl` 中的相对论分支（重力透镜、光行差、多普勒）——大改时别把相对论路径删掉。
6. **官方文件最小化**：优先在 `TerrainRender.inc` / `TerrainLoadWait.inc` 等钩子文件中实现新逻辑；官方 cpp 只加一行调用。
7. **D3D9Client 关闭顺序**（`Orbiter.cpp` CloseSession，提交 `5792ab16`）：插件 `clbkSimulationEnd` 已提前到 `clbkDestroyRenderWindow` 之前——改地形/资源释放时不要在"图形服务关闭后"再释放设备资源。

---

## 5. 已知限制与可扩展方向

### 5.1 地形自身不投射阴影（现状）
- shadow map 投射者**只有 vessel/残骸**（`Scene.cpp` 1538–1543 `Casters` 只收 `OBJTP_VESSEL`；`RenderShadowMap` 只渲染 `SmapRenderList`）
- 所以：**山体不会把阴影投到相邻坡面/山谷**。坡面明暗只有太阳方向的光照（`NewPlanet.hlsl` 的 `fZ`/`fD`，随太阳实时变化），没有地形自投影。
- 若要地形自投影（下一条鱼大改地形时的候选方向）：
  - 把地形瓦片加入 shadow map 投射者（`RENDERPASS_SHADOWMAP` 渲染地形深度）
  - 覆盖范围要放大（当前 10m，wreck 200m；地形需要数百米~数公里）→ 大范围/级联 shadow map；性能风险高
  - 需要处理自阴影 acne/bias（陡坡）、LOD 与阴影一致性
  - 地形接收端已有 `_SHDMAP` 采样代码可复用，但 9-tap 采样/覆盖范围要同步调整

### 5.2 船身阴影的覆盖范围
- 阴影 map 只覆盖焦点船周围 ~10m（wreck 200m）→ 远处地形不接收船身阴影
- 大物体（`bs_rad > 80`）不参与、距离 > 1e3 不参与

### 5.3 旧式投影阴影的参考面
- 已在 `TerrainRender.inc` 中修复为"实际渲染地形/基地平面"（避免高分辨率地形上阴影漂浮/错位）
- 若大改地形渲染改变高程语义（例如新地形网格/缩放），`UpdateShadowPlane` 里的高程/法线查询逻辑需要复查

---

## 6. 测试与调试

1. **配置页**（D3D9Client 设置 → 视频页）：
   - Self shadows：None / Focus + payload / Near by objects / All visible
   - Shadow filter：9 / 27 / 27s dither
   - Terrain shadowing：None / Stencil / Projected
2. **场景**：DeltaGlider 低空/着陆（看船身阴影落到地形）、残骸撞击（shadow map 200m 覆盖）、基地停机坪（投影参考面）。
3. **日志**：`install64\Orbiter\Orbiter.log`（D3D9 初始化、shadow mapping 能力、TerrainLoadWait 行）。
4. **回归点**：
   - 地形加载等待（每次进入模拟都应暂停，重复加载同一场景也至少 600ms）
   - D3D9 关闭无 `UnDeleted Surfaces`（见提交 `5792ab16`）
   - 残骸和地形阴影不重复/不漂浮

---

## 7. 本会话最近的相关改动（供追溯）

| 提交 | 内容 |
|---|---|
| `31b5956c`（本地未推） | TerrainLoadWait 新场景确定性重置（`ResetTerrainLoadWait`） |
| `5c0c044f`（本地未推） | TerrainLoadWait 已暂停不跳过 + 最小等待时长 `TerrainLoadWaitMinMs` |
| `fe782555`（已推） | 残骸系统开关绑定启动器"损伤与故障模拟"（`WreckSystemEnabled()`） |
| `5792ab16`（已推） | D3D9 泄漏修复：关闭顺序（clbkSimulationEnd 提前）、DG 纹理引用计数、DrawOrbits 资源释放 |
| `b891ad33`（已推） | 日志调试残留清理（残骸 VORONOI 输出） |
| `8d597507` | 官方文件最小化重构：地形渲染/地形加载等待/残骸模块拆分（`TerrainRender.inc`、`TerrainLoadWait.inc` 由此而来） |
| `158c90a4` | 地形加载等待功能最初实现 + 相机切换规则 |

---

## 8. 给下一条鱼的建议

1. **先跑一遍**：构建 D3D9Client → 复制到 `install64` → 跑 DG 低空场景 + 残骸撞击，确认当前船身阴影/地形加载等待正常，再开始大改。
2. **改地形渲染时**：保留 `_SHDMAP` 接收链路与 `TerrainRender.inc`/`TerrainLoadWait.inc` 依赖的接口（§4），否则阴影/加载等待会一起坏。
3. **地形自投影**（如果要做）：复用现有 shadow map 框架，重点解决"覆盖范围/分辨率/性能"和"bias"（§5.1）。
4. **提交策略**：只本地提交，不推送（用户明确要求；见 §0.1）。

---

## 9. v2 变更：阴影与地形渲染绑定（2026-11-18，本次会话）

### 9.1 用户需求
> "能否把阴影跟地形渲染绑定：地形啥时候开始渲染，阴影就啥时候开始渲染；地形用啥参数和分辨率，阴影就用啥参数和分辨率。"

### 9.2 已实现
- **时机绑定**：G1 阴影在 `TileManager2<SurfTile>::Render` 内部生成（`ProcessNode` 之后、`RenderNode` 之前），与地形渲染**同帧同流程**；地形瓦片开始渲染时阴影贴图已就绪，`SurfTile::Render` 直接绑定采样（`Surfmgr2.cpp`）。
- **覆盖范围绑定**：`Scene::RenderSceneShadow` 用新静态函数 `SceneShadowCoverageNode()` 遍历**当前地形渲染树**，用 `Scene::IsVisibleInCamera()` 收集**屏幕可见瓦片**，把它们的包围球合并成 AABB → 阴影覆盖中心/半径 = 可见地形范围（+500m 余量）。完全找不到可见瓦片时回退 `alt*6`（`TerrainRender.inc`）。
  - 效果：地形渲染哪个范围、多大范围，阴影就覆盖同样范围（包括远距离低 LOD 瓦片）。
- **分辨率绑定**：阴影贴图尺寸默认 = **back buffer 尺寸的 2 次幂**（1024–4096；2048x1152 视口 → 2048 贴图；`TerrainShadowSize > 0` 时用配置值）。`ShadowSystem::EnsureSize()` 每帧检查，窗口分辨率变化时自动重建贴图（`TerrainRender.inc`）。
- **渲染状态恢复**（重要回归修复）：阴影 pass 结束 `RecallDefaultState()` + `TileManager2::Render` 中重新 `pShader->Setup()`。**没有这两步时地形会整片消失**（地形用阴影深度 shader 绘制 / 状态残留）。
- **正常渲染阴影可见性**：阴影区域除直射光外，环境光也乘 `lerp(1, fShadow, 0.5)`（`NewPlanet.hlsl` TerrainPS），避免阴影只压直射光导致看不出来。

### 9.3 配置（`install64\Orbiter\D3D9Client.cfg`）
| 键 | 值 | 说明 |
|---|---|---|
| `TerrainSelfShadow` | 0/1 | 统一场景阴影（G1）总开关，默认 1 |
| `TerrainShadowRange` | 0 / 300..60000 | 0=自动（=可见地形范围）；>0 固定覆盖半径 |
| `TerrainShadowSize` | 0 / 512..4096 | 0=自动跟随视口分辨率；>0 固定贴图尺寸 |
| `TerrainShadowDebug` | 0/1 | **1=调试可视化**：蓝=覆盖范围外，黄=覆盖内无遮挡，红=阴影（橙黄渐变=fShadow 中间值）；0=正常渲染 |
| `TerrainShadowStrength` | 0..100 | 预留（未接线） |

### 9.4 已验证
- `TerrainShadowDebug=1` 调试图：黄色覆盖整个视野地形直到地平线，蓝色只在天空 → 覆盖范围与地形渲染一致。
- 日志无 shader 编译错误、无崩溃；`ShadowSystem: scene shadow map 2048x2048`（视口 2048x1152）。
- `TerrainShadowDebug=0` 正常渲染：地形正常显示，阴影进入光照计算。

### 9.5 注意
- 阴影分辨率跟随视口：**窗口越大越贵**（4096² 时 R32F+DS 约 112MB 显存）。性能不足可设 `TerrainShadowSize=2048`。
- 覆盖范围跟随可见瓦片：**远视距场景覆盖会变大、分辨率相应下降**；远场更高质量需要 CSM/多级覆盖（后续可做）。
- `TerrainShadowDebug` 是调试模式，正式运行应设 0。
- 本次会话同时修复：D3D9Client.fx 的 ps_3_0 **bool 常量寄存器超限崩溃**（删除新增 `gShadowsEnabled2`/`gBaseBuilding` 分支，改用 `gSHDT2.w <= 0` 判断）；**地形消失**（shader 状态恢复）；覆盖范围从 6000m 扩到可见地形范围。
---

## 10. 地形加载策略 v2（2026-11-18，本会话）

### 10.1 背景与需求
用户要求：加载并行化 + 平滑过渡、提前一个层级加载并视野内统一、地形清晰度低于阈值时不加载高程、加载不过来时等待兜底。
**平滑过渡/几何 morph 暂缓**（用户决定"最后来实现"）。

### 10.2 已实现
- **LOD 预加载（frontier prefetch）**：`Tilemgr2_imp.hpp` 的 `ProcessNode` 在判定"当前不需要细分"的渲染叶子上，若 `lvl < tgtres + Config->TerrainPreloadLevels`（默认 1）则提前加载 4 个子瓦片（`LoadChildNode`/`LoadTileAsync`），但**不切换渲染**；子瓦片加载后保持 Inactive 缓存，等渲染判据追上时零等待切换。阴影遍历（`SceneShadowCoverageNode`/`SceneShadowTerrainNode`）跳过 Inactive/非 ForRender 瓦片，避免把预取瓦片当渲染表示。
- **LOD 滞回（解决抽搐）**：`ProcessNode` 用连续目标层级 `tgtresF` 判定，升级需 `tgtresF > lvl + 0.55`，已细分的瓦片需 `tgtresF > lvl + 0.35` 才保持；避免阈值附近每帧高低清切换（用户已验证抽搐消失）。
- **高程 LOD 阈值（高空省加载）**：`SurfTile::LoadElevationData` 中，当相机高度 > `TerrainElevSkipAlt`（默认 50 km）且瓦片 `lvl < TerrainElevMinLevel`（默认 10）时**不读高程文件**，改走"祖先瓦片高程插值"路径（保留粗略地形，省掉高程文件 IO——瓦片加载的主要成本）；lvl 0 始终读文件以保留插值基准。
- **飞行中加载兜底**：`TerrainLoadWait.inc` 在 state 3 每帧检测——相机快速下降（每帧高度下降 >2 m）且 pending 瓦片数 ≥ `TerrainLoadWaitPendingLimit`（默认 12）且冷却 10 s 后调用 `TriggerTerrainLoadWait("flying")` 暂停模拟等待加载（每会话最多 5 次）；复用已有等待状态机（含超时 `TerrainLoadWaitTimeout` 与最小等待 `TerrainLoadWaitMinMs`）。
- **统计日志**：`DebugLvl > 1` 时每 300 帧输出 `TerrainStats: tiles= pending= queue= threads= alt=`（在 TerrainLoadWait.inc state 3）。
- **加载线程数配置**：`TerrainLoadThreads`（1–4）。**默认 1（单线程）**：实测 Intel UHD 750 上多线程并发创建 D3D 纹理/网格会卡死（冒烟 hang）；多线程代码保留但慎用，真正并行需重构为"PreLoad（IO/解码）并行 + Load（D3D 资源）串行"。

### 10.3 配置（install64\Orbiter\D3D9Client.cfg）
| 键 | 默认 | 说明 |
|---|---|---|
| `TerrainPreloadLevels` | 1 | 提前加载级数（0=关闭，1=提前一级，2=两级） |
| `TerrainTransitionMs` | 0 | 平滑过渡时长（0=关闭；交叉淡入代码保留但未调好，默认关） |
| `TerrainElevMinLevel` | 10 | 高于 SkipAlt 时，LOD 低于此值的瓦片不读高程文件 |
| `TerrainElevSkipAlt` | 50000 | 启用上述高程跳过的相机高度 [m] |
| `TerrainLoadThreads` | 1 | 地形加载线程数（1–4；>1 有并发 D3D 卡死风险） |
| `TerrainLoadWaitFlying` | 1 | 飞行中加载跟不上时暂停等待（0=关闭） |
| `TerrainLoadWaitPendingLimit` | 12 | 触发飞行等待的 pending 瓦片阈值 |

### 10.4 未完成 / 后续
- **平滑过渡链路**（交叉淡入）：代码在 `Tilemgr2.cpp`（fadeTiles/RenderFadeTiles/UpdateFadeTiles）+ `SurfTile::Render`（TerrainTileFade alpha）+ `NewPlanet.hlsl`（`TerrainTileFade` 常量）里保留，默认关闭；之前测试出现父瓦片/子瓦片重影（深度问题，已给父瓦片 ZWrite off 但未调好），需要重新设计（或改几何 morph）。
- **几何 morph**：未做（VERTEX_2TEX.e 已存顶点高程，可作为基础）。
- **跨父节点的整屏批次统一**：当前批次粒度 = 父节点 4 子瓦片（原版行为）+ 预加载提前就绪；全局屏障未做。
- **高程跳过的降高恢复**：高空跳过文件的瓦片在降回低空时不会自动重读音文件（保持粗略地形），直到被 LOD 细分替换；如需完全恢复需加"跳过标记 + 低空重建"。