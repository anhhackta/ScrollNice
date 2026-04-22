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
| 🚫 **Chặn Wheel** | Bật/tắt chặn wheel vật lý (giữ `Alt` để bỏ qua) |
| 🌐 **Chrome Extension** | Phiên bản trình duyệt nhẹ, load unpacked |
| 🔄 **Portable** | Không cài đặt, chạy trực tiếp `.exe` |

---

## 🎮 3 Chế độ lăn / Scroll Modes


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
