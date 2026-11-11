# Orbiter Chinese Localization (English Summary)

This repository is a **Chinese localization branch** of [Orbiter](https://github.com/orbitersim/orbiter), the open-source space flight simulator. It is based on the latest upstream code (64-bit build).

- Localization repo: https://github.com/orbiter-newcomer/orbiter-i18n (localization code on the `zh-cn` branch)
- Current version: **v1.1**

---

## Switching the language

### Option 1: In-game (recommended)

1. Start `Orbiter.exe` to open the **Launchpad**
2. Click **Options**
3. Open the **UI** page
4. Select **Chinese (Simplified)** in the **Language** drop-down
5. The change takes effect immediately and is saved

### Option 2: Configuration file

Edit `Localization.cfg` in the game root directory:

```ini
; Chinese
Language=zh

; English (default)
Language=en
```

---

## English mode

**English mode is byte-for-byte identical to upstream Orbiter.** All localization changes are gated behind Chinese mode only; the English UI, MFDs and text keep the original official behaviour. This is a hard constraint of the project.

---

## What is localized

- **Launchpad / Options pages / dialogs / Object Info window / MFD menus / input boxes / menu bar**
- **Main-window ImGui dialogs** (Camera, Object info, Time acceleration, Map, Custom functions, ...)
- **Built-in MFDs**: Orbit, Surface, Map, HSI, VOR/VTOL, Docking, Align Planes, Sync Orbit, Transfer, COM/NAV
- **Add-on MFDs** (each plugin provides its own Chinese button labels): Terminal (LuaMFD), Generic Camera, Ascent profile, AscentAP, Attitude RCS, TransX
- **Scenario Editor** (full UI: base/body selectors, date page, attitude page, docking page, propellant buttons, etc.)
- **Vessel editor pages & vessel configurators**: DeltaGlider, ShuttleA; Launchpad "Vessel configuration" (DG Configuration / Atlantis Configuration)
- **2D glass cockpit panel & HUD** (new in v1.1): all MFD buttons, PWR/SEL/MNU, panel power button, nav-mode buttons (自稳/水平/顺行/逆行/法向+/法向-/定高), RCS mode buttons (关闭/旋转/平移), HUD mode titles, reference-body names, readout labels, NAV→接收机, airbrake annotation
- **Planetarium**: 88 constellation names in Chinese
- **Label mode**: body names and 39 launch-site names in Chinese (surface features such as craters/cities stay in English)
- **Fonts**: Noto Sans CJK SC for Chinese; runtime language switching supported

---

## Upstream / add-on bugs fixed along the way

| Component | Issue | Fix |
|---|---|---|
| TransX | **Crash (0xc0000005) as soon as the MFD opens**: uninitialised parent-body pointer while initialising the solar system | Initialise the pointer, skip NaN distances, fall back to the sun when no parent is found |
| TransX | Apoapsis latitude shown as NaN (asin normalized by planet radius) | Normalize by position vector magnitude |
| TransX | Launch azimuth printed even when no solution exists | Use standard formula `cos(i)=cos(φ)·sin(A)`; show N/A when unsolvable |
| TransX | Stack buffer overflow crash | Enlarge buffers (20/30 → 64 bytes) |
| TransX | UTF-8 truncation garbage | Enlarge buffers / safe formatting |
| TransX | Duplicate variable-name draw causing overlap | Remove duplicate draw |
| Data | 20 `.mkr` files with non-ASCII characters | Normalize to ASCII (Sao Paulo / Xi'an etc.) |
| D3D9Client | ImGui mojibake after runtime language switch | Always merge CJK font into atlas |
| Core | Chinese text mangled by ANSI APIs | Use W APIs (UTF-8 → UTF-16) |
| Core / HUD | After language switch or scene reload (graphics-device rebuild) the cached HUD CJK font still referenced a destroyed device → crash when opening a scenario | Release the HUD font cache in the device-destroy callback and recreate on demand |
| D3D9Client / GenericCamera | Generic Camera MFD fails to open with the DXVK engine variant (plugin looked up `D3D9Client.dll` only) | Try `D3D9Client.dll` / `D3D9ClientDXVK.dll` in order |
| Panel buttons | Wrong font size fallback made Chinese labels tiny; RCS button coordinates re-read after being hidden made buttons disappear | Fixed fallback strategy; button positions cached statically and refreshed when panel geometry changes |

---

## Build

Requirements: Visual Studio 2022, DirectX SDK (June 2010), CMake 3.31+.

Debug:

```powershell
cmake --build build64 --config Debug --parallel 6
```

Release (Visual Studio / CMake generator, 64-bit, 6 parallel jobs):

```powershell
cmake -S . -B build64 -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_INSTALL_PREFIX="<source dir>/install64" `
  -DDXSDK_DIR="D:/Microsoft DirectX SDK (June 2010)"
cmake --build build64 --config Release --parallel 6
cmake --install build64 --config Release
```

### Install note

- `cmake --install` automatically creates the full directory skeleton of `Textures2\` (high-resolution texture override folder, searched before `Textures\`) mirroring the `Textures\` structure. High-res textures can then simply be dropped into the matching subfolder (same-named files override the low-res versions).

### Render engine selection (optional)

The Launchpad **Video** tab offers two D3D9Client engine variants (same renderer, different D3D9 implementations):

- **D3D9Client**: system Direct3D9 / Direct3D9On12 (default, upstream behaviour)
- **D3D9ClientDXVK**: renders through **DXVK** (Vulkan; requires a Vulkan runtime)

The DXVK translation-layer DLL lives in `RenderingBackends\dxvk\` (shipped with the distribution package) and is loaded at runtime by the `D3D9ClientDXVK` engine plugin. No DLL is replaced or deleted, and switching engines takes effect immediately through the official engine mechanism. The dgVoodoo2 variant is not included because its global D3D9 API hooking conflicts with the system D3D9 implementation loaded in the same process.

---

## Version history (recent)

| Version | Highlights |
|---|---|
| v1.0.5 | **Localization code refactoring (phase 1)**: localizable data moved into standalone `.inc` files (official source diff reduced by ~27%); MFD layout rewritten as "official coordinates + Chinese offset table"; TransX help text auto-wrap; D3D9Client resources/.rc restored to official English; debug code removed; all plugins loaded via config |
| v1.0.6 | **Localization code refactoring (phase 2)**: layout logic extracted; MFD fixed offsets consolidated into `MfdZh.inc` (no more magic numbers in official sources); `cw`/`ch` redefined as panel-relative units so all MFD layouts scale with the panel and no longer depend on font metrics |
| v1.0.7 | **Rendering engine upgrade**: an additional official graphics-engine plugin variant, `D3D9ClientDXVK` (DXVK/Vulkan), selectable and hot-switchable in the Launchpad video tab (no DLL replacement); the distribution build now creates the full `Textures2` high-res texture directory skeleton |
| v1.1 | **Localization architecture refactoring (phase 3)**: logic and data fully separated (`ZhCommon.inc` holds logic only; `MfdZh` / `InstrumentZh` / `DialogZh` / `LaunchpadZh` hold data only); Launchpad window texts and main-program dialog texts moved into their own files; vessel data split into "in-game" + "tools" files per vessel; add-on MFDs now localize themselves (each plugin ships its own `*Zh.inc`; all MFD buttons are font-rendered in Chinese mode); glass-cockpit panel navigation/RCS buttons and HUD localized; new developer APIs (`oapiZhHudText`, `oapiZhActive`); fixed a crash caused by stale HUD font cache after device rebuild, Generic Camera crash with the DXVK engine, and font-fallback bugs |

---

## Developer APIs and conventions

> For add-on / vessel developers who want to integrate with this localization system. All APIs are declared in `Orbitersdk/include/OrbiterAPI.h`; no extra dependency is needed.

### 1. Public APIs

| API | Description |
|---|---|
| `const char *oapiTr(const char *english)` | Look up the translation table; returns the translation (UTF-8) in Chinese mode and the original string in English mode. Pointer stays valid for the process lifetime |
| `int oapiZhActive()` | Returns 1 when Chinese mode is active, 0 otherwise. Use it for layout offsets or text replacements that must only apply in Chinese mode |
| `void oapiZhHudText(int x, int y, int size, const char *utf8)` | Registers a glass-cockpit HUD annotation in screen coordinates (Chinese mode only); the main program draws it with the CJK font after HUD rendering. Intended for vessel HUD annotations that cannot be expressed with the HUD texture character set |

### 2. Translation table rules (`Src/Orbiter/Localization.cpp`)

- Translation table is an array of `{ English, Chinese }` pairs; forward/reverse lookup maps are built at runtime
- `orbiter::Tr(key)` for normal lookups; `orbiter::Tr2(aliasKey)` for alias keys
- **Alias key rule**: when the same English token has different meanings in different contexts (e.g. Map MFD button `DN` = scroll down, while Align MFD content `DN` = descending node), use a dedicated alias key (`MFD_MAP_DN` etc.). Alias keys are excluded from the reverse map and never pollute Chinese→English lookup
- **Case rule**: entries that differ only by letter case but have different translations are marked ambiguous; `Tr()` then uses strict case-sensitive matching
- Adding new text: add a `{ "English", "中文" }` entry in the proper section; use an alias key if the token conflicts with another context; verify English mode still shows the original string

### 3. Localization data files (repository convention)

| Category | File (`Src/Orbiter/`) | Content |
|---|---|---|
| Logic (process) | `ZhCommon.inc` | Font cache, text-width estimation, centered drawing, button-label layout, MFD layout algorithms, `ZH_PANEL` panel render flow, `ZH_HUD` HUD registration/render flow, public interface implementations |
| MFD data | `MfdZh.inc` | Built-in MFD button-label mapping data, per-MFD layout offset data |
| Other instruments data | `InstrumentZh.inc` | Glass-cockpit panel data (font size specs, PWR/SEL/MNU, nav/RCS button texts) + glass-cockpit HUD data (mode titles, readout labels) |
| Main-program dialog data | `DialogZh.inc` | All main-window ImGui dialog texts |
| Launchpad data | `LaunchpadZh.inc` | Launchpad main window / tabs / Options pages / loading screen texts (section A) + Extra-tab supplement (section B, macro-gated) |

**Rule**: data files contain data/parameters only; drawing flow, font management, layout algorithms and public interfaces live in `ZhCommon.inc`. Data files `#include` the logic file in one direction only (data → MfdZh → ZhCommon), no circular dependencies.

### 4. Vessel / plugin / window data convention

- **At most two files per vessel** (inside the vessel directory):
  - `<Ship>Zh.inc`: in-game vessel UI/HUD helpers
  - `<Ship>ToolsZh.inc`: out-of-game UI (Launchpad "Vessel configuration" dialogs, Scenario Editor vessel pages). When the same file is included by different DLLs, use macro sections (e.g. `DG_ZH_EDITOR` / `DG_ZH_CONFIG`)
  - Examples: `DeltaGliderZh.inc` + `DeltaGliderToolsZh.inc`; `AtlantisToolsZh.inc`; `ShuttleAToolsZh.inc`
- **Window/dialog data**: one `*Zh.inc` per build target (DLL), e.g. `D3D9ClientZh.inc` (three setup dialogs merged), `EditorZh.inc`, `AtmConfigZh.inc`, `MeshdebugZh.inc`
- **Official sources only keep call sites**: `#include` plus a few conditional calls; no Chinese strings inside official `.cpp` files

### 5. Add-on MFD localization convention (plugin developers)

In Chinese mode every MFD button is font-rendered by the main program (character atlas hidden, `ButtonLabel()` string drawn with the CJK font). A plugin only needs to return Chinese labels in Chinese mode:

```cpp
char *MyMFD::ButtonLabel (int bt)
{
    static const char *en[] = {"INP", "NEW", "DEL"};
    static const char *zh[] = {"输入", "新建", "删除"};
    if (bt < 0 || bt >= 3) return 0;
    if (oapiZhActive()) return (char*)zh[bt];   // Chinese mode
    return (char*)en[bt];                        // English mode: original label
}
```

- Keep label data in a `<Plugin>Zh.inc` inside the plugin directory
- If the plugin also needs layout offsets, gate them behind `oapiZhActive()` and return 0 offsets in English mode
- Not-localized third-party MFDs: their English labels are drawn with the font in Chinese mode (readable, no crash); no adaptation required
- Reference implementations: `Src/Plugin/TransX/TransXZh.inc`, `Src/Plugin/LuaMFD/LuaMfdZh.inc`, `Orbitersdk/samples/AscentMFD/AscentMFDZh.inc`, `OVP/D3D9Client/samples/GenericCamera/GenericCameraZh.inc`, and the Lua-MFD unified mapping in `Src/Module/LuaScript/LuaInterpreter/LuaMfdZh.inc`

### 6. Vessel HUD annotations

For Chinese HUD annotations that cannot be expressed with the HUD texture character set, call:

```cpp
oapiZhHudText (hps->CX, (int)(hps->CY - hps->Markersize/2), size, oapiTr("AIRBRK"));
```

The main program queues it into the glass-cockpit HUD Chinese overlay; the call is a no-op in English mode (callers may check `oapiZhActive()` first). Reference: `Src/Vessel/DeltaGlider/DeltaGliderZh.inc`.

### 7. Hard constraints and verification

- **English mode must behave exactly like upstream**: all localization code is inside `orbiter::IsChinese()` branches; no Chinese text, layout/font/colour changes in English mode
- Data files (`*Zh.inc`) contain data only; logic changes go to `ZhCommon.inc` or the module's helper file
- Recommended verification: build (`cmake --build build64 --config Release --parallel 1`), install (`cmake --install build64 --config Release`), then test Chinese UI → switch to English → switch back to Chinese (device rebuild) to confirm no crash

---

## Credits

- Localization & testing: [@orbiter-newcomer](https://github.com/orbiter-newcomer)
- AI-assisted development: DeepSeek (translation, code changes, bug investigation)

## License

MIT, following the original [Orbiter](https://github.com/orbitersim/orbiter) project.
