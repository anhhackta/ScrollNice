<div align="center">

# 🖱️ ScrollNice

> **Lăn trang bằng click — không cần bánh lăn chuột**

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

**ScrollNice** là tiện ích **Windows** và **Chrome Extension** (FeelClick): vùng cuộn nổi — **click**, **giữ**, hoặc **chỉ di chuột** để lăn trang. Phù hợp khi wheel hỏng, dùng một tay, hoặc muốn cách cuộn khác.

🌐 **Website:** [anhhackta.github.io/ScrollNice](https://anhhackta.github.io/ScrollNice/) · Mã nguồn trang: thư mục `docs/`

</div>

---

## Tính năng chính

| | |
|--|--|
| **3 chế độ** | Click/giữ · chia trên/dưới · tự cuộn khi hover |
| **Gọn nhẹ** | Bản **native** phát hành là `.exe` portable rất nhỏ ([Releases](https://github.com/anhhackta/ScrollNice/releases)) |
| **Không telemetry** | Không thu thập dữ liệu, không quảng cáo |
| **Phím tắt** | Mặc định: `Ctrl+Alt+S`, `Ctrl+Alt+E`, `Ctrl+Alt+W` — chỉnh trong `config.json` |
| **Chặn wheel** | Tùy chọn; có thể giữ **`Alt`** để bỏ qua (theo cấu hình) |
| **Chrome** | Load unpacked từ `extension/FeelClick/` |

---

## Cài đặt

### Windows (portable, từ Releases)

1. Tải **`ScrollNice-portable.zip`** tại [Releases](https://github.com/anhhackta/ScrollNice/releases).
2. Giải nén và chạy **`ScrollNice.exe`**.
3. Cấu hình: file **`config.json`** cạnh file `.exe`.

> Bản build đang phân phối cho người dùng chủ yếu từ **C++ / Win32**. Nhánh **Rust + Slint** đang migration — xem [MIGRATION_STATUS_2026-05-08.md](MIGRATION_STATUS_2026-05-08.md).

### Chrome Extension (FeelClick)

1. Clone hoặc tải repo.
2. Chrome → `chrome://extensions/` → bật **Chế độ nhà phát triển** → **Tải tiện ích đã giải nén**.
3. Chọn thư mục **`extension/FeelClick/`**.

---

## Ba chế độ lăn

1. **Click / giữ** — Click trái lăn lên, click phải lăn xuống; giữ để cuộn liên tục có gia tốc.
2. **Chia trên / dưới** — Nửa trên / nửa dưới vùng zone; click hoặc giữ từng nửa để đổi hướng.
3. **Hover tự động** — Không cần click: hover nửa trên / nửa dưới để tự cuộn.

Chi tiết: [trang tài liệu — Chế độ](https://anhhackta.github.io/ScrollNice/docs/modes.html).

---

## Build từ mã nguồn

### Ứng dụng C++ (đường release hiện tại)

**Yêu cầu:** CMake ≥ 3.15, MSVC 2019+ (hoặc toolchain tương đương).

```powershell
git clone https://github.com/anhhackta/ScrollNice.git
cd ScrollNice
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Kết quả thường là: `build/Release/ScrollNice.exe` (có thể khác nhẹ tùy generator).

### Rust + Slint (bản thử / migration)

**Yêu cầu:** [Rust stable](https://rustup.rs/).

```powershell
# Build thử nhanh (compile nhanh hơn --release)
cargo build --profile quick
# .\target\quick\scrollnice-rs.exe

# Bản tối ưu
cargo test
cargo build --release
# .\target\release\scrollnice-rs.exe
```

Thiết kế & schema: **`docs/superpowers/specs/2026-05-08-slint-ui-migration-design.md`** · Tiến độ: **`MIGRATION_STATUS_2026-05-08.md`**.

### Nên chạy file `.exe` nào?

| Mục đích | Chạy file |
|----------|-----------|
| **Dùng bình thường** | Tải [Releases](https://github.com/anhhackta/ScrollNice/releases) → `ScrollNice.exe` trong zip (bản C++). |
| **Sau khi build CMake** | `build\Release\ScrollNice.exe` (đôi khi `build\x64\Release\`, tùy generator). |
| **Sau `cargo build`** | `target\debug\scrollnice-rs.exe` |
| **Sau `cargo build --profile quick`** | `target\quick\scrollnice-rs.exe` |
| **Sau `cargo build --release`** | `target\release\scrollnice-rs.exe` |

Chỉ cần **một** binary tương ứng cách bạn build. Thư mục **`target\`** (Rust) và **`build\`** (CMake) có thể xóa bất cứ lúc nào — build lại sẽ tạo lại (đã `.gitignore`).

---

## Cấu hình (`config.json`)

Baseline **Rust** dùng **schema phiên bản 2**. Ví dụ:

```json
{
  "version": 2,
  "enabled": true,
  "language": "vi",
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

Bản **C++** cũ có thể khác một số trường; nên sao lưu `config.json` khi đổi binary. Tham chiếu đầy đủ: [Cài đặt (website)](https://anhhackta.github.io/ScrollNice/docs/settings.html).

---

## Cấu trúc thư mục (chuẩn hóa)

```
ScrollNice/
├── Cargo.toml, build.rs          # Rust + Slint (đang migration)
├── CMakeLists.txt                # App Windows (C++17)
├── config.json                   # Mẫu / mặc định (v2 cho Rust)
├── ui/                           # Giao diện Slint
├── src/
│   ├── main.rs, lib.rs
│   ├── core/                     # Logic: Rust + song song file C++
│   ├── platform/win/             # Win32 (C++)
│   ├── platform/win32/           # Win32 (Rust, đang hoàn thiện)
│   ├── i18n/, themes/, ui/
│   └── main.cpp
├── extension/FeelClick/          # Tiện ích Chrome MV3
├── locales/, presets/
├── docs/                         # GitHub Pages (landing + hướng dẫn)
├── docs/superpowers/specs/       # Spec thiết kế đã duyệt
├── MIGRATION_STATUS_2026-05-08.md
├── ScrollNiceDoc                 # Ghi chép lịch sử phát triển (tùy chọn đọc)
└── vendor/                       # Header C++ (vd. nlohmann json)
```

Thư mục **`build/`** (CMake) và **`target/`** (Rust) không commit — tạo khi build cục bộ.

---

## Bảo mật & quyền riêng tư

- **Không telemetry** — không gửi dữ liệu ra ngoài.
- **Không** đọc nội dung màn hình hay clipboard; **không** inject DLL vào process khác.
- Dùng hook **`WH_MOUSE_LL`** chuẩn Windows cho sự kiện chuột cần cho zone. Phần mềm diệt virus đôi khi cảnh báo chung cho hook cấp thấp — mã nguồn mở để kiểm tra.

---

## Ủng hộ dự án

- ☕ [Ko-fi — hoang2k2](https://ko-fi.com/hoang2k2)
- 💳 [PayPal — bahoang2k2](https://paypal.me/bahoang2k2)

---

## Giấy phép

[MIT License](LICENSE)

---

## Tác giả

**HoàngX** · [@anhhackta](https://github.com/anhhackta) · [Website dự án](https://anhhackta.github.io/ScrollNice/)

*Made with care in Vietnam 🇻🇳*
