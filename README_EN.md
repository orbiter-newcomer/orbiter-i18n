# Orbiter Chinese Localization + Relativity Extension

## 1. Introduction

This repository is a **64-bit branch** of [Orbiter](https://github.com/orbitersim/orbiter), the open-source space flight simulator. On top of the official code it provides:

- **Full Chinese localization**: UI / MFDs / Scenario Editor / vessels / glass-cockpit HUD / help texts; English mode is identical to the official upstream behaviour
- **Relativistic physics and visual extension**: 1PN/Kerr gravity fields, frame dragging, relativistic mass, time dilation, Doppler shift, aberration, gravitational lensing (the "hole" method), gravitational redshift, black hole / neutron star rendering, FTL and strong-field protection, charge-induced gravity

- Repository: https://github.com/orbiter-newcomer/orbiter-i18n
- Current version: **v1.2.5** (synced with the latest official Orbiter 2024 upstream)
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

**English mode**: identical to official upstream Orbiter — all localization changes are gated behind Chinese mode only.

### 2.3 Relativity settings

1. **Options → Physics**: master switch for relativistic physics (when off, no relativistic physics or visuals are applied);
2. **Options → Visual**: visual-effect switches — Doppler shift, aberration, gravitational lensing, gravitational redshift;
3. **Extra → Relativity effects**: gravity model (Newton / 1PN / Kerr) and the charge-induced gravity switch;
4. **Extra → debug rendering options**: lensing wireframe, body position markers, etc.;
5. Built-in relativity test scenarios: the **black hole / supermassive black hole / neutron star** scenarios under `Scenarios\Relativity\`.

### 2.4 Gravitational lensing ("hole" method) parameters

In `D3D9Client.cfg` (installation root):

| Setting | Meaning | Default |
|---|---|---|
| `LensCubemapSize` | lensing cubemap resolution (512/1024/2048) | 2048 |
| `LensBlendStart` | apparent shift [deg] below which the plain star field is kept | 0.2 |
| `LensBlendEnd` | apparent shift [deg] above which the lensed cubemap is used | 1.0 |
| `LensBrightness` | lensed cubemap brightness gain | 1.0 |

Debug modes (`LensBrightness`): `-1` raw cubemap, `-2` hole weight map, `-3` hole boundary line.

> The vertex displacement limit of the plain star field is automatically bound to the hole blend parameters (`min(2°, LensBlendEnd)`), so parameter changes cannot produce stretched-mesh artifacts.

---

## 3. New Features

### 3.1 Relativistic physics

1. **Lorentz transformation referenced to the global simulation time**, with time dilation expressed through time acceleration (velocity Lorentz factor × gravitational time-dilation factor);
2. **Selectable relativistic gravity model**: **Newtonian** (classical), **1PN** (first post-Newtonian, weak field), **Kerr** (exact strong field, including frame dragging);
3. **Optional charge-induced gravity**: Reissner–Nordström charge correction for compact bodies (repulsive acceleration term, time dilation / redshift correction); applies to relativistic bodies only;
4. **New relativistic bodies**: black holes and neutron stars;
5. **Superluminal and strong-field protection**: speed limiting and strong-field fallback to prevent crashes and numerical blow-ups.

### 3.2 Relativistic visuals

1. **Doppler effect**: red/blue shift and brightness correction caused by velocity and gravity;
2. **Reference-frame effects**: observer reference-frame effects caused by velocity and gravity (abbreviated as **velocity aberration** and **gravitational aberration**);
3. **Gravitational light bending**: gravitational lensing by the "hole" method — large deflections are rendered with a lensed cubemap, small deflections keep the plain sharp star field (smooth boundary);
4. **Charge corrections to gravitational visuals**: charge corrections to the lensing deflection and gravitational aberration.

### 3.3 Chinese localization

- **Core UI**: all Launchpad pages, Options pages, Object Info, MFD menus/buttons, menu bar, Win32 dialogs, main-window ImGui dialogs
- **MFDs**: built-in MFDs (Orbit/Surface/Map/HSI/VOR/Docking/Align/Sync/Transfer/COM-NAV) + add-on MFDs (Terminal/Generic Camera/Ascent/AscentAP/Attitude RCS/TransX)
- **Scenario Editor**: full UI localization
- **Vessel editor pages and configurators**: DeltaGlider / ShuttleA / Launchpad vessel configuration
- **2D glass-cockpit panel and HUD**: MFD buttons, panel buttons (power/select/menu), navigation buttons, RCS buttons, HUD readout labels and Chinese annotations
- **Astronomy and labels**: 88 constellation names, body names, 39 launch-site names; surface features stay in English
- **Fonts**: Noto Sans CJK SC; fonts are rebuilt automatically after a runtime language switch

### 3.4 Debug and developer tools

- Lensing debug modes (negative `LensBrightness`), lensing wireframe, body position markers
- Shared physics library `OVP/D3D9Client/shaders/RelativityMapping.hlsl` (single source of truth for the relativistic direction mapping)
- Developer APIs: `oapiTr` / `oapiZhActive` / `oapiZhHudText`, Lua time-dilation interfaces (see "Developer Notes")

---

## 4. Bug Fixes

### 4.1 Localization and stability fixes

| Component | Issue | Fix |
|---|---|---|
| TransX | **Crash (0xc0000005) as soon as the MFD opens**: uninitialised parent-body pointer while initialising the solar system | Initialise the pointer, skip NaN distances, fall back to the sun when no parent is found |
| TransX | Apoapsis latitude shown as NaN | Normalize by position vector magnitude |
| TransX | Launch azimuth printed even when no solution exists | Standard formula + N/A when unsolvable |
| TransX | Stack buffer overflow / UTF-8 truncation garbage | Enlarge buffers, safe formatting |
| Surface data | 20 `.mkr` files with non-ASCII characters | Normalize to ASCII |
| D3D9Client | ImGui mojibake after runtime language switch | Always merge CJK font into the atlas |
| Core | Chinese text mangled by ANSI APIs | Use W APIs (UTF-8 → UTF-16) |
| Core / HUD | Stale cached HUD font after language switch or device rebuild → crash | Release the HUD font cache in the device-destroy callback, recreate on demand |
| D3D9Client / GenericCamera | Generic Camera MFD fails with the DXVK engine variant | Try `D3D9Client.dll` / `D3D9ClientDXVK.dll` in order |
| Panel buttons | Wrong font-size fallback; RCS buttons disappearing | Fixed fallback strategy; static button coordinate cache |

### 4.2 Relativity-related fixes

- Lensing deflection applied to the back side of bodies (fake large deflection behind the observer)
- Seam at the "hole" boundary of the sky-box composite (plain star field now uses the same exact mapping as the cubemap composite, strictly reciprocal)
- Hard blend-band seams, shadow-region overpaint, and wrongly bent milky-way background
- Stretched-mesh artifacts in the blend band (vertex-displacement fallback criterion + vertex limit bound to the hole parameters)
- Upstream fixes included by synchronisation: ImGui viewport fix, distance display improvements, etc.

---

## 5. Version History

| Version | Highlights |
|---|---|
| v0.9 ~ v1.0.6 | Localization framework, core UI/MFD/Scenario Editor/vessel localization, mojibake fixes, TransX crash fix, localization code refactoring (phases 1/2) |
| v1.0.7 | Added the `D3D9ClientDXVK` (DXVK/Vulkan) rendering engine variant |
| v1.1 | Localization architecture refactoring (phase 3, logic/data separation), glass-cockpit panel/HUD localization, developer APIs |
| v1.2.0 | Relativistic physics extension (1PN/Kerr, frame dragging, relativistic mass, time dilation, FTL protection) |
| v1.2.1 | Added relativistic visuals (Doppler, aberration, gravitational lensing) |
| v1.2.2 | Relativity code hook refactoring, localization fixes, Newtonian model, debug options moved |
| v1.2.3 | Lensing "hole" composition, neutron star blue-white rendering, gravitational aberration / back-side deflection fixes |
| v1.2.4 | Sky-box "hole" method refinements (seam fixes, hole debug tools) |
| v1.2.5 | Charge-induced gravity (Reissner–Nordström), vertex limit bound to the hole parameters, vertex-displacement fallback criterion, synced with the latest official 2024 upstream |

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

- All relativity/localization code lives in separate `.inc` files; official sources only keep `#include` lines and a few conditional calls:
  - Relativity physics: `Src/Orbiter/RelativityPhysics.inc`
  - Relativity configuration: `Src/Orbiter/RelativityConfig.inc` (main program), `OVP/D3D9Client/RelativityConfig.inc` (renderer)
  - Relativity rendering: `OVP/D3D9Client/Relativity.inc`
  - Relativity UI: `Src/Orbiter/RelativityUi.cpp` / `RelativityUi.h` / `OrbiterRelativity.rc` / `RelativityResources.h`
  - Localization logic: `Src/Orbiter/ZhCommon.inc`; data: `MfdZh.inc` / `InstrumentZh.inc` / `DialogZh.inc` / `LaunchpadZh.inc`
- **Shared physics library**: `OVP/D3D9Client/shaders/RelativityMapping.hlsl` — the single implementation of aberration, gravitational aberration, frame dragging and lensing deflection; both the vertex path and the cubemap composite call it, so physics changes are made in one file
- **Render settings**: relativity switches are exposed through `CFGPRM_REL*` (`Orbitersdk/include/RelativityAPI.h`) from the main program to the graphics client; physics and visuals share the same switches and data

### 6.3 Developer APIs

| API | Description |
|---|---|
| `const char *oapiTr(const char *english)` | Translation lookup: returns the translation (UTF-8) in Chinese mode, the original string in English mode |
| `int oapiZhActive()` | 1 when Chinese mode is active, 0 otherwise |
| `void oapiZhHudText(int x, int y, int size, const char *utf8)` | Registers a glass-cockpit HUD annotation (Chinese mode) |
| `bool oapiGetRelativitySpin(OBJHANDLE hObj, double *aM, VECTOR3 *axis)` | Relativistic spin parameters (a/M and spin axis) of a celestial body |
| `double oapiGetRelativityTimeDilation()` | Relativistic time dilation at the focus vessel (total / gravitational / velocity) |
| `oapiGetRelativityGravTimeFactor()` / `oapiGetRelativityVelTimeFactor()` | Gravitational / velocity time-dilation factors |

- **Add-on MFD localization**: in Chinese mode all buttons are font-rendered by the main program; a plugin only needs to return Chinese labels in Chinese mode (`oapiZhActive()`), see `Src/Plugin/TransX/TransXZh.inc`;
- **Lua**: `oapi.get_time_dilation()` etc.;
- **Verification**: build → install → verify Chinese UI → switch to English → switch back to Chinese (device rebuild) to confirm no crash.

### 6.4 Relativity configuration (developers)

- Physics: `Orbiter.cfg` — `Relativity` / `RelativityGravityMethod` / `RelativityCharge` / `RelativityChargeRatio`, etc.;
- Visuals: `D3D9Client.cfg` — `LensCubemapSize` / `LensBlendStart` / `LensBlendEnd` / `LensBrightness`, etc.;
- Settings are exposed to graphics clients through `CFGPRM_REL*`; when adding a new effect, update `RelativityAPI.h` and both configuration sides.

---

## 7. License and Credits

### License

- Orbiter itself is under the original [MIT license](https://github.com/orbitersim/orbiter/blob/main/LICENSE)
- The D3D9Client graphics engine is under LGPL (see [LGPL](./OVP/D3D9Client/LGPL.txt))

### Credits

- Localization, relativity extension and testing: [@orbiter-newcomer](https://github.com/orbiter-newcomer)
- AI-assisted development: DeepSeek (translation, code changes, bug investigation)
