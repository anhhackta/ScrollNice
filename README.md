<div align="center">

# 🖱️ ScrollNice

> **Scroll by clicking — no physical mouse wheel required**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%2010%2F11-0078d4?logo=windows)](https://github.com/anhhackta/ScrollNice/releases)
[![Chrome Extension](https://img.shields.io/badge/Chrome-Extension-4285F4?logo=googlechrome&logoColor=white)](https://github.com/anhhackta/ScrollNice)
[![Rust](https://img.shields.io/badge/stack-Rust%20%2B%20Slint%20(migration)-DEA584?logo=rust)](https://github.com/anhhackta/ScrollNice)
[![Pages](https://img.shields.io/badge/docs-GitHub%20Pages-222?logo=github)](https://anhhackta.github.io/ScrollNice/)

<p align="center">
  <a href="https://github.com/anhhackta/ScrollNice/blob/main/README_vn.md" title="Tiếng Việt">
    <img src="https://hatscripts.github.io/circle-flags/flags/vn.svg" width="22" alt="VN">
  </a>
  <a href="https://github.com/anhhackta/ScrollNice/blob/main/README.md" title="English">
    <img src="https://hatscripts.github.io/circle-flags/flags/us.svg" width="22" alt="EN">
  </a>
</p>

**ScrollNice** is a **Windows** system utility and a **Chrome extension** (FeelClick). Use a floating **scroll zone**: **click**, **hold**, or **hover** to scroll — ideal for broken wheels, one-handed use, or a different scrolling feel.

🌐 **Site:** [anhhackta.github.io/ScrollNice](https://anhhackta.github.io/ScrollNice/) · **Sources:** `docs/` (GitHub Pages)

</div>

---

## Highlights

| | |
|--|--|
| **Three modes** | Click/hold · top/bottom split · hover auto-scroll |
| **Small native build** | Classic release is a tiny portable **`.exe`** (see [Releases](https://github.com/anhhackta/ScrollNice/releases)) |
| **No telemetry** | Offline-first; no ads |
| **Hotkeys** | Defaults: `Ctrl+Alt+S` (toggle), `Ctrl+Alt+E` (edit zone), `Ctrl+Alt+W` (wheel block) — configurable in `config.json` |
| **Wheel block** | Optional; hold **`Alt`** to bypass (when configured) |
| **Chrome** | Load unpacked from `extension/FeelClick/` |

---

## Install

### Windows (portable, from Releases)

1. Download **`ScrollNice-portable.zip`** from [Releases](https://github.com/anhhackta/ScrollNice/releases).
2. Extract and run **`ScrollNice.exe`**.
3. Settings: **`config.json`** next to the executable.

> The shipped binary today is built from the **C++ / Win32** stack. A **Rust + Slint** UI rewrite is in progress (see [Migration status](MIGRATION_STATUS_2026-05-08.md)).

### Chrome extension (FeelClick)

1. Clone or download this repo.
2. Chrome → `chrome://extensions/` → enable **Developer mode** → **Load unpacked**.
3. Choose **`extension/FeelClick/`**.

---

## Build from source

### C++ application (current release path)

**Requirements:** CMake ≥ 3.15, MSVC 2019+ (or compatible toolchain).

```powershell
git clone https://github.com/anhhackta/ScrollNice.git
cd ScrollNice
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output: `build/Release/ScrollNice.exe` (layout may vary slightly by generator).

### Rust + Slint (migration / preview)

**Requirements:** [Rust stable](https://rustup.rs/), same repo root.

```powershell
# Debug (nhanh nhất để compile lặp, binary lớn hơn)
cargo build
# .\target\debug\scrollnice-rs.exe

# Thử tối ưu vừa phải — compile nhanh hơn --release
cargo build --profile quick
# .\target\quick\scrollnice-rs.exe

# Bản phát hành tối ưu
cargo test
cargo build --release
# .\target\release\scrollnice-rs.exe
```

Design and schema: **`docs/superpowers/specs/2026-05-08-slint-ui-migration-design.md`** · Progress: **`MIGRATION_STATUS_2026-05-08.md`**.

### Which `.exe` should I run?

| Goal | Run this |
|------|----------|
| **Normal use** | Download from [Releases](https://github.com/anhhackta/ScrollNice/releases) → `ScrollNice.exe` inside the zip (C++ build). |
| **After CMake** (`cmake --build …`) | `build\Release\ScrollNice.exe` (path may vary, e.g. `build\x64\Release\`). |
| **After `cargo build`** | `target\debug\scrollnice-rs.exe` |
| **After `cargo build --profile quick`** | `target\quick\scrollnice-rs.exe` |
| **After `cargo build --release`** | `target\release\scrollnice-rs.exe` |

Only one of these is needed at a time. **`target\`** holds Rust outputs; **`build\`** holds CMake outputs — both are safe to delete and will be recreated on the next build (they are gitignored).

---

## Configuration (`config.json`)

The **Rust** baseline uses **schema version 2** (see spec). Example:

```json
{
  "version": 2,
  "enabled": true,
  "language": "en",
  "theme": "dark",
  "current_profile": "default",
  "hotkeys": {
    "toggle_enabled": "Ctrl+Alt+S",
    "toggle_edit": "Ctrl+Alt+E",
    "toggle_wheel": "Ctrl+Alt+W"
  },
  "scroll": {
    "mode": "hover_auto",
    "scroll_amount": 3,
    "continuous_speed": 5,
    "continuous_accel": 1.2,
    "hover_speed": 3
  },
  "zone": {
    "x": 0,
    "y": 100,
    "width": 60,
    "height": 400,
    "opacity": 0.3,
    "locked": false
  },
  "wheel_block": "off",
  "wheel_block_bypass_modifier": "Alt",
  "start_with_windows": false,
  "sound": { "enabled": false },
  "profiles": []
}
```

The legacy **C++** build may still expect an older shape for some fields; if you mix binaries, keep a backup of `config.json`. Full reference: [docs](https://anhhackta.github.io/ScrollNice/docs/settings.html).

---

## Repository layout

```
ScrollNice/
├── Cargo.toml, build.rs          # Rust + Slint (migration)
├── CMakeLists.txt                # Native Windows app (C++17)
├── config.json                   # Default / sample config (v2 for Rust)
├── ui/                           # Slint UI (.slint)
├── src/
│   ├── main.rs, lib.rs
│   ├── core/                     # Rust: config, zone, scroll_engine, state_machine (+ C++ counterparts)
│   ├── platform/win/             # C++ Win32 implementation
│   ├── platform/win32/           # Rust Win32 stubs / bridge (in progress)
│   ├── i18n/, themes/, ui/
│   └── main.cpp                  # C++ entry
├── extension/FeelClick/          # Chrome MV3 extension
├── locales/, presets/            # Rust i18n + presets
├── docs/                         # GitHub Pages (landing + guides)
│   ├── index.html, changelog.html, favicon.svg
│   ├── css/, js/
│   └── docs/                     # Documentation pages
├── docs/superpowers/specs/       # Approved design specs
├── MIGRATION_STATUS_2026-05-08.md
├── ScrollNiceDoc                 # Optional narrative history (Vietnamese / English)
└── vendor/                       # C++ headers (e.g. nlohmann json)
```

Build artifacts: **`build/`** (CMake) and **`target/`** (Rust) are ignored by Git — generate locally.

---

## Security & privacy

- **No telemetry** — data stays on your machine.
- **No** screen scraping, clipboard harvesting, or foreign-process DLL injection.
- Uses the standard Windows **`WH_MOUSE_LL`** hook for pointer events needed by the scroll zone. Some AV tools may flag low-level hooks generically; the code here is open for review.

---

## Support the project

ScrollNice is **free and open source**. If it helps your workflow:

- ☕ [Ko-fi — hoang2k2](https://ko-fi.com/hoang2k2)
- 💳 [PayPal — bahoang2k2](https://paypal.me/bahoang2k2)

---

## License

[MIT License](LICENSE)

---

## Author

**HoàngX** · [@anhhackta](https://github.com/anhhackta) · [Project site](https://anhhackta.github.io/ScrollNice/)

*Made with care in Vietnam 🇻🇳*
