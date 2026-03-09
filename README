# 🖱️ ScrollNice

> **Lăn trang bằng Click — không cần Wheel chuột** | Scroll by Clicking — no mouse wheel needed

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%2010%2F11-0078d4?logo=windows)](https://github.com/anhhackta/ScrollNice/releases)
[![Chrome Extension](https://img.shields.io/badge/Chrome-Extension-4285F4?logo=googlechrome&logoColor=white)](https://github.com/anhhackta/ScrollNice)
[![Size: <500KB](https://img.shields.io/badge/Size-%3C500KB-green)](https://github.com/anhhackta/ScrollNice/releases)
[![Free & Open Source](https://img.shields.io/badge/Free-%26%20Open%20Source-orange)](https://github.com/anhhackta/ScrollNice)

**ScrollNice** là tiện ích Windows và Chrome Extension cho phép bạn cuộn trang bằng cách **click, giữ chuột, hoặc chỉ di chuột** vào một vùng nổi (Scroll Zone) — không cần bánh lăn chuột.

🌐 **Website:** [anhhackta.github.io/ScrollNice](https://anhhackta.github.io/ScrollNice/)

---

## ✨ Tính năng nổi bật / Key Features

| Tính năng | Mô tả |
|-----------|-------|
| 🖱️ **3 chế độ lăn** | Click/Giữ · Chia vùng Trên/Dưới · Di chuột Tự động |
| 🪶 **Siêu nhẹ** | < 500KB, CPU ≈ 0% khi idle, RAM < 10MB |
| 🔒 **Không telemetry** | Không gửi dữ liệu, không quảng cáo, không internet |
| ⌨️ **Phím tắt** | Ctrl+Alt+S bật/tắt, Ctrl+Alt+E chỉnh sửa, tùy chỉnh được |
| 🎨 **Tùy chỉnh** | Resize/di chuyển zone, ảnh bìa, âm thanh click |
| 🚫 **Chặn Wheel** | Tùy chọn chặn sự kiện wheel vật lý |
| 🌐 **Chrome Extension** | Phiên bản trình duyệt nhẹ, load unpacked |
| 🔄 **Portable** | Không cài đặt, chạy trực tiếp `.exe` |

---

## 🎮 3 Chế độ lăn / Scroll Modes

### Mode 1 — Click / Giữ
- **Click chuột trái** → lăn lên ↑
- **Click chuột phải** → lăn xuống ↓
- **Giữ chuột** → lăn liên tục, tốc độ tăng dần

### Mode 2 — Chia vùng Trên / Dưới
- Zone chia làm 2 nửa (trên / dưới)
- **Click/Giữ nửa trên** → lăn lên ↑
- **Click/Giữ nửa dưới** → lăn xuống ↓

### Mode 3 — Di chuột Tự động *(đặc biệt)*
- **Không cần click!**
- Di chuột vào nửa trên → tự lăn lên ↑
- Di chuột vào nửa dưới → tự lăn xuống ↓

---

## 📥 Cài đặt / Installation

### Windows App (Portable)
1. Tải [`ScrollNice-portable.zip`](https://github.com/anhhackta/ScrollNice/releases) từ Releases
2. Giải nén và chạy `ScrollNice.exe`
3. Zone sẽ xuất hiện — đưa chuột vào để bắt đầu lăn

> Config lưu tại `./config.json` bên cạnh file `.exe`

### Chrome Extension
1. Tải source hoặc clone repo
2. Mở Chrome → `chrome://extensions/`
3. Bật **Developer mode** → **Load unpacked**
4. Chọn thư mục `extension/FeelClick/`

### Build từ source (C++17 / CMake)
```bash
git clone https://github.com/anhhackta/ScrollNice.git
cd ScrollNice
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Yêu cầu: MSVC 2019+ hoặc MinGW, CMake ≥ 3.20

---

## ⌨️ Phím tắt mặc định / Default Hotkeys

| Phím tắt | Chức năng |
|----------|-----------|
| `Ctrl+Alt+S` | Bật / Tắt ScrollNice |
| `Ctrl+Alt+E` | Vào / Ra chế độ chỉnh sửa (Edit Mode) |
| `Ctrl+Alt+Z` | Bật / Tắt zone tiếp theo |
| `Ctrl+Alt+W` | Bật / Tắt chặn wheel vật lý |

> Tất cả phím tắt có thể thay đổi trong `config.json`

---

## ⚙️ Cấu hình / Configuration (`config.json`)

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

### Tùy chọn `wheel_block_mode`

| Giá trị | Ý nghĩa |
|---------|---------|
| `off` | Không chặn wheel (mặc định) |
| `global` | Chặn wheel toàn hệ thống |
| `outside_zone_only` | Chỉ cho phép wheel khi ở trong zone |
| `inside_zone_only` | Ngược lại |

---

## 📁 Cấu trúc dự án / Project Structure

```
ScrollNice/
├── src/
│   ├── core/           # ZoneManager, ScrollEngine, StateMachine, Config
│   ├── platform/win/   # WinMouseHook, WinOverlay, WinInputInjector, WinTray
│   └── ui/             # Zone rendering
├── extension/
│   └── FeelClick/      # Chrome Extension (MV3)
├── docs/               # Website (GitHub Pages)
│   ├── css/style.css
│   ├── js/             # main.js, i18n.js, docs-i18n.js
│   ├── docs/           # Documentation pages
│   └── assets/
├── tests/              # Unit tests (ScrollEngine, HitTest)
├── CMakeLists.txt
└── config.json         # Default config
```

---

## 🔧 Kiến trúc / Architecture

```
Mouse Move Event (WH_MOUSE_LL hook)
         │
         ▼
  ZoneManager.HitTest(cursor)
         │
         ▼
  StateMachine (Idle → Hover → Active → Edit)
         │ (Active only)
         ▼
  ScrollEngine
  ┌────────────────────────────────┐
  │  dy = cursor.y - prev.y       │
  │  if |dy| < dead_zone → skip   │
  │  v = lerp(v_prev, dy×sens, α) │
  │  accum += v × dt              │
  │  while accum >= step:          │
  │    SendInput(WHEEL +120)       │
  └────────────────────────────────┘
         │
         ▼
  WinInputInjector (throttle: max 120/sec)
```

---

## 🛡️ Bảo mật & Quyền riêng tư / Security & Privacy

- ❌ Không gửi dữ liệu ra ngoài
- ❌ Không đọc nội dung màn hình hay clipboard
- ✅ Không inject DLL vào process khác
- ✅ Chỉ dùng `WH_MOUSE_LL` (global mouse hook đã được Windows cho phép)
- ✅ Tự suspend khi phát hiện fullscreen / game process

> **Lưu ý:** Phần mềm sử dụng `WH_MOUSE_LL` hook để đọc vị trí chuột toàn hệ thống — đây là cơ chế tiêu chuẩn của Windows, cần thiết cho chức năng scroll zone. Không có dữ liệu nào được lưu hay gửi đi.

---

## 🤝 Ủng hộ / Support the Project

ScrollNice hoàn toàn **miễn phí và mã nguồn mở**. Nếu tiện ích hữu ích, bạn có thể ủng hộ:

- ☕ **Ko-fi:** [ko-fi.com/hoang2k2](https://ko-fi.com/hoang2k2)
- 💳 **PayPal:** [paypal.me/bahoang2k2](https://paypal.me/bahoang2k2)

---

## 📋 Changelog

Xem [CHANGELOG](docs/changelog.html) hoặc [website](https://anhhackta.github.io/ScrollNice/changelog.html).

---

## 📄 License

MIT License — xem [LICENSE](LICENSE)

---

## 👨‍💻 Tác giả / Author

**HoàngX** ([@anhhackta](https://github.com/anhhackta))  
Website: [anhhackta.github.io/ScrollNice](https://anhhackta.github.io/ScrollNice/)

---

*Made with ❤️ in Vietnam 🇻🇳*
