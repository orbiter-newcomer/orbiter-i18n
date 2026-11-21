# Orbiter Chinese Localization + Relativity Extension

## 1. Introduction

This repository is a **64-bit branch** of [Orbiter](https://github.com/orbitersim/orbiter), the open-source space flight simulator. On top of the official game it provides:

- **Chinese localization**: UI, MFDs, Scenario Editor, vessels, glass-cockpit panel and HUD, help and scenario texts; complete internationalization API (translation lookup, layout data, dialog layout fitting)
- **Relativity extension**: new relativistic gravity models, black hole / neutron star bodies and relativistic visual effects
- **Terrain rendering optimization**: new terrain self-shadowing and terrain preloading system
- **Performance optimization**: physics/rendering parallelism, multi-core compute, rendering optimization
- **Wreck and impact system**: new damage events, wreck model generation and damage physics
- **New contact solver**: new solver modes, settings page and contact parameter tuning
- **Parameter tuning**: autopilot, hover altitude hold and thermal control system parameter tuning
- **Thermal control system optimization**: new mass model, time integration and other thermal optimizations

- Repository: https://github.com/orbiter-newcomer/orbiter-i18n
- Current version: **v1.3.2** (terrain rendering optimization, performance optimization, new contact solver, wreck and impact system, parameter tuning, thermal control system optimization, Chinese layout refinements, internationalization API)
- 中文说明: [README.md](./README.md)

---

## 2. Usage

### 2.1 Installation and running

1. Use an official Orbiter 2024 installation (planet textures etc. are required), or the `install64` directory of this project;
2. Copy the build outputs (`Orbiter.exe`, `Modules/Plugin/D3D9Client.dll`, ...) into the installation directory;
3. Start `Orbiter.exe`, choose a scenario in the Launchpad and fly.

### 2.2 Switching the language

**Option A: In-game (recommended)**

1. Start `Orbiter.exe` to open the **Launchpad**
2. Click **Options** → **UI** page
3. Select **Chinese (Simplified)** in the **Language** drop-down (takes effect immediately and is saved)

**Option B: Configuration file**

Edit `Localization.cfg` in the game root directory:

```ini
Language=zh   ; Chinese
Language=en   ; English (default)
```

**English mode**: keeps the official behaviour; localization content only applies in Chinese mode.

### 2.3 Relativity settings

1. **Options → Physics**: master switch for relativistic physics (when off, neither relativistic physics nor visuals apply);
2. **Options → Visual**: relativistic visual switches (Doppler, reference-frame effects, gravitational light bending, ...);
3. **Extra → Relativity effects**: gravity model (Newton / 1PN / Kerr), charge-induced gravity switch;
4. **Extra → debug rendering options**: lensing wireframe, body position markers;
5. Relativity test scenarios: `NeutronStar`, `StellarBlackHole`, `SupermassiveBlackHole` under `Scenarios\Relativity\`.

### 2.4 Configuration

This section lists the configuration items that can be **set in the Launchpad UI**, together with their configuration file keys. Configuration items without a UI are listed in [6.4 Configuration parameters (developers)](#64-configuration-parameters-developers).

> **When changes take effect**: these settings require a **Launchpad restart** (restarting the Launchpad restarts the whole Orbiter); **restarting the simulation scenario alone does not apply them**. `i18n/layout/zh_CN/layout.ini` and `i18n/locales/zh_CN/*.po` take effect after switching the language once. The files are written back when Orbiter exits, so edit them while Orbiter is closed.
>
> `Orbiter.cfg` keys must start at the **beginning of a line** and booleans only accept `TRUE`/`FALSE` (writing `1` is ignored); `D3D9Client.cfg` allows leading indentation and matches full key names exactly.

| File | Purpose |
|---|---|
| `Orbiter.cfg` | Main program: visual, physics, damage simulation, contact model, relativity parameters |
| `D3D9Client.cfg` | D3D9Client renderer: terrain, gravitational lensing and other render parameters |
| `Localization.cfg` | UI language |

#### 2.4.1 Language (Options → UI)

| Key | Default | Meaning |
|---|---|---|
| `Language` | en | UI language: `zh` = Chinese, `en` = English |

#### 2.4.2 Visual (Options → Visual)

| Key | Default | Meaning |
|---|---|---|
| `EnableTerrainHorizonShadow` | TRUE | Terrain self-shadowing switch (in the "Planetary effects" group; the body needs terrain elevation data) |
| `RelativityDoppler` | FALSE | Doppler effect |
| `RelativityAberration` | FALSE | Reference-frame effects (aberration) |
| `RelativityLensing` | FALSE | Gravitational light bending (lensing) |
| `RelativityExposure` | 2.0 | Doppler exposure adaptation white point (0 ~ 8) |

> Gravitational redshift (`RelativityGravRedshift`) follows the Doppler switch.

#### 2.4.3 Physics and relativity (Options → Physics / Extra → Relativity effects / Extra → debug rendering options)

| Key | Default | Meaning |
|---|---|---|
| `Relativity` | FALSE | Relativistic physics master switch |
| `RelativityGravityMethod` | 0 | Gravity model: 0 = 1PN (default), 1 = Kerr, 2 = Newtonian |
| `RelativityCharge` | FALSE | Charge-induced gravity (Reissner–Nordström) |
| `RelativityLensWireframe` | FALSE | Lensing wireframe (debug display) |
| `RelativityBodyMarkers` | FALSE | Relativity-shifted body position markers (debug display) |

#### 2.4.4 Damage and failure simulation (Options → Vessel)

| Key | Default | Meaning |
|---|---|---|
| `DamageModel` | 0 | 0 = off (no wrecks), 1 = on |

#### 2.4.5 Contact model (Extra → Contact model)

| Key | Default | Meaning |
|---|---|---|
| `ContactSolver` | 0 | Contact solver: 0 = official default, 1 = implicit damping, 2 = fully implicit |
| `ContactGearStiffness` | 0 | Landing gear contact stiffness [N/m]; 0 = vessel original value |
| `ContactGearDampingRatio` | 0 | Landing gear damping ratio; 0 = original damping |
| `ContactGearFriction` | 0 | Landing gear friction coefficient; 0 = original |
| `ContactHullStiffness` | 0 | Hull/body contact stiffness [N/m]; 0 = original |
| `ContactHullDampingRatio` | 0 | Hull/body damping ratio; 0 = original damping |
| `ContactHullFriction` | 0 | Hull/body friction coefficient; 0 = original |

> Contact point allocation: the first 3 are landing gear (wheels), the rest are hull/body.

#### 2.4.6 Tips

- Modify only the keys you need; leave the rest at their defaults, keep unknown lines untouched.
- Terrain self-shadowing costs frame time; on low-end GPUs untick it in the Launchpad.

---

## 3. New Features

### 3.1 Relativistic physics

- New gravity models: added 1PN (weak-field approximation) and Kerr (with frame dragging) relativistic gravity models
- New relativistic dynamics: added relativistic mass and time dilation (expressed through the time acceleration factor)
- New bodies: added black hole and neutron star
- New relativity test scenarios: added the `NeutronStar`, `StellarBlackHole` and `SupermassiveBlackHole` scenarios (`Scenarios\Relativity\`)
- New charge-induced gravity: added the Reissner–Nordström charge correction
- New protection: added superluminal, strong-field zone and numerical anomaly protection (vessel destroyed with a message; superluminal scenarios are speed-limited on load)
- New settings: Options → Physics (master switch), Extra → Relativity effects (gravity model / charge)

### 3.2 Relativistic visuals

- New Doppler effect: added the effect of velocity and gravity on light wavelength and brightness
- New reference-frame effects: added observer reference-frame effects caused by velocity and gravity
- New gravitational light bending: added gravitational light bending (mainly seen as gravitational lensing)
- New settings: Options → Visual, each effect can be toggled independently

### 3.3 Chinese localization

- New Chinese UI: added Chinese translations for all Launchpad pages and Options pages, built-in and common add-on MFDs, Scenario Editor, vessel editor pages, glass-cockpit panel and HUD, help texts, scenario/tutorial descriptions, body / launch-site / constellation names
- New Chinese layout adaptation: added Chinese layout adaptation for buttons, labels and dialogs (no clipping, no overlap)
- New body name format: added the "Chinese name (English alias)" format, e.g. 土卫六（泰坦）
- New internationalization API: added translation lookup (`oapiTr`, `oapiTrContext`, `oapiTrName`), layout data query (`oapiGetLayoutInt/Float/String`) and Chinese dialog layout fitting (`oapiZhFitControlText/Row/Height`, `oapiZhOffsetControls/Window`, `oapiZhLayoutDialogButtons`, `oapiZhFitDialogButtons`); translations and layout data can be edited in the installation directory without recompiling

### 3.4 Terrain rendering optimization

- New terrain self-shadowing: added terrain shadowing of sunlight (mountains cast shadows on shaded slopes and valleys, following the sun's azimuth and altitude; only near the surface of the camera body, body needs elevation data)
- New terrain preloading system: added preloading of terrain data for upcoming detail levels, reducing low-detail pop-in and stutter when entering a scenario or switching views
- New terrain loading and switching optimization: added optional detail-level transition, load wait, loader threads and high-altitude elevation skipping
- New settings: Options → Visual → Planetary effects → Terrain self-shadowing; other parameters see 6.4

### 3.5 Performance optimization

- New physics/rendering parallelism: added parallel execution of physics computation and rendering; about 25~30% higher fps in physics-heavy scenarios
- New multi-core compute: added parallel update of independent vessels in multi-vessel scenarios
- New render optimization: added on-demand skipping of daytime star field / gravitational lensing and reuse of lensing results

### 3.6 New contact solver

- New contact solvers: added implicit damping and fully implicit contact solvers (official explicit remains the default)
- New settings page: added the Extra → Contact model page to choose the solver and adjust landing gear / hull contact parameters (stiffness, damping ratio, friction; 0 = vessel original value)
- New contact parameter tuning: added re-tuned DeltaGlider landing gear / hull contact parameters

### 3.7 Wreck and impact system

- New damage events: added vessel-vessel collision (relative speed above threshold), ground/water impact, atmospheric burn-up at excessive dynamic pressure, and direct destruction from relativistic protection
- New wreck model generation: added real hull wreck models cut from the vessel shape (up to 8 pieces)
- New damage physics: added gravity, collision and terrain physics for the wrecks; they stop integrating after settling and are cleaned up about 30 s after creation or about 10 s after settling
- New settings: Options → Vessel → "damage and failure simulation" (wreck switch); collision thresholds etc. see 6.4

### 3.8 Parameter tuning

- New autopilot parameter tuning: added tuning of the DeltaGlider AAP airspeed / altitude / bank / heading hold control parameters
- New hover altitude hold parameter tuning: added tuning of the HOLD ALT hover altitude hold parameters
- New thermal parameter tuning: added adjustments to the vessel thermal control system parameters to rationalize compartment temperatures

### 3.9 Thermal control system optimization

- New true empty-mass model: added structural equivalent thermal masses derived from the true empty vessel mass; fixed temperature anomalies caused by zero propellant/cabin mass
- New mixed explicit/half-implicit time integration: added half-implicit thermal solving for large steps (high time acceleration), improving stability
- Fixed solar irradiance/shadow direction: added irradiance and shadow direction calculation from the actual sun position; fixed lighting/shadow anomalies caused by the sun-position assumption
- Fixed coolant loop connection: corrected the right-wing heat exchanger loop connection
- New temperature floor and clamping: added the CMB temperature floor and temperature clamping, eliminating negative temperatures and NaN
- New numerical failure reporting: added recording and reporting of thermal numerical anomalies

---

## 4. Official Bug Fixes

- Fixed the official TransX MFD crash bug (uninitialised parent-body pointer during solar system initialisation)
- Fixed the official TransX apoapsis latitude NaN bug
- Fixed the official TransX bug that printed a wrong launch azimuth when no solution exists
- Fixed the official TransX stack buffer overflow bug
- Fixed the official Scenario Editor bug where the reference body was wrong after the body combo box was sorted (selection set by unsorted index)
- Fixed the official bug where surface base buildings floated on high-resolution non-flat terrain (placed on the lowest terrain point within the footprint, since v1.3.0)
- Synced other official upstream bug fixes (ImGui viewport, distance display, ...)

---

## 5. Version History

| Version | Highlights |
|---|---|
| v0.9 ~ v1.0.6 | Localization framework, core UI/MFD/Scenario Editor/vessel localization, TransX fixes |
| v1.0.7 | Added the `D3D9ClientDXVK` (DXVK/Vulkan) rendering engine variant |
| v1.1 | Glass-cockpit panel/HUD localization, localization coverage completion, developer APIs |
| v1.2.0 | Relativistic physics extension (1PN/Kerr, frame dragging, relativistic mass, time dilation, FTL protection) |
| v1.2.1 | Relativistic visuals (Doppler, aberration, gravitational lensing) |
| v1.2.2 | Localization completion, Newtonian gravity model, debug option adjustments |
| v1.2.3 | Lensing refinements, neutron star blue-white rendering, aberration fixes |
| v1.2.4 | Sky-box lensing blend refinements |
| v1.2.5 | Charge-induced gravity (Reissner–Nordström), synced with the latest official Orbiter 2024 |
| v1.3.0 | Wreck/debris system (impact destruction, wreck messages/camera, atmospheric burn-up), base buildings on terrain, dynamic strong-field criterion, translation completion |
| v1.3.1 | External layout data (editable layout without rebuilding), translation completion |
| v1.3.2 | Terrain rendering optimization (terrain self-shadowing, terrain preloading, loading and switching optimization); performance optimization (physics/rendering parallelism, multi-core compute, rendering optimization); new contact solvers; wreck and impact system (damage events, wreck model generation, damage physics); parameter tuning (autopilot, hover altitude hold, thermal parameters); thermal control system optimization (mass model, time integration, numerical stability); Chinese layout refinements; internationalization API |

---

## 6. Developer Notes

### 6.1 Build

Requirements: Visual Studio 2022 (MSVC 14.44+), DirectX SDK (June 2010), CMake 3.31+.

```powershell
# Configure (VS 2022 generator, x64)
cmake -S . -B build64 -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_INSTALL_PREFIX="<source dir>/install64" `
  -DDXSDK_DIR="D:/Microsoft DirectX SDK (June 2010)"

# Release build
cmake --build build64 --config Release --parallel 6

# Install into install64
cmake --install build64 --config Release
```

> **Note**: D3D9Client shaders are loaded at runtime from `Modules/D3D9Client/*.hlsl` in the installation directory.
> After editing a shader under `OVP/D3D9Client/shaders/`, copy it to `Modules/D3D9Client/` or the change will not take effect.

### 6.2 Code organization (official-file hooks)

All extension code lives in separate `.inc` / resource files; the official sources only keep `#include` lines and a few conditional calls:

- **Relativity**:
  - Physics: `Src/Orbiter/RelativityPhysics.inc`
  - Configuration: `Src/Orbiter/RelativityConfig.inc` (main program), `OVP/D3D9Client/RelativityConfig.inc` (renderer)
  - Rendering: `OVP/D3D9Client/Relativity.inc`
  - UI: `Src/Orbiter/RelativityUi.cpp` / `RelativityUi.h` / `OrbiterRelativity.rc` / `RelativityResources.h`
  - Shared physics library: `OVP/D3D9Client/shaders/RelativityMapping.hlsl` (single implementation of aberration, gravitational aberration, frame dragging and lensing deflection)
- **Localization**:
  - Core implementation: `i18n/core/` (Localization, MfdZh/InstrumentZh/ZhCommon layout algorithms, `DlgLayoutApi.inc` layout/dialog API implementation, `OptionsVisualZh.inc`)
  - Data: `i18n/locales/zh_CN/*.po` (loaded at runtime), `i18n/layout/zh_CN/layout.ini` (layout values)
  - Modules: `i18n/d3d9client/`, `i18n/scneditor/`, `i18n/vessels/`, `i18n/plugins/`
  - SDK declarations: `Orbitersdk/include/I18nAPI.h` (layout/dialog API)
- **Terrain**: `OVP/D3D9Client/TerrainRender.inc` (hooked into official `Surfmgr2.cpp`/`Tilemgr2.cpp`/`D3D9Client.cpp` as `TERRAINRENDER_SURF/TILES/LOADWAIT/HORIZON` parts), `TerrainRenderMembers_D3D9Config.inc` (config members), `shaders/NewPlanet.hlsl` (terrain self-shadowing mask pass)
- **Performance**: `Src/Orbiter/ParallelUpdate.inc` + `ParallelUpdateConfig.inc` (multi-core per-vessel compute), `PhysicsThread.inc` (physics thread + main-thread rendering), `VesselUpdate.inc`, `OVP/D3D9Client/SceneRender2D.inc`, `PerfStats.h` / `PerfStats.inc` (frame profiler)
- **Contact model**: `Src/Orbiter/ContactConfig.inc` (configuration), `TabExtraContact.inc` (Launchpad page), `ContactDialogRes.rc` / `ContactDialogLang.rc` / `ContactResources.h` (dialog resources)
- **Wreck**: `Src/Wreck/WreckDebris.inc` (spawn/physics/cleanup), `WreckApi.inc`, `WreckConfig.inc`, `WreckDebris.h`
- **Thermal (DeltaGlider)**: `Src/Vessel/DeltaGlider/ThermalModel.inc` + `ThermalModelMembers_CoolantLoop.inc` / `ThermalModelMembers_ThermalSubsystem.inc`
- **Class member blocks**: `*Members_*.inc` (e.g. `OrbiterMembers_Orbiter.inc`, `VesselMembers_Vessel.inc`, `ClientMembers_D3D9Client.inc`, `SceneMembers_Scene.inc`, `ProjectConfigMembers_*.inc`) — new member declarations are collected here; the official header only grows by one include line
- **Tools**: `tools/smoke_test.ps1` (automated smoke test), `tools/run_game.cmd` (command-line launcher for local builds)

> The principle is minimal changes to official sources: thermal, terrain, i18n layout, contact dialog resources and class member declarations all live in separate files; the official shadow logic is unchanged and terrain self-shadowing is an independent overlay.

### 6.3 Developer APIs

| API | Description |
|---|---|
| `const char *oapiTr(const char *english)` | Translation lookup: returns the translation (UTF-8) in Chinese mode, the original string in English mode |
| `const char *oapiTrContext(const char *context, const char *english)` | Context-aware translation lookup (for the same English text with different meanings per context) |
| `const char *oapiTrName(const char *english)` | Name translation (celestial body names etc., with normalization) |
| `int oapiZhActive()` | 1 when Chinese mode is active, 0 otherwise |
| `void oapiZhHudText(int x, int y, int size, const char *utf8)` | Registers a glass-cockpit HUD annotation (Chinese mode) |
| `int oapiGetLayoutInt/Float/String(section, key, def)` | Query layout data from `i18n/layout/zh_CN/layout.ini` (English mode returns the default value) |
| `oapiZhFitControlText/Row/Height`, `oapiZhOffsetControls/Window`, `oapiZhLayoutDialogButtons`, `oapiZhFitDialogButtons` | Chinese dialog layout helpers: fit control width/height to the Chinese text, repack button rows, offset controls ("official layout + offset" rule; English mode does not touch the layout) |
| `bool oapiGetRelativitySpin(OBJHANDLE hObj, double *aM, VECTOR3 *axis)` | Relativistic spin parameters (a/M and spin axis) of a celestial body |
| `double oapiGetRelativityTimeDilation()` | Relativistic time dilation at the focus vessel (total / gravitational / velocity) |
| `oapiGetRelativityGravTimeFactor()` / `oapiGetRelativityVelTimeFactor()` | Gravitational / velocity time-dilation factors |
| `bool oapiGetWreckMessageEx(char *buf, int n, double *age_ms, int *isFocus, char *ship, int shipN)` | Retrieve the current wreck message (UTF-8, including focus-vessel flag and ship name) |
| `OBJHANDLE oapiGetWreckAnchorVessel()` | Wreck observation anchor (reserved interface; the anchor mechanism is not enabled in this version and always returns 0) |
| `void oapiWreckReportNumericFailure(OBJHANDLE hVessel, double maxTemp, int compartment)` | Report a numerical failure to the wreck system (for vessel modules; currently only queued/logged — the destruction policy is a reserved interface, the vessel is not deleted) |

**Configuration parameters (SDK macros / CFGPRM)**

| Header | Content |
|---|---|
| `Orbitersdk/include/GraphicsAPI.h` | New `CFGPRM_TERRAINHORIZONSHADOW` (terrain self-shadowing master switch, bool) |
| `Orbitersdk/include/ContactAPI.h` | `CFGPRM_CONTACTSOLVER` (contact solver); the old experimental friction model `CFGPRM_CONTACTFRICTION` was removed |
| `Orbitersdk/include/WreckAPI.h` | `CFGPRM_WRECKS` / `CFGPRM_RELCOLLISIONDV` / `CFGPRM_RELGASGIANT` |
| `Orbitersdk/include/RelativityAPI.h` | `CFGPRM_REL*` relativity parameters (unchanged) |
| `Orbitersdk/include/I18nAPI.h` | Layout/dialog API declarations (new public header, moved out of `OrbiterAPI.h`) |

**Removed/changed APIs (breaking changes)**

- Old 2D debris rendering API: `oapiGetWreckDebrisCount` / `oapiGetWreckDebris` (not needed with real vessel wrecks);
- Legacy scene shadow-mode API: `oapiSetVesselShadowMode` / `oapiGetVesselShadowMode` and `SHADOWMODE_*` (removed; the official shadow path is unchanged);
- The layout API declarations (`oapiGetLayout*`, `oapiZhFit*`, `oapiZhOffset*`) moved to `I18nAPI.h` (the functions themselves are unchanged).

**Add-on MFD localization**: in Chinese mode all buttons are font-rendered by the main program; a plugin only needs to return Chinese labels in Chinese mode (`oapiZhActive()`), see `i18n/plugins/TransXZh.inc`.

**Lua**: `oapi.get_time_dilation()` etc. (unchanged).

**Dual-thread / multi-core switches**: the macro `DUAL_THREAD_EXPERIMENT` at the top of `Src/Orbiter/Orbiter.cpp` (1 = physics thread + main-thread rendering, default; 0 = official single-thread loop, requires a rebuild); the multi-core per-vessel compute is controlled by the `VesselParallelUpdate` config key.

**Verification**: build → install → verify Chinese UI → switch to English → switch back to Chinese (device rebuild) to confirm no crash; verify relativity scenarios; verify terrain self-shadowing and the contact model on the ground.

### 6.4 Configuration parameters (developers)

This section lists the configuration items **without a UI** (files in the game root directory). Same as 2.4: a **Launchpad restart** is required (restarting the scenario alone does not apply them); edit them while Orbiter is closed (the files are written back on exit).

**Orbiter.cfg (main program)**

| Key | Default | Range | Meaning |
|---|---|---|---|
| `VesselParallelUpdate` | TRUE | TRUE / FALSE | Multi-core compute switch: independent vessels are updated in parallel in multi-vessel scenarios |
| `VesselParallelRange` | 50000 | 1000 ~ 1e7 | Vessels closer than this to any other vessel [m] are updated in sequence (possible interaction) |
| `Wrecks` | TRUE | TRUE / FALSE | Additional wreck system switch (requires `DamageModel`) |
| `RelativityCollisionDVThreshold` | 10.0 | 1 ~ 1e6 | Relative speed threshold [m/s] above which a vessel-vessel collision destroys the vessel |
| `RelativityGasGiantFactor` | 0.8 | 0.1 ~ 1.0 | Gas-giant crushing radius factor |
| `RelativityChargeRatio` | 0.5 | 0 ~ 1 | Charge ratio q = r_Q/r_s |
| `RelativityGravRedshift` | TRUE | TRUE / FALSE | Gravitational redshift of the background sky (follows the Doppler switch in Options → Visual) |

**D3D9Client.cfg (renderer) — terrain self-shadowing**

| Key | Default | Range | Meaning |
|---|---|---|---|
| `TerrainHorizonShadow` | 1 | 0 / 1 | Renderer-side switch (both this and the Orbiter.cfg master switch must be on) |
| `TerrainHorizonFieldSize` | 32 | 16 ~ 128 | Shadow detail; higher = finer but more expensive |
| `TerrainHorizonDirs` | 16 | 8 ~ 32 (even) | Smoothness of the sun-azimuth interpolation; higher = smoother but more expensive |
| `TerrainHorizonRangeKm` | 300 | 50 ~ 500 | Maximum shadow search distance [km] |
| `TerrainHorizonStrength` | 75 | 0 ~ 100 | Shadow strength [%] |
| `TerrainHorizonThreads` | 1 | 1 ~ 3 | Shadow computation threads |
| `TerrainHorizonCacheMB` | 64 | 8 ~ 512 | Shadow data cache budget [MB] |
| `TerrainHorizonMaxAlt` | 50000 | 0 ~ 1e6 | Camera altitude [m] above which self-shadowing is disabled |
| `TerrainHorizonMinLevel` | 4 | 0 ~ 20 | Only apply to terrain at this detail level or finer |
| `TerrainHorizonUpdate` | 4 | 1 ~ 16 | Shadow update check interval [frames] |
| `TerrainHorizonDebug` | 0 | 0 ~ 3 | Debug view (0 = normal shadows; 1/2/3 semantics see 6.6) |
| `TerrainHorizonContour` | 200 | 0 ~ 5000 | Debug contour interval [m] (0 = off) |
| `TerrainHorizonShadowFloor` | 40 | 0 ~ 90 | Minimum brightness kept inside shadows [%] (approximates sky/ambient light) |
| `TerrainHorizonSoftness` | 27 | 1 ~ 300 | Soft shadow edge width in 0.01 degrees; 27 = 0.27°, i.e. the sun disc radius |

**D3D9Client.cfg (renderer) — terrain loading / detail transition**

| Key | Default | Range | Meaning |
|---|---|---|---|
| `WaitTerrainLoad` | 1 | 0 / 1 | Wait for the initial terrain load before the simulation starts |
| `TerrainLoadWaitTimeout` | 30 | 1 ~ 120 | Maximum terrain load wait [s] |
| `TerrainLoadWaitMinMs` | 600 | 0 ~ 5000 | Minimum terrain load wait [ms] (kept even when the tiles are already cached) |
| `WaitTerrainLoadCamera` | 1 | 0 / 1 | Re-run the terrain load wait when the camera target changes |
| `TerrainPreloadLevels` | 1 | 0 ~ 2 | Number of detail levels to preload ahead |
| `TerrainTransitionMs` | 0 | 0 ~ 2000 | Terrain detail transition duration [ms]; 0 = off (default), 200 ~ 350 recommended for smooth switches |
| `TerrainElevMinLevel` | 10 | 0 ~ 20 | Elevation skipping: above `TerrainElevSkipAlt` do not read elevation files for tiles below this detail level |
| `TerrainElevSkipAlt` | 50000 | 0 ~ 1e6 | Camera altitude [m] above which elevation files are skipped (0 = always apply) |
| `TerrainLoadThreads` | 1 | 1 ~ 4 | Terrain tile loader threads |

**D3D9Client.cfg (renderer) — relativistic visuals / gravitational lensing**

| Key | Default | Range | Meaning |
|---|---|---|---|
| `LensCubemapSize` | 1024 | 512 / 1024 / 2048 | Lensing resolution (higher = smoother boundary, more memory and cost) |
| `LensBlendStart` | 0.2 | 0 ~ 45 | Apparent shift [deg] below which the plain star field is kept |
| `LensBlendEnd` | 1.0 | 0 ~ 90 | Apparent shift [deg] above which the lensing effect is used |
| `LensBrightness` | 1.0 | -10 ~ 4 | Lensing brightness gain (negative values are debug modes, see 6.6) |
| `RelativityRenderCache` | 1 | 0 / 1 | Lensing render cache: skip re-rendering while the parameters are unchanged; automatically disabled in the strong-field / high-speed regime |

**i18n/layout/zh_CN/layout.ini (Chinese layout data)**

Chinese-mode layout values (control widening/heightening, offsets, font sizes, button gaps, ...) are read at runtime (UTF-8 INI); changes need no recompile and take effect after switching the language once; missing sections/keys fall back to built-in defaults, and English mode does not read the file.

| Section | Content |
|---|---|
| `[mfd.*]` / `[panel.*]` / `[hud.*]` | Font sizes / coordinate offsets for MFDs, glass-cockpit panels and the HUD |
| `[dialog.*]` | Win32 dialog control offsets and button repacking parameters |
| `[dialog.visual]` (new in v1.3.2) | Launchpad Options → Visual page layout offsets: `groupboxExtraWidth` (group box extra width, default 12 DLU), `planetaryExtraHeight` (extra height of the "planetary effects" group box, default 8 DLU), `lowerBlockYOffset` (offset of the content below, default 9 DLU) |

**Implementation notes**

- The contact model damping ratio ζ is converted at runtime to a damping coefficient `c = 2ζ√(k·m)` (k = effective stiffness, m = vessel mass); solvers 1/2 evaluate the damping / spring force at the end of the step, which improves stability at large step sizes.
- Settings are exposed to graphics clients through the `CFGPRM_*` macros (`Orbitersdk/include/*API.h`). When adding a new graphics/physics parameter, update the API header and both configuration sides (main program `Config`, renderer `D3D9Config`/`RelativityConfig.inc`) and keep the official files minimal (put the read/write implementation into a separate `.inc`).
- Debug/diagnostic settings (terrain self-shadowing debug views, lensing debug, frame profiler, ...) are described in 6.6.

### 6.5 Automated tests

- `tools/smoke_test.ps1`: starts the given scenario via `CreateProcess` (default `Delta-glider\DG and DG-S`, override with `-Scenario`; bypasses the Windows security prompt for unsigned exes), polls the log with a timeout/hang watchdog, takes screenshots and reports PASS/FAIL (about 25 s);
- `tools/run_game.cmd`: starts the locally built `Orbiter.exe` from the command line.

### 6.6 Debug views and performance logging

**Terrain self-shadowing debug views** (`D3D9Client.cfg`):

| Key | Default | Meaning |
|---|---|---|
| `TerrainHorizonDebug` | 0 | 0 = normal shadows; 1 = sun visibility red (dark) / yellow (lit) + contours; 2 = horizon angle field + contours; 3 = normal shading + contours. 1/2 are overlay debug views — keep 0 for normal play |
| `TerrainHorizonContour` | 200 | Debug contour interval [m] (0 = off) |

**Lensing debug**: negative `LensBrightness`: `-1` raw cubemap, `-2` hole weight map, `-3` hole boundary line; the lensing wireframe and relativity body markers are available in `Extra → debug rendering options`.

**Frame profiler** (`D3D9Client.cfg`):

| Key | Default | Meaning |
|---|---|---|
| `PerfLog` | 0 | 0 = off (zero overhead); 1 = log CPU stage timings to `Orbiter.log` (one `PERF:` line every 2 s); 2 = additionally profile the GPU (stalls the pipeline, so the profiled frames are not representative); 3 currently equals 2 |

---

## 7. License and Credits

### License

- Orbiter itself is under the original [MIT license](https://github.com/orbitersim/orbiter/blob/main/LICENSE)
- The D3D9Client graphics engine is under LGPL (see [LGPL](./OVP/D3D9Client/LGPL.txt))

### Credits

- Localization, relativity extension and testing: [@orbiter-newcomer](https://github.com/orbiter-newcomer)
- AI-assisted development: DeepSeek (translation, code changes, bug investigation, documentation)
