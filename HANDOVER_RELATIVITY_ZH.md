# 相对论物理 交接文档（HANDOVER_RELATIVITY_ZH）

> 交接人：相对论物理开发（v1.1.1）
> 接手人（下一条鱼）：**负责相对论视觉特效（红蓝移 / 光行差 / 空间扭曲）**
> 最后更新：v1.1.1（commit `d9f069f7`，tag `v1.1.1`）

---

## 1. 项目状态

| 项目 | 内容 |
|---|---|
| 源码目录 | `D:\AI_projet\orbiter\orbiter`（git） |
| 构建目录 | `build64`（VS2022 x64，CMake） |
| 分发包 | `install64\Orbiter` |
| 当前版本 | **v1.1.1**（tag `v1.1.1`，commit `d9f069f7`，main 已推送） |
| 内容 | v1.1 汉化（已完成）+ **相对论物理**（已完成，见第 2 节） |
| 未完成 | **相对论视觉特效**（本次交接的重点，见第 3 节） |

另外请先读 `HANDOVER_ZH.md`（汉化交接文档，含构建/游戏操作规范，同样适用）。

---

## 2. 相对论物理（v1.1.1 已完成）

### 2.1 设计原则

1. 全局时间 = 惯性系坐标时（Orbiter 原有求解器不变）
2. 每个天体用相对论场（1PN 或 Kerr），多体**线性叠加**（主导源精确 + 其余弱场）
3. 相对论效应**只影响载具（飞船）**：天体的轨道/位置保持牛顿（SPICE 驱动）
4. 默认**关闭**（原版行为不变），作为物理配置项

### 2.2 配置项（`Src/Orbiter/Config.h` → `CFG_PHYSICSPRM`）

| 字段 | 值 | 说明 |
|---|---|---|
| `bRelativity` | false（默认） | 启用相对论物理 |
| `RelativityGravityMethod` | 0=1PN，1=Kerr（默认 0） | 引力场求解器 |
| `RelativityFTLMode` | 0=报错，1=限制到 0.9c（默认 0） | 超光速速度处理 |

写入了 `Orbiter.cfg` 的 `[Physics]` 段（`Relativity` / `RelativityGravityMethod` / `RelativityFTLMode`）。

### 2.3 UI 位置

- **选项 → 物理模拟**（`IDD_OPTIONS_PHYSICS`）：
  - "Relativeistic effects" 复选框（`IDC_OPT_PHYS_RELATIVITY`，1405）
  - "FTL velocity:" 组合框（`IDC_OPT_PHYS_RELFTL`，1408）：报错 / 限制到 0.9c
- **额外 → 相对论**（`TabExtra.cpp` 的 `ExtraRelativity` 类，`IDD_EXTRA_RELATIVITY`）：
  - 引力场求解器：弱场近似（1PN）/ 精确求解（Kerr）
- 翻译条目在 `Localization.cpp`（"相对论效应"、"报错"、"限制到 0.9c" 等）

### 2.4 实现清单（物理部分）

| 功能 | 文件/函数 | 说明 |
|---|---|---|
| **1PN 引力修正** | `Psys.cpp` `SingleGacc_pn()` | 单天体：牛顿 + `GM/(c²r³)[(4GM/r−v²)R + 4(R·v)v]`；含非球面扰动 |
| **Lense-Thirring 拖曳** | 同上（`SingleGacc_pn` 内） | `J = inertia·M·R²·ω·axis`；`a_LT = -2·v×B_g` |
| **惯性因子接口** | `Celbody.h/cpp`：`inertia_factor`（默认 0.4=均匀球体） | 天体 cfg 可用 `InertiaFactor = x` 覆盖 |
| **Kerr 精确解** | `Psys.cpp` `SingleGacc_kerr()` + `ks_*` 辅助函数 | **Kerr-Schild 笛卡尔坐标** + 解析度规 + **数值 Christoffel** + 4-速度归一化 → 坐标加速度 → SI/全局转换；仅用于**主导引力源** |
| **多体分发** | `Psys.cpp` `Gacc_intermediate(..., const Vector *gvel, int relMethod)` | `gvel` 非空 = 相对论路径；`relMethod=1` 时主导源用 Kerr、其余 1PN |
| **调用点** | `Rigidbody.cpp` `GetIntermediateMoments` | `g_psys->Gacc_intermediate (..., &state.vel, CfgPhysicsPrm.RelativityGravityMethod)` |
| **非引力加速度修正** | `Vessel.cpp` `Vessel::GetIntermediateMoments` | 推力/气动/辐射压/碰撞：`a = [a0 − (v·a0)v/c²]/γ`（纵向 γ³、横向 γ） |
| **视角锁定时间加速** | `Orbiter.cpp` `UpdateRelativityWarp()` | 焦点飞船 γ × 用户档位；`DisplayWarp()` 让右上角显示用户档位（自动缩放隐藏） |
| **超光速保护** | `Vessel.cpp` `Vessel::Update` 开头 | `|v| ≥ c`：报错模式 → 日志 + 暂停（**不要用 MessageBox**，物理更新中弹模态框会重入崩溃）；限制模式 → 沿速度方向截断到 **0.9c** |
| **测试工具** | `Script/relativity_meter.lua` | Terminal MFD 加速度计（`run('relativity_meter')`） |
| 光速常量 | `Astro.h` `c_light = 299792458.0` | |

### 2.4a 关于 Kerr 的适用性（重要说明，避免误解）

严格来说：**旋转的延展球体（有内部结构的天体）没有精确解析外部解**——外部场只能用多极展开（质量单极 + 自旋偶极 + 四极矩…）。

- **Kerr 度规**是**旋转黑洞**的精确解析解（真空 Einstein 场方程，1963）
- 本实现用 Kerr-Schild 形式**近似**天体外部场：把天体当作"球对称质量 + 自旋偶极主导"的 Kerr 源，**忽略高阶多极矩**（四极矩等）。对太阳系天体误差极小（多极矩非常小）
- 弱场下与"Schwarzschild 精确解 + 自旋修正"（或 Hartle-Thorne 慢转微扰）一阶一致；Kerr-Schild 还包含自旋高阶项（对黑洞精确、对天体近似到自旋主导阶）
- **不建议**改成"Schwarzschild 精确解 + 修正"（无必要：当前实现更简洁、阶数更高；黑洞场景下 Kerr 是精确的）

**结论**：Kerr 解用于天体外部场是标准且合适的近似；只有在需要**严格多极矩**精度（如高精度轨道计算）时才需要 Hartle-Thorne 或多极展开修正——对游戏精度不需要。

### 2.5 验证结果（用户实测，已通过）

| 测试 | 实测 | 理论 | 结论 |
|---|---|---|---|
| 低速推力加速度 a₀ | 13.15 m/s² | — | 基准 |
| 0.6c 横向 | 0.800 a₀ | 1/γ = 0.8 | ✓ |
| 0.6c 纵向 | 0.512 a₀ | 1/γ³ = 0.512 | ✓ |
| 时间加速（0.6c，视角锁定） | ×1.25 | γ=1.25 | ✓（右上角显示已隐藏自动缩放） |
| 相对论关闭（原版行为） | 正常 | — | ✓ |

---

## 3. 视觉特效（未完成 —— 交给下一条鱼）

### 3.1 目标

三个**独立开关**（放在 D3D9Client 视频设置里）：
1. **红蓝移**（相对论 Doppler：前方蓝移/后方红移 + 亮度变化）
2. **光行差**（高速时星空/画面向运动方向聚集）
3. **空间扭曲**（引力透镜：强场天体附近画面扭曲；弱场偏转角 `4GM/(c²b)`，可多体线性叠加）

物理依据：相对论公式（Doppler `1/[γ(1−βcosθ)]`、光行差 `cosθ'=(cosθ+β)/(1+βcosθ)`、弱场透镜偏转角）。**不要做成纯视觉编造**。

### 3.2 ⚠️ 已尝试的方案与失败记录（重要！避免重复踩坑）

**方案**：D3D9Client 屏幕空间后处理（`ImageProcessing` + 自写 HLSL，插在 `Scene::RenderMainScene` 中 pLightBlur 段之后）。

**失败记录**（按时间顺序）：
1. **无条件创建 GBUF_COLOR**：为了让视效不依赖后处理开启，把辅助颜色缓冲创建改成无条件 → **Intel UHD 集显上创建失败/崩溃** → 已移除
2. **成员未初始化**：`Scene::pRelativity` 未在构造函数初始化为 NULL → PostProcess 关闭时野指针，`if (pRelativity && pRelativity->IsOK())` 崩溃 → 已修（初始化 NULL）
3. **最小版 HLSL uniform 被优化**：pass-through 版本没用到任何 uniform → 编译器全部优化掉 → 每帧 `SetFloat Invalid variable name` 刷屏
4. **渲染状态被破坏（最终问题）**：后处理执行后，下一帧 `UpdateCamVis` 失败 + `pDevice->Clear()` 返回 `D3DERR_INVALIDCALL` → 画面只剩"一堆线"。**根因**：在 `RenderMainScene` 中间直接做 StretchRect/后处理，没有正确管理渲染目标（MSAA/离屏目标状态），把设备状态弄坏了
5. **D3D9Client 原版断言**：`Scene::SetupInternalCamera` 在相机附近天体检测失败时 `assert(false)`（`Scene.cpp` 约 3583 行）——在深空/边缘相机状态下触发（debug 断言；改成安全 `return false` 可避免，但断言修复已随回退移除）

**结论**：**不要**在 `RenderMainScene` 中间直接插 StretchRect 后处理。必须走 D3D9Client 已有的规范后处理路径。

### 3.3 建议方案（按优先级）

**方案 A（推荐）：复用 PostProcess 框架**
- 参考 D3D9Client 现有"轻辉光"（LightBlur）后处理流程：
  - `Scene.cpp` 构造（约 254-303 行）：`if (Config->PostProcess)` → 创建 `pOffscreenTarget`（主场景渲染到它）+ `ptgBuffer[GBUF_COLOR]` + `pLightBlur = new ImageProcessing(...)`
  - `RenderMainScene`（约 2006-2076 行）：pLightBlur 流程（StretchRect → 模糊 → 合成 → backbuffer）
- 做法：
  1. 视效开启时**同样创建** `pOffscreenTarget`（**非 MSAA** 离屏目标：`CreateRenderTarget(viewW, viewH, fmt, D3DMULTISAMPLE_NONE, 0, ...)`）和 `GBUF_COLOR`（同格式）
  2. 主场景渲染到 pOffscreenTarget（`RenderMainScene` 里 `pBackBuffer = pOffscreenTarget ? pOffscreenTarget : backbuffer` 已有逻辑）
  3. 在 pLightBlur 段之后，按 LightBlur 的同样流程：`StretchRect(pOffscreenTarget → GBUF_COLOR)` → `SetTextureNative("tBack", ...)` → `SetOutputNative(0, backbuffer)` → `Execute(true)`
  4. **注意**：必须检查所有 HR 返回值；执行前后**不要**干扰渲染目标（ImageProcessing::Execute 自己管理）
- 或者更省事：**视效依附于"轻辉光"后处理**（要求 PostProcess≥1 才启用视效，复用它的离屏目标/后处理链）——文档里说明这个限制即可
- HLSL 完整版（可复用，之前已写好，随回退删除）保存在本文档 3.4 节

**方案 B（简化）**：放弃逐像素，只做**全局色调叠加**（2D 半透明色层/色彩分级）——红蓝移表现为整体色调（无方向性），稳定、简单、不碰渲染目标

**方案 C**：独立 MFD/插件显示数据（不改画面）——只能做信息展示

### 3.4 HLSL 参考代码（之前实现，可复用）

```hlsl
// Relativity.hlsl —— 相对论视效后处理（ps_3_0，ImageProcessing 加载）
sampler2D tBack : register(s0);

uniform extern float3 camPos;      // 相机位置（全局系）
uniform extern float3 camRight;    // 相机基向量
uniform extern float3 camUp;
uniform extern float3 camFwd;
uniform extern float  tanHalfFov;
uniform extern float  aspect;
uniform extern float3 shipVel;     // 焦点飞船全局速度 [m/s]
uniform extern float  gamma;       // 洛伦兹因子
uniform extern float4 gravSrc;     // xyz=引力源世界位置, w=G*M/c²
uniform extern float dopplerStrength;    // 0..1
uniform extern float aberrationStrength; // 0..1
uniform extern float lensStrength;       // 0..1

#define C_LIGHT 299792458.0

float4 PSMain(float2 uv : TEXCOORD0) : COLOR
{
	float2 ndc = uv * 2.0 - 1.0;
	float3 dir = normalize(camFwd + camRight*(ndc.x*tanHalfFov*aspect) + camUp*(ndc.y*tanHalfFov));
	float beta = length(shipVel)/C_LIGHT;
	float3 vdir = normalize(shipVel + float3(1e-9,0,0));
	float2 uv2 = uv;

	// 光行差
	if (aberrationStrength > 0.0 && beta > 0.0) {
		float cosT = dot(dir, vdir);
		float cosT2 = (cosT + beta)/(1.0 + beta*cosT);
		float da = acos(clamp(cosT,-1,1)) - acos(clamp(cosT2,-1,1));
		float2 vScreen = float2(dot(vdir, camRight), dot(vdir, camUp));
		float vlen = length(vScreen);
		if (vlen > 1e-5) uv2 += (vScreen/vlen) * (da * aberrationStrength * 0.5);
	}

	// 引力透镜（弱场，主导源）
	if (lensStrength > 0.0) {
		float3 sp = gravSrc.xyz - camPos;
		float m = gravSrc.w;
		float dist = length(sp);
		if (dist > 1.0 && m > 0.0) {
			float3 sdir = sp/dist;
			float cosang = dot(dir, sdir);
			if (cosang > 0.1) {
				float b = dist*sqrt(max(0.0, 1.0-cosang*cosang));
				if (b < 1.0) b = 1.0;
				float alpha = 4.0*m/b;   // 偏转角
				float2 sscreen = float2(dot(sp,camRight), dot(sp,camUp))/dist;
				uv2 += (sscreen - ndc) * alpha * lensStrength * 0.5;
			}
		}
	}

	uv2 = clamp(uv2, 0.0, 1.0);
	float3 col = tex2D(tBack, uv2).rgb;

	// 相对论 Doppler（红蓝移）
	if (dopplerStrength > 0.0) {
		float dop = 1.0/(gamma*(1.0 - beta*dot(vdir, dir)));
		float t = saturate((dop-1.0)*0.5 + 0.5);
		float3 tint = lerp(float3(1.25,0.75,0.65), float3(0.65,0.85,1.35), t);
		float bright = pow(abs(dop), 0.8);
		col *= lerp(float3(1,1,1), tint*bright, dopplerStrength);
	}
	return float4(col, 1.0);
}
```

### 3.5 三个开关的配置/UI（之前已做过一半，已随回退移除，可重做）

- `D3D9Config.h`：`int RelDoppler; int RelAberration; int RelLensing;`（默认 0）
- `D3D9Config.cpp`：`oapiReadItem_int(hFile, (char*)"RelDoppler", ...)` / `oapiWriteItem_int(...)`（同 PostProcess 模式）
- `D3D9Client.rc`（`IDD_D3D9SETUP` 对话框）：底部加组
  ```rc
  GROUPBOX        "Relativistic effects", IDC_STATIC, 5, 375, 180, 42
  AUTOCHECKBOX    "Doppler shift (red/blue)", IDC_REL_DOPPLER, 15, 386, 130, 8
  AUTOCHECKBOX    "Aberration", IDC_REL_ABERRATION, 15, 398, 100, 8
  AUTOCHECKBOX    "Gravitational lensing", IDC_REL_LENSING, 15, 410, 130, 8
  ```
  对话框高度 378 → 420
- `resource.h`：`#define IDC_REL_DOPPLER 4086` / `IDC_REL_ABERRATION 4087` / `IDC_REL_LENSING 4088`
- `VideoTab.cpp`：初始化 `CheckDlgButton(...)` / 保存 `IsDlgButtonChecked(...)`（参考 PostProcess 位置）
- `Scene.cpp` 每帧：从 `Config->RelDoppler/RelAberration/RelLensing` 传 0/1 到三个 strength 参数

### 3.6 偏转角精度选择（建议）

空间扭曲（引力透镜）的偏转角**不必**用弱场近似 `4GM/(c²b)`，可以按精度分层：

1. **Schwarzschild 精确偏转角**（椭圆积分，Darwin 1959）：
   - 弱场展开：`α = 4M/b + 15πM²/4b² + 128M³/3b³ + …`（M = GM/c²，b = 冲击参数）
   - 或直接用椭圆积分形式（可查表/多项式近似，GPU 几十次运算/像素，性能可接受）
2. **Kerr 自旋修正**：用弱场自旋修正项（Lense-Thirring 对偏转角的线性修正，`O(a/b²)`）——不需要完整 Kerr 光线测地线
3. **完整的 Kerr 光线测地线**（逐像素积分）：理论可行但**不推荐**（屏幕空间无法正确处理多重像/遮挡，收益小、成本高）

**实现顺序**：先让后处理框架正确工作（渲染状态管理，这才是之前失败的原因）→ 框架通了再换精确偏转角公式。
**注意**：偏转角公式属于"单次偏转 + 屏幕空间 UV 偏移"近似，即使偏转角精确，屏幕空间仍无法生成多重像/爱因斯坦环的完整结构——这是方案本身的限制，不是公式精度问题。

---

## 3.7 强场验证结果（2026-11-12，用户实测）

### ✅ 1PN 模式在黑洞强场可用（验证通过）
- 测试环境：`install64\Orbiter\Scenarios\BlackHole_Test.scn`（10 M☉ 黑洞，r=1000km ≈ 33.9 倍 Schwarzschild 半径，v=0.12c）
- **近点进动**（椭圆轨道）：~16°/圈 ✓（`Extra → 相对论 → 弱场近似（1PN）`）
- **参考系拖曳（Lense-Thirring）**：自旋黑洞（`SidRotPeriod=0.001` → a/M≈0.5）+ 圆轨道 → **轨道面 LAN 进动 ~0.6°/圈** ✓
  - 自旋黑洞资产：`install64\Orbiter\Config\BlackHoleSpin.cfg`、`BlackHoleSpinSystem.cfg`、`Scenarios\BlackHoleSpin_Test.scn`（未提交 git）
  - 观测方法：Orbit MFD 的 LAN（升交点经度）——1PN 无自旋时不变，自旋拖曳时进动

### ⚠️ Kerr 精确模式（强场）暂不可用
- 实测：黑洞强场下**轨道失控（被甩出）**——原因未定位
- 可能原因：数值 Christoffel 的差分精度/步长、单位换算、4-速度归一化符号
- **修复建议**：
  1. **远场一致性验证**：在弱场（如 r=1e8 m）下 Kerr 应收敛到 1PN——对不上即定位 bug
  2. 长步长问题：强场先试时间加速 ×0.1 + 积分器 RK8（`额外 → Time propagation`）
  3. 长期：用 Kerr-Schild 度规的**解析 Christoffel 公式**替代数值差分
- **结论**：强场物理测试请用 **1PN 模式**（已验证）；Kerr 模式待修后启用


## 4. 待办/待验证（物理部分，可选）

1. **引力修正强场验证**：1PN/Kerr 的进动/拖曳需要专门场景，甚至**人造黑洞**（大质量/大自旋天体）——用户提过"只能写专门的场景甚至专门的天体"
2. **Kerr 性能/精度**：Kerr 数值 Christoffel 只在主导源使用（~128 次度规评估/源/帧），强场未实测
3. **D3D9Client 断言修复**：`Scene::SetupInternalCamera` 的 `assert(false)`（相机附近天体检测失败时）建议改为 `return false`（原版代码，深空/边缘相机会触发）
4. 视效（第 3 节，下一条鱼）

---

## 5. 关键文件清单（物理部分）

| 文件 | 作用 |
|---|---|
| `Src/Orbiter/Astro.h` | 光速常量 `c_light` |
| `Src/Orbiter/Config.h/.cpp` | 相对论配置项（读写 Orbiter.cfg） |
| `Src/Orbiter/Psys.h/.cpp` | 引力：`SingleGacc_pn`（1PN+LT）、`SingleGacc_kerr`（Kerr-Schild）、`Gacc_intermediate` 分发 |
| `Src/Orbiter/Celbody.h/.cpp` | 天体自转/惯性因子接口（`RotOmega()/RotAxis()/InertiaFactor()`） |
| `Src/Orbiter/Vessel.cpp` | 非引力加速度修正 + 超光速保护 |
| `Src/Orbiter/Rigidbody.cpp` | 调用点（传速度/求解器） |
| `Src/Orbiter/Orbiter.h/.cpp` | 视角锁定时间加速 `UpdateRelativityWarp()` / `DisplayWarp()` |
| `Src/Orbiter/MenuInfoBar.cpp` | 右上角时间加速显示（隐藏自动缩放） |
| `Src/Orbiter/OptionsPages.cpp` + `Orbiter.rc` + `resource.h` | 物理设置页 UI |
| `Src/Orbiter/TabExtra.cpp/.h` | Extra 页"相对论"（引力场求解器选择） |
| `Src/Orbiter/Localization.cpp` | 相对论相关翻译 |
| `Script/relativity_meter.lua` | Terminal MFD 加速度计（测试工具） |
| `install64\Orbiter\Scenarios\Relativity_Test.scn` / `Relativity_Terminal.scn` | 测试场景（在分发包，未提交 git） |

---

## 6. 操作规范（更完整见 HANDOVER_ZH.md）

1. **构建**：`cmake --build build64 --config Release --parallel 6`（本机限制 6 核）；安装 `cmake --install build64 --config Release`
2. **构建前**确认游戏已关闭；**不要**擅自关闭用户的游戏
3. **打开游戏后不要操作/测试/关闭**——测试由用户进行；启动用 WMI（`Invoke-CimMethod Win32_Process Create`）
4. 测试脚本：`Script/relativity_meter.lua`（Terminal MFD `run('relativity_meter')`，显示 |a|/along/|v|/gamma）
5. 文件编码：`Defpanel.h` 等老文件 CP1252；`Localization.cpp`/`Vessel.cpp`/`Psys.cpp`/`OptionsPages.cpp` 等 UTF-8（无 BOM）
6. 翻译条目加在 `Localization.cpp` 的 `g_translations[]` 数组末尾（v1.1 的 BuildTables 会合并）

---

## 7. 给下一条鱼的开工建议

1. 先读 `HANDOVER_ZH.md`（汉化）+ 本文档
2. 构建一次确认环境（`BUILD EXIT: 0`）
3. 视效从**方案 A** 开始：先把 D3D9Client 的 LightBlur 后处理流程读懂（`Scene.cpp` 254-303 / 2006-2076），确认离屏目标/渲染目标管理，再插入视效 pass
4. **不要**重复本文档 3.2 的失败路线（在 RenderMainScene 中间直接 StretchRect/后处理）
5. 每改一步就用**用户测试**（游戏内进场景），并**监控日志**（Orbiter.log 的 D3D9ERROR/D3D9 行）
6. 视效的 HLSL（3.4 节）和三个开关的 UI（3.5 节）可以直接复用
