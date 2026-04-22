# ScrollNice — Zone Startup, Scroll Target Detection & GUI Fix

## Root Cause Analysis

### 🐛 Bug 1: Zone auto-opens on startup
[StateMachine](file:///e:/Game/ScrollNice/src/core/StateMachine.h#12-45) defaults to `AppState::Idle` (line 42 [StateMachine.h](file:///e:/Game/ScrollNice/src/core/StateMachine.h)).  
[ApplyConfig()](file:///e:/Game/ScrollNice/src/main.cpp#242-266) → [SetEnabled(cfg.enabled)](file:///e:/Game/ScrollNice/src/core/StateMachine.h#19-23) → if `cfg.enabled = true` (default) → overlay shows immediately.  
**Expected behavior:** App starts in system tray, zone only shows when user explicitly enables it.

**Fix:**
- Change [StateMachine](file:///e:/Game/ScrollNice/src/core/StateMachine.h#12-45) default state to `AppState::Disabled`
- OR start with `cfg.enabled = false` as default
- Recommended: keep `enabled=true` in config (user preference) but add a **first-run / tray startup** flow that shows zone after a 1-second delay with a balloon tip explaining how to use it

### 🐛 Bug 2: Scroll does NOT reach the right window
`ScrollEngine::SendWheelEvent` calls `SendInput(MOUSEEVENTF_WHEEL)` which sends wheel event to the **currently focused window** — NOT to the window under the mouse cursor.  
When zone is floating, the zone window itself gets focus → scrolling goes nowhere useful.

**Fix:** Use `WindowFromPoint(cursor)` to find the window under the **actual cursor position** (not zone position), then use `PostMessage(target_hwnd, WM_MOUSEWHEEL, ...)` directly to that window.

Detection steps:
```
1. On each click/hover event in zone → call GetCursorPos() to get real cursor position
2. Hide/show zone window briefly? No — use GetWindow under excluding zone hwnd
3. Use WindowFromPoint on a point NEAR the zone (outside it) → or use the last remembered non-zone position
```

Better approach: **remember the last cursor position OUTSIDE the zone** and send scroll to `WindowFromPoint(lastOutsidePos)`. This is the scrollable "target" window.

### 🎨 Bug 3: GUI layout issues
From the screenshot, Settings opens but controls may overflow or group boxes render incorrectly on different DPI.

---

## Architecture Explanation (Code Map)

```
WinMain (main.cpp)
 ├── Creates: MsgWnd (hidden HWND for WM_HOTKEY / WM_TIMER)
 ├── Creates: WinOverlay  → floating zone window (WS_POPUP | TOPMOST | LAYERED)
 ├── Creates: WinTray     → system tray icon + context menu
 ├── Registers: WinHotkeys → Ctrl+Alt+S/E/W  (global hotkeys via RegisterHotKey)
 └── Runs: GetMessage loop (MSG pump)

Zone Events (WinOverlay → callback → main.cpp::OnZoneEvent)
 ├── LeftClickDown/Up   → HandleZoneClick() → ScrollEngine.ClickScroll()
 ├── RightClickDown/Up  → HandleZoneClick() → ScrollEngine.ClickScroll()
 └── HoverMove/Leave    → HandleZoneHover() → StartHoverScroll() / StopHoverScroll()

Scroll Timers (in main.cpp message loop)
 ├── TIMER_ID_HOLD (ID:201, 16ms=~60fps) → ContinuousScrollTick → SendInput(WHEEL)
 └── TIMER_ID_HOVER (ID:202, 16ms)       → ContinuousScrollTick → SendInput(WHEEL)

ScrollEngine (ScrollEngine.cpp)
 ├── ClickScroll(dir, px)         → immediate SendInput(MOUSEEVENTF_WHEEL)
 ├── ContinuousScrollTick(...)    → accumulator + threshold → SendInput(WHEEL)
 └── ⚠ Problem: SendInput sends to FOCUSED window, not to window UNDER cursor

StateMachine (StateMachine.h)
 ├── Disabled → zone hidden, no events processed
 ├── Idle     → zone visible, scrolling active
 └── Edit     → zone visible, drag/resize handles shown

Config (Config.h + Config.cpp)
 ├── AppConfig { enabled, wheel_block, zone, scroll, sound, hotkeys }
 ├── ZoneConfig { x, y, width, height, opacity, color, cover_image, locked }
 └── ScrollConfig { mode, scroll_amount, continuous_speed, hover_speed }
```

---

## Proposed Changes

### Fix 1: Zone startup behavior

#### [MODIFY] [StateMachine.h](file:///e:/Game/ScrollNice/src/core/StateMachine.h)
- Change default `state_ = AppState::Idle` → `AppState::Disabled`
- Zone now stays hidden until user presses Ctrl+Alt+S or clicks Enable in tray

#### [MODIFY] [WinTray.cpp](file:///e:/Game/ScrollNice/src/platform/win/WinTray.cpp)
- After `Shell_NotifyIconW(NIM_ADD)`, show a balloon tip notification: _"ScrollNice is running. Press Ctrl+Alt+S to show the scroll zone."_

---

### Fix 2: Scroll target detection (CRITICAL)

#### [MODIFY] [ScrollEngine.h](file:///e:/Game/ScrollNice/src/core/ScrollEngine.h)
- Add `void SetTargetHwnd(HWND hwnd)` and `HWND targetHwnd_ = nullptr`
- When set, scroll events go to that specific window via `PostMessage`, not `SendInput`

#### [MODIFY] [ScrollEngine.cpp](file:///e:/Game/ScrollNice/src/core/ScrollEngine.cpp)
- [SendWheelEvent()](file:///e:/Game/ScrollNice/src/core/ScrollEngine.cpp#7-21): if `targetHwnd_` is valid, use `PostMessage(targetHwnd_, WM_MOUSEWHEEL, ...)`; otherwise fallback to `SendInput()`

#### [MODIFY] [WinOverlay.cpp](file:///e:/Game/ScrollNice/src/platform/win/WinOverlay.cpp)
- Track last cursor position outside zone (via [WinMouseHook](file:///e:/Game/ScrollNice/src/platform/win/WinMouseHook.h#20-21) or zone MOUSELEAVE)
- On `WM_MOUSELEAVE`, record cursor pos → `lastOutsidePos_`

#### [MODIFY] [main.cpp](file:///e:/Game/ScrollNice/src/main.cpp)
- In [OnZoneEvent](file:///e:/Game/ScrollNice/src/main.cpp#119-144), before scroll: `POINT cursor; GetCursorPos(&cursor);`
- Hide zone temporarily? No — use `WindowFromPoint(lastOutsidePos)` OR walk window hierarchy with `ChildWindowFromPointEx` excluding zone hwnd
- Set `g_scrollEngine.SetTargetHwnd(targetHwnd)`

**Algorithm for finding scroll target:**
```cpp
// Get the window under cursor, skipping the zone itself
HWND FindScrollTarget(HWND zoneHwnd) {
    POINT pt;
    GetCursorPos(&pt);
    HWND w = WindowFromPoint(pt);
    // Walk up if it's the zone or no scrollbar
    if (w == zoneHwnd || w == nullptr) w = g_lastWindowOutsideZone;
    // Drill down to find child with scrollbar
    HWND child = ChildWindowFromPointEx(w, ptClient, CWP_SKIPTRANSPARENT | CWP_SKIPINVISIBLE);
    return child ? child : w;
}
```

---

### Fix 3: GUI improvements

#### [MODIFY] [WinSettings.cpp](file:///e:/Game/ScrollNice/src/platform/win/WinSettings.cpp)
- Add `continuous_speed` and `hover_speed` spinners
- Fix DPI scaling: use `GetDpiForWindow()` for font size calculation
- Add `%` suffix to scroll amount label

#### [MODIFY] [WinTray.cpp](file:///e:/Game/ScrollNice/src/platform/win/WinTray.cpp)
- Add balloon tip on first launch
- Show mode name in tooltip (e.g. `"ScrollNice — Mode 1 (Click/Hold)"`)

---

## Verification Plan

### Build
```powershell
$cmake --build "e:\Game\ScrollNice\build" --config Release
```

### Manual Testing
1. Launch app → zone should NOT appear, tray icon shows
2. Press `Ctrl+Alt+S` → zone appears
3. Open a browser, overlay the zone on a webpage, click → page scrolls (not zone)
4. Mode 3: hover mouse into zone top → page scrolls up automatically
5. Settings → `Esc` closes dialog, opacity label updates live
