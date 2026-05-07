# ScrollNice Slint UI Migration Design Document

**Date:** 2026-05-08
**Version:** 1.0
**Status:** Approved

---

## Executive Summary

This document outlines the complete migration of ScrollNice from C++/Win32 to Rust/Slint UI framework. The migration aims to provide a modern, maintainable codebase with Windows 11 Fluent Design aesthetics, theme support (light/dark), internationalization (English/Vietnamese), and extensible preset/profile systems.

---

## Goals

1. **Full UI Migration:** Replace all Win32 UI components with Slint
2. **Modern Design:** Windows 11 Fluent Design aesthetics
3. **Theme Support:** Light and dark themes with easy switching
4. **Internationalization:** English and Vietnamese support, extensible for more languages
5. **Presets System:** Built-in presets for common applications (Chrome, VS Code, Edge, Firefox)
6. **Profiles System:** Multiple configuration profiles with save/load functionality
7. **Wheel Blocking:** Configurable wheel blocking modes
8. **Auto-detect Scroll Target:** Intelligent scroll target detection

---

## Non-Goals

1. Linux/macOS support in this phase (though architecture will support it)
2. Plugin system (deferred to future phases)
3. Advanced visual effects (particles, animations beyond basic transitions)

---

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Slint UI Layer                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ MainWindow   │  │ ZoneOverlay  │  │ TrayMenu     │  │
│  │ (Settings)   │  │ (Scroll)     │  │ (System)     │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   Rust Bridge Layer                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ SlintBridge  │  │ Win32Bridge  │  │ ConfigBridge │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   Core Logic (Rust)                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ ScrollEngine │  │ ZoneManager  │  │ StateMachine │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   Platform Layer (Win32)                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ MouseHook    │  │ InputInjector│  │ Hotkeys      │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### Key Changes from C++ Version

1. **Language:** C++ → Rust for core logic
2. **UI Framework:** Win32/GDI → Slint
3. **Config:** JSON → serde_json with Rust structs
4. **Threading:** Single-threaded UI → Async/await where appropriate

---

## Components

### MainWindow (Settings)

**Purpose:** Main configuration window

**Features:**
- Tab-based layout: General, Zone, Scroll, Presets, Profiles
- Theme toggle (light/dark)
- Language selector (English/Vietnamese)
- Live preview of zone settings
- All current settings controls

**Slint Window Configuration:**
- Standard window with title bar
- Minimize/Close buttons
- Size: 800x600 (resizable)
- Position: Center screen on first launch
- Remember position/size on close

### ZoneOverlay

**Purpose:** Floating scroll zone

**Features:**
- Visual feedback for 3 scroll modes (ClickHold, SplitHold, HoverAuto)
- Edit mode with resize/drag handles
- Lock indicator
- Click-through support (WS_EX_TRANSPARENT)
- Theme-aware styling

**Slint Window Configuration:**
- Borderless window
- Always on top
- Click-through (WS_EX_TRANSPARENT)
- Layered window for transparency
- No taskbar entry
- Custom window class for Win32 integration

### TrayMenu

**Purpose:** System tray integration

**Features:**
- Toggle enable/disable
- Toggle edit mode
- Open settings
- Quit

**Implementation:**
- Win32 tray icon (Slint doesn't support system tray directly)
- Bridge between Win32 tray events and Slint UI

---

## Data Flow

### Config Flow

```
Config JSON → ConfigStore → Slint UI (display/edit)
                ↓
         User changes → ConfigStore → Save to JSON
                ↓
         Apply to Core Logic (ScrollEngine, ZoneManager)
```

### Scroll Flow

```
Mouse Hook → ZoneManager → StateMachine → ScrollEngine
                                          ↓
                                    InputInjector → Win32 API
```

### Preset/Profile Flow

```
User selects preset → Load preset config → Apply to ConfigStore
User saves profile   → Save current config → Add to profiles list
```

---

## Theme System

### Theme Structure

```json
{
  "themes": {
    "light": {
      "background": "#FFFFFF",
      "foreground": "#000000",
      "accent": "#0078D4",
      "secondary": "#F3F3F3",
      "border": "#E0E0E0",
      "success": "#107C10",
      "warning": "#FF8C00",
      "error": "#E81123"
    },
    "dark": {
      "background": "#202020",
      "foreground": "#FFFFFF",
      "accent": "#60CDFF",
      "secondary": "#2D2D2D",
      "border": "#404040",
      "success": "#4CC9F0",
      "warning": "#F72585",
      "error": "#7209B7"
    }
  }
}
```

### Slint Integration

- Global theme variables in Slint
- Reactive theme switching
- Persist theme choice in config
- Theme preview in settings

---

## Internationalization (i18n)

### Translation Structure

```
locales/
  en.json
  vi.json
```

### English (en.json)

```json
{
  "app_name": "ScrollNice",
  "menu": {
    "toggle_enabled": "Toggle Enabled",
    "toggle_edit": "Toggle Edit Mode",
    "settings": "Settings",
    "quit": "Quit"
  },
  "settings": {
    "general": "General",
    "zone": "Zone",
    "scroll": "Scroll",
    "presets": "Presets",
    "profiles": "Profiles"
  },
  "scroll_modes": {
    "click_hold": "Click & Hold",
    "split_hold": "Split Hold",
    "hover_auto": "Hover Auto"
  },
  "wheel_block": {
    "off": "Off",
    "global": "Global (when zone active)",
    "outside_zone": "Outside Zone Only",
    "inside_zone": "Inside Zone Only"
  }
}
```

### Vietnamese (vi.json)

```json
{
  "app_name": "ScrollNice",
  "menu": {
    "toggle_enabled": "Bật/Tắt",
    "toggle_edit": "Chế độ chỉnh sửa",
    "settings": "Cài đặt",
    "quit": "Thoát"
  },
  "settings": {
    "general": "Tổng quan",
    "zone": "Vùng cuộn",
    "scroll": "Cuộn",
    "presets": "Mẫu cài đặt",
    "profiles": "Hồ sơ"
  },
  "scroll_modes": {
    "click_hold": "Click & Giữ",
    "split_hold": "Chia & Giữ",
    "hover_auto": "Hover Tự động"
  },
  "wheel_block": {
    "off": "Tắt",
    "global": "Toàn bộ (khi zone hoạt động)",
    "outside_zone": "Chỉ ngoài zone",
    "inside_zone": "Chỉ trong zone"
  }
}
```

### i18n System Features

- Runtime language switching
- Fallback to English if translation missing
- Easy to add new languages (just add JSON file)
- Translation key validation at build time

---

## Presets System

### Built-in Presets

1. **Chrome:** Optimized for web browsing
2. **VS Code:** Optimized for code editing
3. **Edge:** Optimized for Edge browser
4. **Firefox:** Optimized for Firefox browser

### Preset Structure

```json
{
  "name": "Chrome",
  "description": "Optimized for Chrome browser",
  "config": {
    "scroll": {
      "mode": "hover_auto",
      "scroll_amount": 3,
      "continuous_speed": 5,
      "continuous_accel": 1.2,
      "hover_speed": 3
    },
    "zone": {
      "width": 60,
      "height": 400,
      "opacity": 0.3
    }
  }
}
```

### Custom Presets

- User can create custom presets
- Edit existing presets
- Delete custom presets
- Export/import presets

---

## Profiles System

### Profile Structure

```json
{
  "profiles": [
    {
      "id": "default",
      "name": "Default",
      "config": { /* full config */ }
    },
    {
      "id": "gaming",
      "name": "Gaming",
      "config": { /* full config */ }
    }
  ],
  "current_profile": "default"
}
```

### Profile Features

- Multiple profiles
- Switch between profiles
- Create new profiles
- Delete profiles
- Export/import profiles

---

## Wheel Blocking

### Wheel Block Modes

```rust
enum WheelBlockMode {
    Off,              // Không chặn
    Global,           // Chặn toàn bộ khi zone active
    OutsideZoneOnly,  // Chặn khi ngoài zone
    InsideZoneOnly,   // Chặn khi trong zone
}
```

### Implementation

- Hook WM_MOUSEWHEEL events
- Check current mode and cursor position
- Block or allow based on configuration
- Bypass modifier key support (e.g., Alt to bypass)

---

## Auto-detect Scroll Target

### Implementation

- Use `WindowFromPoint` Win32 API
- Fallback with offset positions if needed
- Cache target window to avoid frequent lookups
- Handle multi-monitor scenarios

### Behavior

- Detect window under cursor when entering zone
- Send scroll events to detected window
- Update target when cursor leaves zone
- Fallback to foreground window if detection fails

---

## File Structure

```
ScrollNice/
├── Cargo.toml
├── build.rs
├── config.json
├── locales/
│   ├── en.json
│   └── vi.json
├── presets/
│   ├── chrome.json
│   ├── vscode.json
│   ├── edge.json
│   └── firefox.json
├── src/
│   ├── main.rs
│   ├── lib.rs
│   ├── core/
│   │   ├── mod.rs
│   │   ├── config.rs
│   │   ├── zone.rs
│   │   ├── scroll_engine.rs
│   │   └── state_machine.rs
│   ├── platform/
│   │   ├── mod.rs
│   │   └── win32/
│   │       ├── mod.rs
│   │       ├── mouse_hook.rs
│   │       ├── input_injector.rs
│   │       ├── hotkeys.rs
│   │       └── window.rs
│   ├── ui/
│   │   ├── mod.rs
│   │   ├── main_window.slint
│   │   ├── zone_overlay.slint
│   │   ├── bridge.rs
│   │   └── components/
│   │       ├── mod.rs
│   │       ├── settings_tab.slint
│   │       ├── zone_preview.slint
│   │       └── preset_list.slint
│   ├── i18n/
│   │   ├── mod.rs
│   │   └── translations.rs
│   └── themes/
│       ├── mod.rs
│       └── theme.rs
└── docs/
    └── superpowers/
        └── specs/
            └── 2026-05-08-slint-ui-migration-design.md
```

---

## Error Handling

### Config Errors

- Invalid JSON → Use defaults + log warning
- Missing fields → Use defaults
- Corrupt file → Backup + recreate

### UI Errors

- Window creation failed → Show error dialog + exit
- Theme load failed → Fallback to default theme
- Translation missing → Fallback to English

### Runtime Errors

- Hook installation failed → Log + continue without hook
- Input injection failed → Retry + log
- Hotkey registration failed → Log + continue

---

## Testing Strategy

### Unit Tests

- Config serialization/deserialization
- ScrollEngine math (smoothing, dead-zone)
- Zone hit-testing
- i18n translation loading
- Theme loading

### Integration Tests

- Config → UI sync
- UI → Config sync
- Preset application
- Profile switching
- Language switching
- Theme switching

### Manual Tests

- All 3 scroll modes
- Theme switching
- Language switching
- Wheel blocking modes
- Multi-monitor support
- Preset application
- Profile switching

---

## Performance

### Targets

- Idle CPU: < 1%
- Idle RAM: < 20MB
- Active CPU: < 5%
- Active RAM: < 50MB

### Optimizations

- Lazy load translations
- Cache theme colors
- Minimize redraws
- Efficient event handling
- Use `#[inline]` for hot paths

---

## Dependencies

### Core Dependencies

```toml
[dependencies]
slint = "1.0"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tokio = { version = "1.0", features = ["full"] }
anyhow = "1.0"
tracing = "0.1"
tracing-subscriber = "0.3"
```

### Platform Dependencies

```toml
[target.'cfg(windows)'.dependencies]
windows = { version = "0.52", features = [
    "Win32_Foundation",
    "Win32_UI_WindowsAndMessaging",
    "Win32_UI_Input_KeyboardAndMouse",
    "Win32_System_LibraryLoader",
    "Win32_Graphics_Gdi"
]}
```

---

## Migration Strategy

### Phase 1: Core Infrastructure (Days 1-3)

1. Set up Rust project structure
2. Implement ConfigStore with serde
3. Implement core logic modules (ScrollEngine, ZoneManager, StateMachine)
4. Set up Win32 platform layer

### Phase 2: Slint UI Foundation (Days 4-6)

1. Create Slint bridge layer
2. Implement MainWindow with basic tabs
3. Implement ZoneOverlay with basic rendering
4. Set up theme system

### Phase 3: Features Implementation (Days 7-10)

1. Implement all settings controls
2. Implement preset system
3. Implement profile system
4. Implement i18n system

### Phase 4: Integration & Polish (Days 11-12)

1. Integrate all components
2. Implement wheel blocking
3. Implement auto-detect scroll target
4. Testing and bug fixes

### Phase 5: Build & Release (Day 13)

1. Create release build
2. Test on clean system
3. Create installer/portable package
4. Documentation update

---

## Success Criteria

1. All existing features work in new implementation
2. UI matches Windows 11 Fluent Design
3. Theme switching works correctly
4. Language switching works correctly
5. Preset system functional
6. Profile system functional
7. Wheel blocking modes work correctly
8. Performance targets met
9. No regressions from C++ version

---

## Open Questions

None at this time.

---

## Appendix A: Scroll Modes Detail

### ClickHold Mode

- Left click: Scroll up
- Right click: Scroll down
- Hold: Continuous scroll with acceleration

### SplitHold Mode

- Top half of zone: Scroll up
- Bottom half of zone: Scroll down
- Click anywhere in half: Scroll in that direction
- Hold: Continuous scroll with acceleration

### HoverAuto Mode

- Hover in top half: Auto scroll up
- Hover in bottom half: Auto scroll down
- No click required
- Speed based on hover position

---

## Appendix B: Config Schema

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
  "sound": {
    "enabled": false
  }
}
```

---

## Appendix C: Preset Schema

```json
{
  "presets": [
    {
      "id": "chrome",
      "name": "Chrome",
      "description": "Optimized for Chrome browser",
      "icon": "chrome",
      "config": {
        "scroll": {
          "mode": "hover_auto",
          "scroll_amount": 3,
          "continuous_speed": 5,
          "continuous_accel": 1.2,
          "hover_speed": 3
        },
        "zone": {
          "width": 60,
          "height": 400,
          "opacity": 0.3
        }
      }
    }
  ]
}
```

---

## Appendix D: Profile Schema

```json
{
  "profiles": [
    {
      "id": "default",
      "name": "Default",
      "created_at": "2026-05-08T00:00:00Z",
      "config": { /* full config */ }
    }
  ],
  "current_profile": "default"
}
```

---

**End of Design Document**
