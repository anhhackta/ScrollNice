<div align="center">

# 🖱️ ScrollNice

> **Scroll by Clicking — no physical mouse wheel needed**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%2010%2F11-0078d4?logo=windows)](https://github.com/anhhackta/ScrollNice/releases)
[![Chrome Extension](https://img.shields.io/badge/Chrome-Extension-4285F4?logo=googlechrome&logoColor=white)](https://github.com/anhhackta/ScrollNice)
[![Size: <500KB](https://img.shields.io/badge/Size-%3C500KB-green)](https://github.com/anhhackta/ScrollNice/releases)
[![Free & Open Source](https://img.shields.io/badge/Free-%26%20Open%20Source-orange)](https://github.com/anhhackta/ScrollNice)

<p align="center">
<a href="https://github.com/anhhackta/ScrollNice/blob/main/README_vn.md">
    <img src="https://hatscripts.github.io/circle-flags/flags/vn.svg" width="20">
</a>
  <!-- Keep these links. Translations will automatically update with the README. -->
 <a href="https://github.com/anhhackta/ScrollNice">
    <img src="https://hatscripts.github.io/circle-flags/flags/us.svg" width="35">
</a>
</p>

**ScrollNice** is a Windows utility and Chrome Extension that allows you to scroll by **clicking, holding, or just hovering** over a floating zone (Scroll Zone). Perfect for broken scroll wheels, one-handed workflows, or an alternative scrolling experience.

🌐 **Website:** [anhhackta.github.io/ScrollNice](https://anhhackta.github.io/ScrollNice/)

</div>

---

## ✨ Key Features

| Feature | Description |
|---------|-------------|
| 🖱️ **3 Scroll Modes** | Click/Hold · Top/Bottom Split · Auto-Scroll on Hover |
| 🪶 **Ultra Lightweight** | < 500KB, CPU ≈ 0% at idle, RAM < 10MB |
| 🔒 **No Telemetry** | No data collected, no ads, completely offline |
| ⌨️ **Hotkeys** | Ctrl+Alt+S to toggle, Ctrl+Alt+E for edit mode. Fully customizable |
| 🎨 **Customizable** | Resize/move zones, adjust opacity, sensitivity, edit click sounds |
| 🚫 **Wheel Block** | Optionally block physical wheel scroll events globally |
| 🌐 **Chrome Extension** | Lightweight browser version, load as unpacked |
| 🔄 **Portable** | No installation required, just extract the zip and run the `.exe` |

---

## 🎮 3 Scroll Modes

### Mode 1 — Click / Hold
- **Left click** → scroll up ↑
- **Right click** → scroll down ↓
- **Hold mouse button** → continuous scroll with acceleration

### Mode 2 — Top / Bottom Split
- The zone is split into two halves (top / bottom)
- **Click/Hold top half** → scroll up ↑
- **Click/Hold bottom half** → scroll down ↓

### Mode 3 — Auto-Scroll on Hover *(Special)*
- **No click required!**
- Hover top half → auto scroll up ↑
- Hover bottom half → auto scroll down ↓

---

## 📥 Installation

### Windows App (Portable)
1. Download [`ScrollNice-portable.zip`](https://github.com/anhhackta/ScrollNice/releases) from Releases.
2. Extract the file and run `ScrollNice.exe`.
3. The scroll zone will appear — simply move your cursor into it to start scrolling.

> Settings are loaded from `./config.json` next to the executable.

### Chrome Extension (FeelClick)
1. Download the source code or clone the repository.
2. Open Chrome and navigate to `chrome://extensions/`.
3. Enable **Developer mode** and click **Load unpacked**.
4. Select the `extension/FeelClick/` directory.

### Build from source (C++17 / CMake)
```bash
git clone https://github.com/anhhackta/ScrollNice.git
cd ScrollNice
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Requirements: MSVC 2019+ or MinGW, CMake ≥ 3.20

---

## ⌨️ Default Hotkeys

| Hotkey | Action |
|--------|--------|
| `Ctrl+Alt+S` | Enable / Disable ScrollNice (Panic Key) |
| `Ctrl+Alt+E` | Toggle Edit Mode (Drag/Resize Zones) |
| `Ctrl+Alt+Z` | Toggle the next zone |
| `Ctrl+Alt+W` | Toggle physical wheel blocking |

> You can fully customize these hotkeys in `config.json`.

---

## ⚙️ Configuration (`config.json`)

```json
{
  "version": 1,
  "enabled": true,
  "wheel_block_mode": "off",
  "bypass_modifier": "Alt",
  "hotkeys": {
    "toggle_enabled":  "Ctrl+Alt+S",
    "toggle_edit":     "Ctrl+Alt+E",
    "toggle_zone":     "Ctrl+Alt+Z",
    "toggle_wheel":    "Ctrl+Alt+W"
  },
  "engine": {
    "sensitivity":      1.2,
    "dead_zone_px":     2,
    "smoothing":        0.85,
    "accel_power":      1.35,
    "tick_ms":          10,
    "max_events_per_sec": 120
  },
  "zones": [
    {
      "id":          "floating_zone",
      "type":        "floating",
      "x": 100, "y": 100,
      "width": 120, "height": 200,
      "mode":        "click_step",
      "locked":      false
    }
  ],
  "exclusions": {
    "auto_suspend_fullscreen": true,
    "process_names": []
  },
  "ui": {
    "visible":      true,
    "hover_alpha":  0.15,
    "active_alpha": 0.35
  }
}
```

### `wheel_block_mode` Options

| Value | Meaning |
|-------|---------|
| `off` | Do not block physical wheel scrolling (default) |
| `global` | Block physical wheel scrolling system-wide |
| `outside_zone_only` | Allow wheel scrolling ONLY when inside a zone |
| `inside_zone_only` | Block wheel scrolling ONLY when inside a zone |

---

## 📁 Project Structure

```
ScrollNice/
├── src/
│   ├── core/           # ZoneManager, ScrollEngine, StateMachine, Config
│   ├── platform/win/   # WinMouseHook, WinOverlay, WinInputInjector, WinTray
│   └── ui/             # Zone rendering logic
├── extension/
│   └── FeelClick/      # Chrome Extension version (MV3)
├── docs/               # Website source code (GitHub Pages)
│   ├── css/style.css
│   ├── js/             # main.js, i18n.js, docs-i18n.js
│   ├── docs/           # Documentation pages
│   └── assets/
├── tests/              # Unit tests (ScrollEngine, HitTest algorithms)
├── CMakeLists.txt
└── config.json         # Default app configuration
```

---

## �️ Security & Privacy

- ❌ Automatically telemetry is prohibited. No data leaves your machine.
- ❌ Does not read the contents of your screen, active windows, or clipboard.
- ✅ Does not inject DLLs into foreign processes.
- ✅ Only utilizes `WH_MOUSE_LL` (Windows standard low-level mouse hook) specifically to read positioning and capture events.
- ✅ Will automatically attempt to suspend itself if a fullscreen app or defined game process is in the foreground.

> **Note:** Because the application utilizes `WH_MOUSE_LL` to read the global cursor position (to determine if you are inside a hover zone), security software may sometimes flag it generically. The source code is entirely open and available in `src/`.

---

## 🤝 Support the Project

ScrollNice is entirely **free and open source**. If it helps to improve your workflow, consider buying me a coffee!

- ☕ **Ko-fi:** [ko-fi.com/hoang2k2](https://ko-fi.com/hoang2k2)
- 💳 **PayPal:** [paypal.me/bahoang2k2](https://paypal.me/bahoang2k2)

---

## � License

Distributed under the MIT License — see the [LICENSE](LICENSE) file for details.

---

## 👨‍💻 Author

**HoàngX** ([@anhhackta](https://github.com/anhhackta))  
Website: [anhhackta.github.io/ScrollNice](https://anhhackta.github.io/ScrollNice/)

---

*Made with ❤️ in Vietnam 🇻🇳*
