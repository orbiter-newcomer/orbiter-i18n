# Orbiter Chinese Localization (English Summary)

This repository is a **Chinese localization branch** of [Orbiter](https://github.com/orbitersim/orbiter), the open-source space flight simulator. It is based on the latest upstream code (64-bit build).

- Localization repo: https://github.com/orbiter-newcomer/orbiter-i18n (localization code on the `zh-cn` branch)
- Current version: **v1.0.6**

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

- **Launchpad / Options / dialogs / Object Info window / MFD menus / input boxes / menu bar**
- **Built-in MFDs**: Orbit, Surface, Map, HSI, VOR/VTOL, Docking, Align Planes, Sync Orbit, Transfer, COM/NAV
- **Add-on MFDs**: Terminal (LuaMFD), Generic Camera, Ascent profile, Attitude RCS, TransX (variables, help system, menus, precision levels, data labels, plans)
- **Scenario Editor** (full UI: base/body selectors, date page, attitude page, docking page, propellant buttons, etc.)
- **Vessel editor pages**: DeltaGlider, ShuttleA
- **Planetarium**: 88 constellation names in Chinese
- **Label mode**: body names and 39 launch-site names in Chinese (surface features such as craters/cities stay in English)
- **Terminology**: Eject→出发, Intercept→交会, Through point→飞掠, etc.
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
```

---

## Version history (recent)

| Version | Highlights |
|---|---|
| v1.0.5 | **Localization code refactoring (phase 1)**: localizable data moved into standalone `.inc` files (official source diff reduced by ~27%); MFD layout rewritten as "official coordinates + Chinese offset table"; TransX help text auto-wrap; D3D9Client resources/.rc restored to official English; debug code removed; all plugins loaded via config |
| v1.0.6 | **Localization code refactoring (phase 2)**: layout logic extracted to `MfdLayoutZh.inc`; MFD fixed offsets consolidated into `MfdZh.inc` (no more magic numbers in official sources); `cw`/`ch` redefined as panel-relative units so all MFD layouts scale with the panel and no longer depend on font metrics |

---

## Distribution package (v1.0.4)

- `Orbiter.exe` (Release build with all localization)
- 22 plugin DLLs + Scenario Editor + D3D9Client
- **SPICE ephemeris** `Kernels/orbiter.bsp` (~1.5 GB, required)
- Official low-resolution planet textures (D3D9Moon / D3D9Mars etc.)
- Updated MSVC runtime (14.44)
- Development MFD template (MFDTemplate) disabled

> Note: high-resolution planet textures are distributed separately by the official Orbiter project and are **not** included.

---

## Code refactoring (completed)

> The localization code has gone through two refactoring phases (v1.0.5 / v1.0.6). Goals: **strip localizable content out of upstream sources**, reduce the diff against upstream, and make MFD layouts stable under panel scaling / font-size changes.

- **Localization data lives in standalone files**: `Src/Orbiter/Localization.cpp` (translation table + `Tr()`/`Tr2()`), ten `*Zh.inc` files (strings for plugins/vessels, embedded via `#include`), `MfdZh.inc` (MFD Chinese offset data) and `MfdLayoutZh.inc` (layout helpers)
- **MFD layout rule**: official coordinates are kept; Chinese mode only adds `zhOff_*` offsets; where text width matters (e.g. Docking column positions) the layout uses the measured text width so Chinese labels never overlap
- **`cw`/`ch` are panel-relative units** (`ch=(IH*9)/200`, `cw=(ch*3)/5`): layouts scale proportionally with the MFD panel and are no longer affected by font metrics
- **Hard constraint**: English mode behaves exactly like upstream; all localization changes are gated behind Chinese mode, and D3D9Client resources/.rc are restored to official English

---

## Credits

- Localization & testing: [@orbiter-newcomer](https://github.com/orbiter-newcomer)
- AI-assisted development: DeepSeek (translation, code changes, bug investigation)

## License

MIT, following the original [Orbiter](https://github.com/orbitersim/orbiter) project.
