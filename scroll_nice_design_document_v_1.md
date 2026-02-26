# ScrollNice — Design Document (v1)

> **Mục tiêu:** tiện ích siêu nhẹ cho Windows giúp cuộn trang/khung bằng **Scroll Zone** (vùng neo cuộn) khi con trỏ đi vào, giải quyết trường hợp chuột hỏng bánh lăn hoặc thao tác một tay. Bản MVP **Windows native** (portable + installer), sau đó mới tính port.

---

## 0) Executive Summary

ScrollNice là một overlay click-through luôn nổi (always-on-top) tạo một hoặc nhiều vùng “zone”. Khi con trỏ ở trong zone, ứng dụng chuyển **chuyển động dọc của con trỏ** thành các **wheel events** (SendInput). Thiết kế ưu tiên:

- **Idle ≈ 0 CPU**, RAM tối thiểu, không poll.
- **Tách core vs UI**, mọi visual/plugin **tắt mặc định**.
- **Không telemetry**, không remote code.

Deliverable MVP: 1 file `ScrollNice.exe` portable + `config.json` cạnh exe.

---

## 1) Problem Statement — Phân tích vấn đề (có phản biện giả định)

### 1.1 Nỗi đau (pain)
- Chuột hỏng bánh lăn: cuộn là thao tác nền tảng trong web, IDE, file explorer.
- Một tay (tay còn lại bận): muốn cuộn nhanh mà không cần wheel.
- Trackpad/gesture không sẵn trên PC hoặc người dùng không quen.

### 1.2 Giả định cần kiểm tra (và rủi ro nếu sai)
1) **SendInput wheel** đủ để cuộn “đa số app”.
   - Rủi ro: app dùng raw input, custom scroll, game/engine bỏ qua wheel.
   - Giảm rủi ro: fallback gửi `WM_MOUSEWHEEL` cho cửa sổ dưới con trỏ (phải xác định window handle) hoặc chỉ cam kết “most Win32/UWP browsers/Office/Explorer”.

2) **WH_MOUSE_LL** hook không làm tăng CPU khi idle.
   - Rủi ro: hook vẫn nhận mọi mousemove → tần suất cao.
   - Giải pháp: state machine + fast path cực rẻ; không làm toán nặng khi ngoài zone; throttle ở active mode.

3) Overlay click-through không phá tương tác.
   - Rủi ro: hit-test sai → chặn click/hover.
   - Giải pháp: mặc định **WS_EX_TRANSPARENT**; chỉ bật hit-test nếu cần drag/resize zone khi vào “edit mode”.

4) “Zone-based scroll” không gây khó chịu/jitter.
   - Rủi ro: micro-movement tạo scroll không mong muốn.
   - Giải pháp: dead-zone, hysteresis, smoothing, activation delay optional.

### 1.3 Chỉ số thành công (Success metrics)
- Người dùng có thể cuộn mượt trong Chrome/Edge/Firefox, VS Code/IDE, Explorer.
- **Idle:** CPU ≈ 0% (Task Manager), RAM < 8MB.
- **Active:** CPU < 2% trên CPU phổ thông; phản hồi < 16ms cảm nhận.

---

## 2) Goals / Non-goals

### 2.1 Goals (Phase 1 — Windows MVP)
- 1–2 zone: **edge left/right**.
- Scroll dựa trên movement dọc (delta) trong zone.
- Điều chỉnh: sensitivity, dead_zone_px, max_rate, smoothing.
- Hotkey: enable/disable (global).
- Tray icon tối thiểu (optional trong MVP+, nhưng nên có sớm để kiểm soát).
- Config JSON, portable friendly.

### 2.2 Non-goals (để tránh phình scope)
- Plugin/theme system.
- Marketplace/mod.
- Port Linux/macOS.
- UI config phức tạp (GUI) — có thể ở Phase 2.
- “Scroll everywhere including games” — không cam kết.

---

## 3) User Personas & Primary Use-cases

### Persona A — “Chuột wheel chết”
- Cần giải pháp nhanh, ít cài đặt, chạy portable.
- Ưu tiên: ổn định, không ngốn tài nguyên.

### Persona B — “One-hand workflow”
- Dùng zone ở cạnh phải để cuộn trong khi tay trái làm việc khác.
- Ưu tiên: preset, cảm giác cuộn tự nhiên.

### Use-cases
- UC1: mở exe → zone hiện ở cạnh phải → đưa chuột vào zone → cuộn.
- UC2: nhấn hotkey tắt/bật nhanh khi chơi game/thiết kế.
- UC3: bật edit mode → kéo/đổi kích thước zone → lưu config.

---

## 4) UX Spec

### 4.1 Trạng thái hiển thị
- **Normal mode:** zone có thể ẩn (invisible) hoặc hiển thị rất mờ (alpha thấp) tùy cấu hình.
- **Hover/Active:** khi con trỏ vào zone, hiển thị indicator (mỏng) để người dùng biết đang ở “scroll mode”.
- **Edit mode:** zone hiển thị rõ, có handle resize/drag; overlay nhận click.

### 4.2 Interactions
- Enter zone → kích hoạt scroll engine.
- Move up/down trong zone → scroll.
- Leave zone → dừng.
- Hotkey toggle enable/disable toàn app.
- Optional: hotkey toggle edit mode.

### 4.3 Accessibility / Safety UX
- Có “panic key” (ví dụ Ctrl+Alt+S) để tắt ngay.
- Có giới hạn max scroll rate để tránh “runaway scroll”.

---

## 5) Technical Approach (Windows)

### 5.1 Core choices
- Language: **C++17/20**.
- Build: **CMake**.
- Rendering: Win32 + (GDI hoặc Direct2D). MVP có thể không cần render phức tạp.
- Input: `SetWindowsHookEx(WH_MOUSE_LL)` và `SendInput(MOUSEEVENTF_WHEEL)`.

### 5.2 Why WH_MOUSE_LL
- Không cần inject DLL vào process.
- Hoạt động global.
- Dễ deploy portable.

### 5.3 Wheel injection strategy
- Primary: `SendInput` wheel.
- Fallback (Phase 2 nếu cần): xác định window dưới cursor (`WindowFromPoint`) và gửi `WM_MOUSEWHEEL` (cẩn thận với UWP/Edge mới).

---

## 6) Architecture

### 6.1 Modules
- **Core**
  - `ZoneManager`: load config, quản lý zones, hit test.
  - `ScrollEngine`: map pointer delta → wheel events với smoothing/throttle.
  - `StateMachine`: idle/hover/active/edit.
  - `ConfigStore`: JSON load/save.
- **Platform (Windows)**
  - `WinMouseHook`: WH_MOUSE_LL, dispatch events.
  - `WinOverlayWindow`: layered window, click-through toggling.
  - `WinInputInjector`: SendInput wrapper + safety throttle.
  - `WinTray`: tray icon + menu tối thiểu.
- **UI (minimal)**
  - draw zone indicator + edit handles.

### 6.2 Data Flow
1) Mouse hook nhận event (move).
2) ZoneManager hit-test cursor point.
3) StateMachine chuyển trạng thái.
4) Khi Active: ScrollEngine tính v(t) và WinInputInjector phát wheel events.

### 6.3 Threading model
- **Single-thread** trên UI thread là đủ cho MVP.
- Hook callback phải cực nhẹ; đẩy event vào queue ring buffer nếu cần.
- Timer (high-resolution) chỉ bật khi Active để phát wheel theo tick; khi idle thì off.

---

## 7) State Machine Spec

### States
- `Disabled` — không xử lý.
- `Idle` — enabled nhưng cursor ngoài zone.
- `Hover` — cursor trong zone nhưng chưa scroll (chờ hysteresis/activation delay).
- `Active` — đang scroll.
- `Edit` — cho phép drag/resize.

### Transitions
- Disabled ↔ Idle: hotkey.
- Idle → Hover: enter zone.
- Hover → Active: movement vượt dead-zone/hysteresis hoặc sau delay.
- Active → Idle: leave zone hoặc timeout.
- Any → Edit: hotkey/menu.

### Guards
- Nếu user đang kéo scrollbar (mouse button down) thì có thể tạm disable scroll zone (optional).

---

## 8) Scroll Engine — Spec chi tiết (thực thi được)

### 8.1 Input signal
Có 2 cách chính:

- **Delta-based** (khuyến nghị): dùng `dy = y_now - y_prev` khi cursor trong zone.
  - Ưu: trực quan, ít jitter khi đứng yên.
- **Center-based**: `dy = y_now - zone_center_y`.
  - Ưu: giống “joystick”; nhược: đứng yên vẫn có dy nếu lệch tâm.

MVP nên dùng **delta-based**.

### 8.2 Filter / smoothing
Dùng exponential smoothing:

- `v_raw = dy * sensitivity`
- `v = lerp(v_prev, v_raw, alpha)` với `alpha = 1 - smoothing` (smoothing ∈ [0..0.95])

Thêm dead-zone:
- nếu `|dy| < dead_zone_px` thì dy = 0.

### 8.3 Acceleration (optional)
Nonlinear scale:
- `v_acc = sign(v) * |v|^p` (p khoảng 1.2–1.6) hoặc logistic clamp.

### 8.4 Map to wheel events
Windows wheel mặc định `WHEEL_DELTA = 120`.

- Tích lũy: `accum += v_acc * dt`
- Khi `accum >= step` → phát wheel `+120`, `accum -= step`.
- Khi `accum <= -step` → phát wheel `-120`.

Trong đó:
- `dt` = thời gian giữa tick active (ví dụ 8–16ms).
- `step` điều chỉnh “mật độ” wheel.

### 8.5 Throttle / max rate
- `max_events_per_sec` (ví dụ 120).
- Nếu vượt thì drop hoặc scale down v.

### 8.6 Anti-runaway
- Nếu Active nhưng cursor “teleport” (dy quá lớn do DPI glitch), clamp |dy| <= `max_dy_px`.
- Nếu phát events liên tục > N giây ở max rate, tự giảm tốc (soft limiter).

---

## 9) Zone System

### 9.1 Zone types
- `edge`: bám cạnh trái/phải/top/bottom.
- `floating`: rect cố định ở tọa độ.

### 9.2 Multi-monitor
- Zone gắn theo monitor (primary hoặc index).
- Config chứa `monitor_id`.
- Sử dụng `EnumDisplayMonitors` + work area.

### 9.3 Hit testing
- O(1) check rect.
- “Edge zone” nên dựa trên work area để không đè taskbar nếu user muốn.

---

## 10) Overlay Window Spec

### 10.1 Window styles
- `WS_EX_LAYERED` để alpha.
- `WS_EX_TRANSPARENT` để click-through.
- `WS_EX_TOPMOST` luôn nổi.
- `WS_EX_TOOLWINDOW` tránh xuất hiện Alt-Tab.

### 10.2 Input behavior
- Normal: click-through.
- Edit: tắt transparent để nhận click.

### 10.3 Rendering budget
- Khi Idle: không cần redraw liên tục.
- Khi Active/Hover/Edit: redraw theo event (không loop).

---

## 11) Config & Persistence

### 11.1 Config format (JSON)

```json
{
  "version": 1,
  "enabled": true,
  "hotkeys": {
    "toggle_enabled": "Ctrl+Alt+S",
    "toggle_edit": "Ctrl+Alt+E"
  },
  "engine": {
    "sensitivity": 1.2,
    "dead_zone_px": 2,
    "smoothing": 0.85,
    "accel_power": 1.35,
    "tick_ms": 10,
    "max_events_per_sec": 120
  },
  "zones": [
    {"id":"right","type":"edge","edge":"right","width":10,"height_pct":100,"monitor":"primary"},
    {"id":"left","type":"edge","edge":"left","width":6,"height_pct":100,"monitor":"primary"}
  ],
  "ui": {
    "visible": false,
    "hover_alpha": 0.15,
    "active_alpha": 0.25
  }
}
```

### 11.2 Storage rules
- Portable default: `./config.json`.
- Nếu cài đặt: `%APPDATA%/ScrollNice/config.json`.
- Không registry trừ khi user bật autostart (Task Scheduler/Run key) — ghi rõ.

---

## 12) Performance Budget & Instrumentation

### 12.1 Budgets
- Idle: CPU ~ 0%, RAM < 8MB.
- Active: CPU < 2%, RAM < 20MB.

### 12.2 Profiling plan
- Windows Task Manager + Performance Monitor.
- ETW/WPA (Phase 2) để kiểm tra hook overhead.

### 12.3 Hot paths
- Hook callback: chỉ làm hit-test + state update đơn giản.
- Scroll tick: math nhẹ, integer wheel emission.

---

## 13) Security & Privacy

- **No telemetry**.
- Không tải code từ xa.
- Plugin system (nếu có) phải sandbox + signature hoặc tắt mặc định.
- Hook cần giải thích rõ ràng trong README/installer.

Threat model cơ bản:
- App bị giả mạo để keylog: giảm rủi ro bằng release signing + checksums.

---

## 14) Testing Strategy

### 14.1 Unit tests
- `ScrollEngine`: dead zone, smoothing, throttle, accumulator.
- `Zone hit test`: edge/floating, multi-monitor.

### 14.2 Integration tests
- Simulate mouse movement vào zone → verify số wheel events phát ra (mock injector).

### 14.3 Manual test matrix
- Windows 10/11.
- Chrome/Edge/Firefox.
- Explorer, VS Code.
- Multi-monitor, DPI scaling 125%/150%.

---

## 15) Packaging & Distribution

- Portable zip: `ScrollNice.exe` + `config.json`.
- Installer (Phase 2): Inno Setup/NSIS.
- Code signing (nếu có điều kiện) để giảm SmartScreen friction.

---

## 16) Roadmap (ưu tiên theo rủi ro)

### Phase 0 — UX Prototype (Chrome)
- Mục tiêu: validate hành vi zone + cảm giác scroll.

### Phase 1 — Windows MVP (core risk)
- Hook + zone + scroll engine + config.
- Hotkey toggle.
- Minimal tray.

### Phase 2 — Polish
- Multi-zone, edit mode UI.
- Presets.
- Better fallback WM_MOUSEWHEEL nếu cần.

### Phase 3 — Plugin/theme (opt-in)
- Chỉ làm khi core ổn định và có demand.

---

## 17) Open Questions (phải quyết nhanh)

1) MVP có cần tray icon ngay không?
   - Không có tray: khó tắt nhanh → rủi ro UX cao. Khuyến nghị: có tray tối thiểu.

2) Default zone visible hay invisible?
   - Invisible giảm phiền; nhưng discoverability kém. Khuyến nghị: hover indicator mờ.

3) Delta-based vs center-based?
   - Khuyến nghị delta-based cho MVP.

---

## 18) Concrete Next Steps (7 ngày)

1) **Spike kỹ thuật (ngày 1–2):**
   - WH_MOUSE_LL + SendInput wheel demo.
   - Overlay click-through demo.

2) **Implement core (ngày 3–5):**
   - ZoneManager + ScrollEngine + config load/save.
   - Hotkey toggle.

3) **Stabilize (ngày 6–7):**
   - Throttle, dead-zone, perf check.
   - Zip portable, README publish.

---

### Appendix A — Repo skeleton đề xuất

```
ScrollNice/
  CMakeLists.txt
  src/
    core/
    platform/win/
    ui/
  tests/
  docs/
  tools/
```

### Appendix B — Coding standards
- clang-format.
- Keep allocation off hot path.
- No heavy dependencies.



---

## 19) Feature Set (MVP vs Extended)

### 19.1 MVP (phải có)
- Zones: `edge` left/right (top/bottom là tuỳ chọn nhưng nên có sớm).
- Toggle toàn app + **toggle zones** (enable/disable từng zone) bằng hotkey.
- **Disable physical mouse wheel** (tùy chọn): chặn `WM_MOUSEWHEEL`/low-level wheel event.
- Deep tuning: sensitivity, dead zone, smoothing, tick_ms, max rate.
- Edit mode: resize/drag zone + **lock position**.
- Tray icon: bật/tắt, edit mode, open config, quit.

### 19.2 Extended (opt-in, mặc định OFF)
- Zone modes: motion / click-step / hold-to-autoscroll / wheel-boost.
- Visual: ảnh bìa zone, glow/particle nhẹ, ripple khi click.
- Audio: click sound (PlaySound) khi click-step.
- Profiles/presets: One-hand, Reading, Coding.

---

## 20) Input & Zone Modes (nhiều cách dùng zone)

Mỗi zone có **mode** và **binding** riêng.

### 20.1 Motion mode (mặc định)
- Di chuyển dọc trong zone → scroll (delta-based).

### 20.2 Click-step mode
- Click trong zone → scroll theo bước cố định.
- Mapping mặc định:
  - Left click: scroll down
  - Right click: scroll up
  - Middle click: toggle enable zone (optional)

Tham số:
- `click_step_lines` hoặc `click_step_px`
- `click_repeat_ms` (hold để lặp)
- `click_smoothing_ms` (ramp up/down để “mượt”)

### 20.3 Hold-to-autoscroll mode
- Giữ nút chuột trong zone → auto scroll theo tốc độ tăng dần.

### 20.4 Top/Bottom zones
- Zone top: ưu tiên scroll up
- Zone bottom: ưu tiên scroll down
- Có thể kết hợp với motion/click.

### 20.5 Wheel-boost mode
- Khi cursor trong zone: wheel vật lý vẫn hoạt động nhưng được **amplify/smooth**.
- Có thể dùng để “tăng lực cuộn” cho wheel yếu.

---

## 21) Disable Physical Mouse Wheel (chặn wheel)

### 21.1 Options
- `wheel_block_mode`:
  - `off`
  - `global` (chặn mọi wheel trong hệ thống)
  - `outside_zone_only` (chỉ cho phép wheel khi ở trong zone)
  - `inside_zone_only` (ngược lại, hiếm dùng)

### 21.2 Implementation (Windows)
- Trong `WH_MOUSE_LL` hook:
  - Nếu event type là `WM_MOUSEWHEEL` hoặc `WM_MOUSEHWHEEL` và rule match → **return 1** để chặn.

### 21.3 Safety
- Có `bypass_modifier` (ví dụ giữ Alt thì không chặn wheel).
- Có panic key tắt toàn app ngay.

---

## 22) Hotkeys

- Dùng `RegisterHotKey` (không cần hook keyboard).
- Hotkeys đề xuất:
  - Toggle app: `Ctrl+Alt+S`
  - Toggle edit: `Ctrl+Alt+E`
  - Toggle zone next: `Ctrl+Alt+Z`
  - Toggle wheel block: `Ctrl+Alt+W`

---

## 23) Zone Editing, Lock, Resize

- Edit mode bật overlay nhận click (tắt WS_EX_TRANSPARENT).
- Mỗi zone có:
  - `locked`: không cho drag/resize
  - `snap`: snap theo pixel grid (ví dụ 2px/4px)
  - min/max size

---

## 24) Visuals / Audio (opt-in)

### 24.1 Theme assets
- `themes/<name>/zone.png` (9-slice optional) + `theme.json`.
- Mặc định tắt để giữ idle memory thấp.

### 24.2 Effects
- Chỉ effect nhẹ, event-driven (không loop).
- Ví dụ: ripple 150–300ms khi click.

### 24.3 Sounds
- Dùng `PlaySound` (async) hoặc wav nhẹ.
- Có limiter để không spam âm thanh.

---

## 25) “Không vi phạm game” / Game Safety Policy

Mục tiêu là **không can thiệp** và **tự tránh** bối cảnh nhạy cảm:
- Không inject DLL.
- Không đọc memory process khác.
- Không giả lập input theo pattern gian lận.

Auto-suspend rules (khuyến nghị bật mặc định):
- Nếu foreground app là fullscreen exclusive (hoặc cửa sổ top-level thuộc nhóm game) → tạm disable.
- Nếu phát hiện process thuộc danh sách user-defined “Game Exclusions” → disable.
- Nếu process chạy elevated/anti-cheat → chỉ cho phép user bật thủ công.

---

## 26) Extension Prototype (Chrome) — Spec đầy đủ

### 26.1 Purpose
- Chỉ để test UX: vị trí zone, mode motion/click-step, cảm giác smoothing.

### 26.2 Architecture (MV3)
- `content.js`: inject zone overlay vào trang, xử lý pointer events, gọi `window.scrollBy`.
- `popup.html/js`: toggle enable + presets.
- `service_worker.js`: lưu settings vào `chrome.storage` + handle commands.
- `commands`: hotkeys extension (toggle zone / toggle enabled).

### 26.3 Features in extension
- Zones: left/right/top/bottom.
- Mode: motion + click-step.
- Optional: block wheel on page (`addEventListener('wheel', e => e.preventDefault(), {passive:false})`).

### 26.4 Non-goals
- Không mô phỏng Windows global scroll. Chỉ trong web page.

---

## 27) Packaging: Portable + Installer

### 27.1 Portable
- `ScrollNice.exe` + `config.json` + (optional) `themes/`.
- Không ghi registry.

### 27.2 Installer
- Tool: Inno Setup/NSIS.
- Options:
  - Add to Startup (Task Scheduler/Run key)
  - Store config in `%APPDATA%/ScrollNice/`
  - Create shortcut + uninstall

### 27.3 Updates
- MVP: manual download.
- Phase 2: optional updater (nhưng cân nhắc footprint).

---

## 28) Website + Docs (thông tin ứng dụng)

### 28.1 Tech gợi ý
- Static site: **Astro Starlight** hoặc **Docusaurus** (docs tốt).
- Hosting: GitHub Pages.

### 28.2 Sitemap
- Home (value prop + demo GIF)
- Download (portable/installer + checksum)
- Docs
  - Install (portable vs installer)
  - Quick Start (zones, hotkeys)
  - Settings (engine, wheel block, zones)
  - Presets
  - Troubleshooting
  - Privacy
- Changelog
- Contributing

---

## 29) Config Schema (bổ sung)

Thêm vào JSON:
- `wheel_block_mode`, `bypass_modifier`
- `zones[].mode`, `zones[].locked`, `zones[].click_step_*`
- `exclusions` (process list)

