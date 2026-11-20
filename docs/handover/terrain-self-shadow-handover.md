# 地形自遮挡阴影交接文档（Terrain Self-Shadow / Horizon Field）

> 交接对象：**下一条鱼**（接手"地形自遮挡阴影 / 地形渲染大改"的会话）。
> 基线：2026-11-20，HEAD `cab821bb`（已含 `895502eb` 回加的 G1 地形场景阴影）。
> 目的：把"山体投影到相邻坡面/山谷"这个需求在当前代码里的现状、之前尝试过的路线和失败点、
> 推荐方案（地平线角场）、实施步骤、验收标准和**不能破坏的东西**交接清楚。
>
> 先读这一份 + `docs/handover/vessel-shadow-handover.md` + `docs/handover/performance-handover.md`。
> 注意：`vessel-shadow-handover.md` §5.1 写"地形自身不投射阴影"是 G1 回退期间的旧描述，
> `895502eb` 之后已重新加回"仅地形"的 G1，阅读时以代码为准。

---

## 0. TL;DR（一句话任务）

要解决的是**地形对地形的投射阴影**（山影落在别的坡面/山谷上，随太阳实时变化），
**不是** AO/天空亮度场，**不是**船身阴影。

- 当前代码里已有一套自研的 **G1 场景阴影**（`OVP/D3D9Client/TerrainRender.inc`）：
  **只把"当前屏幕可见的地形 tile"渲染进 shadow map，地形接收**。
  覆盖范围 = 可见 tile 的 AABB（+500m）或固定半径，分辨率 = 后缓冲的 2 的幂，9-tap PCF，
  深度约定 `1 - z/w`。配置项 `TerrainSelfShadow` 等已存在。
- 这套方案解决了"局部、可见范围内"的地形自阴影，但有硬伤：
  **看不到的山脊不投影**（山谷阴影缺块/随视角跳）、覆盖范围与分辨率矛盾、
  低太阳长阴影差、每帧/隔帧重渲地形有性能开销。这正是之前
  `04fe67f5 加 G1 → 87b343f4 回退 → 895502eb 只保留地形版重加` 反复折腾的那套。
- **推荐下一阶段方案：地平线角场（Horizon Field）**。
  按 tile 预计算每个 texel 在 K 个方位上的地平线仰角（含行星曲率），运行时逐像素
  与局部太阳仰角比较得到阴影。天然支持动态太阳、无 shadow map 偏移/锯齿、
  可覆盖跨 tile/屏幕外/远山/低太阳长阴影；副产品是 SVF/AO 场。
- 施工策略：**先不动 G1/G0**。新方案第一版作为"远场/全局地形自阴影"，
  G1 暂时保留作近场/对比；验证 sampler 预算和性能后再决定是否关掉 G1。
- **不要破坏**：G0 官方近场阴影、legacy projected 船影、地形加载等待、相对论地形着色、
  汉化、D3D9 资源释放顺序。详见 §9。

---

## 1. 需求与约束

### 1.1 需求

- 输入：地形高程（DEM tile）、动态太阳方向、相机；
- 输出：地形像素的**太阳可见性/阴影因子**，可直接乘到地形直接光上；
- 可选输出：SVF/AO（天空亮度场）——本任务不要求，但同一份地平线数据可顺带得到。

### 1.2 约束

- 行星尺度、tile 流式加载、LOD/morph 过渡；
- 动态太阳（时间加速下太阳方向连续变化）；
- 当前渲染器是 D3D9Client（PS_3_0、**16 sampler 上限**、无 compute/DXR）；
- 后续可能换渲染器（所以核心数据/算法要能独立于 D3D9）；
- 不能破坏英文模式、相对论视觉、汉化、地形加载等待、现有阴影系统。

### 1.3 非目标（可后续再做）

- 多次反弹 GI / 天空辐照度物理模型（先用简单天空项）；
- 与云影/船影统一为单一 shadow 系统（已有 G0/legacy，不要求合并）；
- 实时反射/间接光。

---

## 2. 当前代码现状（必须读）

当前地形附近一共有 **三套阴影**，不要混淆：

### 2.1 G0：官方近场 shadow map（船/残骸/基地自阴影）

- 入口：`Scene::RenderShadowMap()`（`OVP/D3D9Client/Scene.h` L286/289，实现 `Scene.cpp`）；
- 配置：`D3D9Config.h` L75-77 `ShadowMapMode`、`ShadowFilter`、`ShadowMapSize`；
- 接收端：船体/基地网格（`NewMesh.hlsl` 等）；**地形 shader 不再采样 G0**
  （`_SHDMAP` 在 `VPlanetAtmo.cpp` L151/L174 被注释，原因：`ps_3_0` sampler 数已接近 16 上限）；
- 范围小（焦点船周围约 10m，残骸约 200m），不能解决地形自遮挡。

### 2.2 legacy 投影/模板地面阴影（船影）

- 代码：`TerrainRender.inc` PART 1（L41-236）；
  `vVessel::UpdateShadowPlane()` L66、`vVessel::RenderGroundShadow()` L140；
- 配置：`TerrainShadowing`（0=None/1=Stencil/2=Projected）；
- 参考面来自**实际渲染的地形高程/基地平面**（`vPlanet::GetElevation` + 官方法线 API），
  不要改回 mean-sphere 切平面，否则高分辨率地形上会出现船影漂浮/错位（已修过）。

### 2.3 G1：统一场景阴影（自研，当前只投地形、只给地形接收）

> 这是"之前反复开发/回退"的系统，当前是**仅地形版**（`895502eb`）。

- 代码：`OVP/D3D9Client/TerrainRender.inc` PART 2（L237-800）
  - `class ShadowSystem`（L250+）：资源/着色器/尺寸管理；
  - `SceneShadowCoverageNode()`（L434）：从当前地形渲染树收集**屏幕可见 tile** 的包围球，
    合并成 AABB → 阴影覆盖中心/半径（+500m）；没有可见 tile 时按 `alt*6` 回退；
  - `SceneShadowTerrainNode()`（L480）：遍历地形树，把可见 tile 画进 shadow map；
  - `Scene::UpdateEffectShadows()`（L530）：把 G0/G1 参数传到 D3D9Effect uniform；
  - `Scene::RenderSceneShadow()`（L582）：G1 的主流程
    （尺寸/覆盖计算 → 地形 caster → 阴影图渲染 → 诊断日志）；
  - `Scene::InitShadowSystem()` / `ExitShadowSystem()`（L784/L792）；
- 调用链：`Surfmgr2.cpp` L1542，在 tile tree 处理完之后、地形渲染之前调用
  `scene->RenderSceneShadow(vp)`；
- 接收端：`NewPlanet.hlsl`（**地形渲染主体**）
  - VS 输出 `shdH2`（L52）并做 normal offset（L406，`Prm.vSHD2.x * 1.5f`）；
  - PS 的 `_TERRSHD` 分支（L517-549）：透视除法 → `z = 1 - z` → 9-tap `SampleShadowsT()`
    （L186+）→ 覆盖边缘 fade → slope-scaled bias（`omxMax = max(0.5, vSHD2.x*4)`）；
  - caster shader：`ShadowVS` / `ShadowPS`（`NewPlanet.hlsl` L1125/L1134），
    写入约定为 **`1 - z/w`**（L1102 注释）；
- 资源/参数：`Scene.h` L179-195（`SHADOWMAPPARAM smapT`、`smapTValid`、
  `GetSMapDataT()`/`IsSceneShadowValid()`）；uniform 见 `D3D9Effect`（`eShadowMap2`、
  `eLVP2`、`eSHD2`、`eSHDT2`）；`VPlanetAtmo.cpp` L152/L175 给地形 technique 加 `_TERRSHD`，
  L154/L177 加 `_TERRSHDDBG`（debug）；
- 配置（`D3D9Config.h` L75-86，默认值 `D3D9Config.cpp` L95-103）：
  - `TerrainSelfShadow = 1`：G1 总开关（同时需要 `ShadowMapMode != 0`）；
  - `TerrainShadowSize = 0`：0 = 后缓冲尺寸的 2 的幂；
  - `TerrainShadowRange = 0`：0 = 可见 tile AABB；>0 为固定半径（300~20000m）；
  - `TerrainShadowMaxAlt = 100000`：相机高于此高度不生成阴影；
  - `TerrainShadowUpdate = 1`：更新间隔（帧）；
  - `TerrainShadowLegacy = 0`：是否保留 legacy 船影；
  - `TerrainShadowDebug = 0`：G1 debug 可视化；
  - `TerrainShadowStrength = 100`：阴影强度；
- **当前限制（重要）**：
  - caster 只包含**本帧屏幕可见的地形 tile**（`SceneShadowCoverageNode` 只看
    `IsVisibleInCamera()` 的 tile）→ 屏幕外山脊不会投影；山谷阴影会缺块/随视角跳动；
  - 覆盖范围与分辨率矛盾：map = backbuffer 2 的幂（如 2048），覆盖 60km 时 texel ≈ 30m，
    低太阳长阴影很糊/断裂；缩小覆盖则远处无影；
  - `TerrainShadowMaxAlt` 默认 100000 → 高空也做阴影，但覆盖/精度更差；
  - 每帧/隔帧重渲地形进 shadow map（`performance-handover.md` §1.4 已标为
    "可能是渲染 CPU 大头之一，待测"）；更新间隔大时阴影滞后；
  - 深度 bias/fade/9-tap 软化会吃掉细节；
  - 注释不一致：`VPlanetAtmo.cpp` L148-150 说"scene shadow map 里含 vessels"，
    但 `RenderSceneShadow` 中 L738-746 明确 **vessels/base 不投影**（走 G0）——以代码为准；
  - 地形 shader 因 sampler 上限没有采样 G0（`_SHDMAP` 注释），船影到地形目前靠 legacy projected。

### 2.4 失败/回退历史（避免重复踩坑）

- `04fe67f5`（2026-11-19）：地形渲染交接基线，首次加 G1 统一场景阴影（含 vessel/base 投射）；
- `87b343f4`（2026-11-20）：**整体回退** G1 与地形算法改动（恢复官方 + 已有本地功能），
  保留加载等待/相对论/汉化；
- `895502eb`（2026-11-20）：**只把 G1 按"仅地形投射 + 地形接收"加回**，
  移除 G1 对 vessel/base 的投射与接收，`Common.hlsl` 回退避免未声明 uniform。
- 结论：G1 的"shadow map 路线"能做局部效果，但行星级 + 动态太阳 + 长阴影下的
  覆盖/分辨率/性能/屏幕外遮挡问题是结构性问题，**调参解决不了**。

---

## 3. 推荐方案：地平线角场（Horizon Field）

### 3.1 原理与公式

对每个地形 texel $p_0$（高程 $h_0$），在 K 个方位角 $\varphi_k$ 上预计算**地平线仰角**：

$$
h_k(p_0)=\max_{d}\; \operatorname{atan2}\!\left(h(d)-h_0-\frac{d^{2}}{2R},\; d\right)
$$

- $d^2/(2R)$：行星曲率下沉量（$R$ = 行星半径）。**必须做**：
  50km ≈ 196m、200km ≈ 3.1km，不做远处山影全错；
- 距离分段：近处（≤50km）用高分辨率 tile；远处（50~500km）用低层级高程金字塔；
  取两段最大值。1° 太阳高度、4km 山 → 阴影长约 230km，必须覆盖到；
- 太阳可见性：把全局太阳方向转到该 texel 的局部 ENU 系，求仰角 $\theta_s$：

$$
V = \operatorname{clamp}\!\left(\frac{\theta_s - h(\varphi_s)}{2r_\odot} + 0.5,\;0,\;1\right),\qquad r_\odot \approx 0.27^\circ
$$

  （太阳圆盘角半径 → 半影/软阴影；也可以按 3~5 个方位采样太阳盘）；
- 阴影因子 = $1-V$，乘到地形直接光上；
- **副产品**（可选）：天空可见性 $SVF=\frac{1}{K}\sum_k\cos^2 h_k$，
  乘天空亮度模型即可得到环境亮度场。

### 3.2 数据格式建议

- 每个 tile 一张/一组 **horizon 纹理**：
  - 分辨率 = 高程 tile 的 **1/2~1/4**（如 64×64 或 128×128，渲染 tile 的 259×259 是 256+3 padding）；
  - 方位 $K = 16 \sim 32$；
  - 编码：**16-bit 优先**（8-bit 对低太阳长阴影不够；或存 $\tan h$ / 非线性）；
  - 打包：受 D3D9 格式/采样限制，建议把 K 个方位做成**行/列**或 4 通道 × 多张；
    或写进一张 atlas；PS 手动在方位方向插值；
- 内存估算：64² × 16 方位 × 2B = **128KB/tile**；只加载近处/可见 tile，
  远处用粗层级或全局低分辨率场；
- 元数据：tile 索引、K、编码、角度范围、高程数据版本（含 `Elev_mod`/KSC 平整）、
  边缘 halo（跨 tile 烘焙用，不需要运行时处理接缝）；
- 可选：SVF/AO 每 texel 8-bit，与 horizon 同 atlas。

### 3.3 烘焙管线（CPU 多线程，不需要 compute）

**输入高程必须与渲染地形一致**：

- 高程文件/归档：`Src/Orbiter/elevmgr.cpp` L13（`elev_grid=256`、`elev_stride=259`）、
  L53（`Elev.tree`/`Elev_mod.tree`）、L110（`Textures\<planet>\Elev\<lvl>\<ilat>\<ilng>.elv`）；
  `.elv` 头 `ELEVFILEHEADER`：INT16 raw + `scale/offset` + min/max/mean，
  即 `elev_m = raw * scale + offset`；
- 渲染 tile level 与 Elev 文件 level：`Surfmgr2.cpp` L509 `lvl + 4`
  （渲染 tile L ↔ Elev 文件 L+4），别搞错层级；
- 视觉高程可能被 GraphicsClient 的 `clbkFilterElevation`（`GraphicsAPI.h` ~L1480，
  KSC 平整等）修改；**烘焙必须用最终视觉高程**，否则阴影和实际地形错位；
  运行时也可用 `vPlanet::GetElevation(lng,lat)`（`Surfmgr2.cpp` L628/L1686）或
  `oapiSurfaceElevation`/`oapiSurfaceElevationEx`（`Orbitersdk/include/OrbiterAPI.h` L4138/L4161）；
- 高程金字塔：把相邻层级的 tile 组成可任意采样（含跨 tile、粗 LOD）的采样器——
  这是整个烘焙器的核心基础设施，先做它。

**算法（二选一）**：

1. **扫描线/单调栈**（推荐，快）：对每个方位 $\varphi_k$，把高度场按该方位重采样成剖面，
   沿剖面用单调栈一次求出该剖面上所有点的地平线角；复杂度约 $O(K\cdot N^2)$；
2. **逐 texel 多步 march**（实现最简单）：每个 texel 每方位按距离步进采样高程，
   取最大仰角；需限制最大步数 + 粗 LOD 采样；适合先做通/对拍。

**并行与缓存**：按方位/行分块多线程；烘焙结果按 (planet, tile, 高程版本, filter 版本)
缓存；可离线烘到安装目录/归档，也可 tile 加载时异步烘焙（工作线程只读高程；
D3D9 纹理上传只能在渲染线程做）。

**触发**：离线工具（安装/首次访问）优先；运行时异步烘焙作为后备；
地形可编辑/程序化生成时按 tile 失效重烘。

### 3.4 运行时集成（D3D9Client）

- **数据**：随 tile 加载 horizon 纹理（或 atlas）；tile LOD 切换不影响场分辨率
  （按物理 tile level 烘焙，渲染 LOD 只影响几何）；
- **shader**：`NewPlanet.hlsl`（地形 VS/PS）新增 `_TERRHORIZON` 分支：
  1. VS 传局部 ENU 或由 `normalize(frg.nrmW)` + 太阳方向推局部仰角（不要用全局单一仰角）；
  2. PS 以 `sunAzimuth` 在 horizon 纹理中插值取 $h(\varphi_s)$；
  3. 用公式算 $V$，软阴影；`fShadow = min(fShadow, 1 - V)`；
  4. 与现有 G1/G0 组合（先 min，确认后再考虑替换）；
- **D3D9 限制**：PS_3_0 16 个 sampler 已接近上限（`_SHDMAP` 就是因为这个被注释）；
  horizon 纹理必须精打细算——打包进现有纹理、替换 G1 sampler、
  或按 tile 用量子化后在 CPU 侧预计算（每 tile 一张低分辨率"太阳可见性图"）。
  如果加 sampler 导致超限，优先考虑**替换 G1**（G1 本来就要被取代）而不是加第三套；
- **配置**（`D3D9Config.h/.cpp` 新增，命名参考现有）：
  `TerrainHorizonShadow`（开关）、`TerrainHorizonSunSoftness`（软影角度）、
  `TerrainHorizonDebug`（可视化）、`TerrainHorizonFieldSize`（纹理分辨率）；
  默认建议先可回退（默认 1，但保留关掉走 G1 的路径）；
- **调试**：debug 模式画 horizon 场/覆盖范围/太阳仰角；`PerfStats` 增加烘焙/采样计时；
- **相对论一致性（可选但建议）**：我们 fork 的视觉太阳方向有 `Scene::RelativityApparentSunDir()`
  （`Relativity.inc` L1212；`Scene.cpp` L1195 已用于 glare），射线/光行差后的太阳方向
  与星空/glare 视觉一致。地形阴影用真实 `sunLight.Dir` 还是 apparent 方向需要拍板：
  建议至少记录差异；若追求视觉一致，用 apparent 方向（注意它每帧可能变化，horizon 比较没问题）。

### 3.5 与现有系统的组合策略

- 第一版：**horizon 场做远场/全局自阴影，G1 保留近场高精度**（G1 只覆盖屏幕可见 tile，
  正好补 horizon 场分辨率不足的近处细节）；两者 `min` 合并；
- 如果 horizon 场分辨率/软影已足够：关闭 G1 省掉每帧地形树遍历 + shadow map 渲染，
  性能反而更好（这是 horizon 方案的主要收益之一）；
- G0/legacy 船影保持不动（船/残骸/基地的阴影不在地形自遮挡范围内）。

### 3.6 备选方案（如果地平线场成本太高，想先看到效果）

- **G1 改进（治标）**：增大 `TerrainShadowRange`、`TerrainShadowSize`、
  把 caster 从"可见 tile"改为"可见 + 屏幕外粗 LOD tile"、级联/分块 shadow map、
  降低更新频率；仍解决不了低太阳/超远阴影，且性能更差；
- **方案 C：逐 tile 太阳方向扫描缓存**：对每个 tile 按太阳方位重采样剖面 → 1D 阴影传播 →
  tile 阴影纹理；按太阳方位/仰角分桶缓存；跨 tile 加 halo。实现比 A 简单、无深度偏移问题，
  适合快速验证；缺点是太阳变化要重算/桶间跳变。

---

## 4. 关键坑（逐条核对，之前大概率栽在这里）

1. **Orbiter 左手系 + 方位/仰角定义**：东/北/上方向、atan2 参数顺序、符号；
   搞反会出现"阴影在太阳反方向"或"看起来像阴影但位置错"；
2. **太阳方向必须逐 texel/逐 tile 转到局部 ENU**，不能用全局单一太阳仰角；
3. **曲率修正** $d^2/(2R)$ 漏了 → 远处山影出现/消失位置错；
4. **跨 tile / 屏幕外**：只读本 tile 高程 → tile 边缘阴影断裂；
   必须用全局高程金字塔采样（含粗 LOD），并把烘焙半径做到足够远；
5. **高程层级/格式**：渲染 tile L ↔ Elev 文件 L+4；INT16 `scale/offset`；
   259×259（256+3 padding）；`Elev_mod`/KSC 平整；
6. **视觉-物理高程一致性**：`clbkFilterElevation` 对视觉高程的修改要进烘焙输入，
   否则阴影与可见地形错位（这与物理碰撞高程也有关系，不要绕过）；
7. **LOD/morph**：用渲染 LOD 的高程烘场 → 阴影边界随镜头抖；
   烘焙应按物理 tile level 固定分辨率，渲染 LOD 不影响场；
8. **深度约定**：现有 G0/G1 都是 `1 - z/w`（`NewPlanet.hlsl` L1102 注释），
   如果新方案也要输出 shadow map/比较深度，保持一致；
9. **sampler 预算**：PS_3_0 16 个；`_SHDMAP` 已因此禁用地形接收 G0；
   horizon 纹理要么打包、要么替换 G1 sampler；
10. **低太阳长阴影**：最大地平线距离不足 → 日出日落整片阴影丢失；
    1° 太阳高度、4km 山需要 ~230km 覆盖；
11. **软阴影**：太阳不是点光源（0.27°），硬边不可接受；
12. **不要重复乘光照**：$N\cdot L$ 已有明暗，horizon 只乘可见性；不要把坡度/horizon
    当第二遍暗化；
13. **精度**：行星尺度世界坐标用双精度/相对坐标，float 会抖；
14. **性能**：烘焙/采样/纹理带宽；G1 现状每帧地形树遍历 + shadow map 渲染是已知开销，
    用 `PerfStats`/`PerfLog` 先量；
15. **资源释放顺序**：会话关闭时 D3D 资源的释放顺序（`Orbiter.cpp` CloseSession；
    见 vessel-shadow-handover §4 第 7 条），新纹理/RT 不要在图形服务关闭后再释放。

---

## 5. 验证与验收标准

1. **合成地形**：单高斯山、单山脊、两山夹谷；CPU ray march 求 ground truth 阴影图；
   烘焙器输出与其逐太阳高度/方位对比，边界误差 ≤ 1~2 texel；
2. **跨 tile**：2~3 个相邻 tile（不同 LOD、morph 中）边界处阴影无断层；
3. **行星尺度**：低太阳仰角（<5°）、晨昏线、极区、远山（>100km）阴影；
4. **视觉**：与参考截图对比；无阴影 acne、无硬边/锯齿、无覆盖边缘硬边；
   相机移动/旋转时阴影稳定；
5. **动态太阳**：时间加速下阴影连续变化、无跳变；
6. **性能**：G1 开/关、horizon 开/关对比；`PerfStats` 分阶段；
   目标：无明显帧率下降（具体阈值先测后定）；
7. **回归**：船影（legacy/G0）、地形加载等待、相对论地形着色、汉化 HUD、
   D3D9 设备重建/DXVK 变体正常。

---

## 6. 里程碑（建议 4~6 周，单人熟手；只做 demo 约 2 周）

| 里程碑 | 内容 | 时间 |
|---|---|---|
| M0 | 审计现状 + 复现 G1 问题（山区低太阳截图）+ 定坐标/数据格式/层级约定 | 2~3 天 |
| M1 | CPU 参考 ray march + tile 高度金字塔采样器 + 单 tile horizon 烘焙（离屏可视化） | ~1 周 |
| M2 | D3D9 集成：纹理加载/采样/软影/配置/调试；单 tile + 合成测试 | 1~2 周 |
| M3 | 跨 tile/多尺度/曲率/低太阳/晨昏线；与 G1/G0 组合；性能 | 1~2 周 |
| M4 | 文档、汉化/配置界面（若加 UI）、回归清单、默认值/开关 | 3~5 天 |

---

## 7. 交付物与目录建议

- 烘焙器：`Utils/TerrainShadow/`（独立工具；输入高程 tile/归档 + 高程金字塔，
  输出 horizon 数据/归档；支持增量、并行、可视化对比）；
- 运行时：`OVP/D3D9Client/TerrainShadow.inc`（或扩展 `TerrainRender.inc`）+
  `NewPlanet.hlsl` 分支 + `D3D9Config` 配置项 + 调试绘制；
- 资源：`Textures/<planet>/Horizon/...` 或新归档（遵循现有 install/CMake 规则，
  注意 `compileOrbiter.py`/CDN 数据布局，不要把官方数据目录搞乱）；
- 文档：更新本文件 + `docs/handover/performance-handover.md`（性能数据）+
  根 `HANDOVER.md`（如果加配置项/需要用户可见说明）。

---

## 8. 参考

- Max, N. *Horizon Mapping: Shadows for Bump-Mapped Surfaces*, The Visual Computer, 1988；
- Sloan, P.-P. & Cohen, M. *Hardware Accelerated Horizon Mapping*, Rendering Techniques 2000；
- Stewart, A. J. *Fast Horizon Computation at All Points of a Terrain With Visibility and
  Shading Applications*, IEEE TVCG 1998；
- 屏幕空间 AO 对照：HBAO（Bavoil et al. 2008）、GTAO（Jimenez et al. 2016）；
- 后续亮度场：PRT/SH9（Sloan et al. 2002）、irradiance volume/DDGI（Majercik et al. 2019）；
- 仓库内现有实现：`TerrainRender.inc`、`NewPlanet.hlsl`、`Scene.h/.cpp`、
  `docs/handover/vessel-shadow-handover.md`、`docs/handover/performance-handover.md`。

---

## 9. 注意事项（用户偏好/工程规则）

1. 中文交流/思考；回复简洁；**不擅自做未要求的事**（用户："我没说要干的事不要做"）；
2. **不要擅自 git commit**（仓库有大量未提交改动）；改完先 `git status`/`git diff` 汇报；
3. **构建/安装前确认游戏已关闭**；不要擅自关用户的游戏；开游戏后不要操作/测试，
   测试由用户进行（自动截图脚本见 performance-handover §5）；
4. 构建：vcvarsall x64 + cmake，`--parallel 1`（沙箱内多节点 MSBuild 会静默失败）；
   安装后校验 SHA256；
5. **英模字节级原版**：所有新功能必须在英文模式/官方路径关闭时保持原有行为；
6. **相对论**：`Relativity.inc`/`RelativityShaders.hlsl`/`NewPlanet.hlsl` 的相对论分支
   不要删/不要打乱；地形大改时优先保持相对论路径可用；
7. **汉化**：如果新增 UI 配置页/提示，要补 `Localization.cpp`/`DialogZh.inc`/
   `LaunchpadZh.inc` 等翻译；不要破坏 `ZhCommon.inc` 的 HUD 绘制；
8. **地形加载等待**：`TerrainLoadWait.inc`、`TileManager2::TileTree()`、`Tile::State()`、
   `vPlanet::SurfMgr2()` 依赖不要破坏（见 vessel-shadow-handover §3.3/§4）；
9. 交接时先做三件事：
   (a) 确认当前 build 是否包含 `895502eb` + `cab821bb`（G1 地形版）；
   (b) 在山区/低太阳场景打开 `TerrainSelfShadow=1` 截图，记录 G1 的具体问题；
   (c) 用 `PerfStats`/`PerfLog` 量 G1 阴影 pass 的 CPU 开销，再决定"修 G1"还是"上 horizon 场"。
