#include "WinMainWindow.h"
#include <commctrl.h>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

namespace sn {

static WinMainWindow* g_mainWnd = nullptr;

// ─── Modern dark theme colors ───
static const COLORREF CLR_BG         = RGB(24, 24, 32);      // Deep dark background
static const COLORREF CLR_SURFACE    = RGB(32, 32, 44);      // Slightly lighter surface
static const COLORREF CLR_CARD       = RGB(40, 40, 52);      // Card background
static const COLORREF CLR_TEXT       = RGB(240, 240, 250);    // Near-white text
static const COLORREF CLR_TEXT_DIM   = RGB(160, 160, 180);   // Dimmed text
static const COLORREF CLR_ACCENT     = RGB(99, 130, 246);    // Blue accent
static const COLORREF CLR_ACCENT_H   = RGB(120, 150, 255);   // Lighter blue
static const COLORREF CLR_SUCCESS    = RGB(80, 200, 120);    // Green
static const COLORREF CLR_BORDER     = RGB(60, 60, 80);      // Border color
static const COLORREF CLR_HOVER      = RGB(50, 50, 70);      // Hover state

// ─── Control IDs ───
enum {
    IDC_ZONE_ENABLE      = 101,
    IDC_MODE_COMBO       = 102,
    IDC_ZONE_X           = 110,
    IDC_ZONE_Y           = 111,
    IDC_ZONE_W           = 112,
    IDC_ZONE_H           = 113,
    IDC_ZONE_OPACITY     = 114,
    IDC_ZONE_OPACITY_LBL = 115,
    IDC_ZONE_LOCKED      = 116,
    IDC_SCROLL_AMOUNT    = 120,
    IDC_HOLD_SPEED       = 121,
    IDC_HOLD_ACCEL       = 122,
    IDC_HOVER_SPEED      = 123,
    IDC_WHEEL_BLOCK      = 130,
    IDC_START_WINDOWS    = 131,
    IDC_SOUND_ENABLED    = 132,
    IDC_HK_TOGGLE        = 140,
    IDC_HK_EDIT          = 141,
    IDC_HK_WHEEL         = 142,
    IDC_SAVE_BTN         = 200,
    IDC_RESET_BTN        = 201,
    IDC_STATUS_BAR       = 300,
    IDC_TIMER_STATUS     = 400,
};

static const wchar_t* kMainClass = L"ScrollNice_MainWindow";

// ─── Helper: create child control ───
static HWND Mk(HWND parent, const wchar_t* cls, const wchar_t* text,
               DWORD style, int x, int y, int w, int h, int id, HINSTANCE hInst) {
    return CreateWindowExW(0, cls, text,
        WS_CHILD | WS_VISIBLE | style,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, hInst, nullptr);
}

// ─── Create ───
bool WinMainWindow::Create(HINSTANCE hInst, AppConfig& cfg,
                            MainWindowSaveCallback onSave,
                            MainWindowEventCallback onEvent) {
    g_mainWnd = this;
    hInst_    = hInst;
    cfg_      = &cfg;
    onSave_   = onSave;
    onEvent_  = onEvent;

    hBrushBg_      = CreateSolidBrush(CLR_BG);
    hBrushSurface_ = CreateSolidBrush(CLR_SURFACE);
    hBrushCard_    = CreateSolidBrush(CLR_CARD);

    // Modern font with better readability
    hFont_ = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    hFontBold_ = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    hFontSmall_ = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = kMainClass;
    wc.hbrBackground  = hBrushBg_;
    wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon          = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    const int W = 520, H = 680;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - W) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - H) / 2;

    hwnd_ = CreateWindowExW(0, kMainClass, L"ScrollNice - Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        sx, sy, W, H, nullptr, nullptr, hInst, nullptr);
    if (!hwnd_) return false;

    InitControls();
    SyncFromConfig(cfg);
    return true;
}

void WinMainWindow::Destroy() {
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    if (hFont_)          { DeleteObject(hFont_);          hFont_ = nullptr; }
    if (hFontBold_)      { DeleteObject(hFontBold_);      hFontBold_ = nullptr; }
    if (hFontSmall_)     { DeleteObject(hFontSmall_);     hFontSmall_ = nullptr; }
    if (hBrushBg_)       { DeleteObject(hBrushBg_);       hBrushBg_ = nullptr; }
    if (hBrushSurface_)  { DeleteObject(hBrushSurface_);  hBrushSurface_ = nullptr; }
    if (hBrushCard_)     { DeleteObject(hBrushCard_);     hBrushCard_ = nullptr; }
    g_mainWnd = nullptr;
}

void WinMainWindow::Show()  { if (hwnd_) { ShowWindow(hwnd_, SW_SHOW); SetForegroundWindow(hwnd_); } }
void WinMainWindow::Hide()  { if (hwnd_) ShowWindow(hwnd_, SW_HIDE); }
bool WinMainWindow::IsVisible() const { return hwnd_ && IsWindowVisible(hwnd_); }

// ─── InitControls ───
void WinMainWindow::InitControls() {
    auto mk = [&](const wchar_t* cls, const wchar_t* text,
                   DWORD style, int x, int y, int w, int h, int id) {
        return Mk(hwnd_, cls, text, style, x, y, w, h, id, hInst_);
    };

    int y = 12;
    const int LX = 16;       // left margin
    const int PW = 488;      // panel width
    const int LBL_W = 100;   // label width
    const int EDIT_W = 70;   // edit field width

    // ═══ Zone GroupBox ═══
    HWND g1 = mk(L"BUTTON", L"  🖱️ Zone", BS_GROUPBOX, LX, y, PW, 180, 0);
    SendMessage(g1, WM_SETFONT, (WPARAM)hFontBold_, TRUE);
    y += 26;

    mk(L"BUTTON", L"Enable Zone", BS_AUTOCHECKBOX, LX+16, y, 140, 22, IDC_ZONE_ENABLE);
    mk(L"STATIC", L"Mode:", 0, LX+200, y, 50, 20, 0);
    HWND combo = mk(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL,
                     LX+255, y-2, 180, 120, IDC_MODE_COMBO);
    SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)L"Mode 1: Click/Hold");
    SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)L"Mode 2: Top/Bottom");
    SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)L"Mode 3: Hover Auto");
    y += 30;

    mk(L"STATIC", L"Position X:", 0, LX+16, y, LBL_W, 20, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, LX+120, y-2, EDIT_W, 24, IDC_ZONE_X);
    mk(L"STATIC", L"Y:", 0, LX+200, y, 20, 20, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, LX+225, y-2, EDIT_W, 24, IDC_ZONE_Y);
    y += 28;

    mk(L"STATIC", L"Size W:", 0, LX+16, y, LBL_W, 20, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, LX+120, y-2, EDIT_W, 24, IDC_ZONE_W);
    mk(L"STATIC", L"H:", 0, LX+200, y, 20, 20, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, LX+225, y-2, EDIT_W, 24, IDC_ZONE_H);
    y += 30;

    mk(L"STATIC", L"Opacity:", 0, LX+16, y, 70, 20, 0);
    HWND slider = mk(TRACKBAR_CLASSW, L"", TBS_HORZ | TBS_AUTOTICKS,
                     LX+90, y-3, 260, 28, IDC_ZONE_OPACITY);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(5, 100));
    SendMessage(slider, TBM_SETTICFREQ, 10, 0);
    mk(L"STATIC", L"25%", SS_CENTER, LX+360, y, 50, 22, IDC_ZONE_OPACITY_LBL);
    y += 30;

    mk(L"BUTTON", L"Lock Position", BS_AUTOCHECKBOX, LX+16, y, 150, 22, IDC_ZONE_LOCKED);
    y += 28 + 8;

    // ═══ Scroll GroupBox ═══
    HWND g2 = mk(L"BUTTON", L"  ⚙️ Scroll", BS_GROUPBOX, LX, y, PW, 120, 0);
    SendMessage(g2, WM_SETFONT, (WPARAM)hFontBold_, TRUE);
    y += 26;

    mk(L"STATIC", L"Amount:", 0, LX+16, y, 60, 20, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, LX+80, y-2, EDIT_W, 24, IDC_SCROLL_AMOUNT);
    mk(L"STATIC", L"px/click", 0, LX+160, y, 60, 20, 0);
    y += 28;

    mk(L"STATIC", L"Hold spd:", 0, LX+16, y, 70, 20, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, LX+90, y-2, 50, 24, IDC_HOLD_SPEED);
    mk(L"STATIC", L"Accel:", 0, LX+155, y, 45, 20, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, LX+205, y-2, 50, 24, IDC_HOLD_ACCEL);
    mk(L"STATIC", L"Hover:", 0, LX+270, y, 50, 20, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, LX+325, y-2, 50, 24, IDC_HOVER_SPEED);
    y += 28 + 8;

    // ═══ General GroupBox ═══
    HWND g3 = mk(L"BUTTON", L"  🎛️ General", BS_GROUPBOX, LX, y, PW, 100, 0);
    SendMessage(g3, WM_SETFONT, (WPARAM)hFontBold_, TRUE);
    y += 26;

    mk(L"BUTTON", L"Block Mouse Wheel (Ctrl+Alt+W)", BS_AUTOCHECKBOX, LX+16, y, 300, 22, IDC_WHEEL_BLOCK);
    y += 24;
    mk(L"BUTTON", L"Start with Windows", BS_AUTOCHECKBOX, LX+16, y, 200, 22, IDC_START_WINDOWS);
    y += 24;
    mk(L"BUTTON", L"Click Sound", BS_AUTOCHECKBOX, LX+16, y, 150, 22, IDC_SOUND_ENABLED);
    y += 28 + 8;

    // ═══ Hotkeys GroupBox ═══
    HWND g4 = mk(L"BUTTON", L"  ⌨️ Hotkeys", BS_GROUPBOX, LX, y, PW, 90, 0);
    SendMessage(g4, WM_SETFONT, (WPARAM)hFontBold_, TRUE);
    y += 26;

    mk(L"STATIC", L"Toggle:", 0, LX+16, y, 60, 20, 0);
    mk(L"EDIT", L"", WS_BORDER, LX+80, y-2, 110, 24, IDC_HK_TOGGLE);
    mk(L"STATIC", L"Edit:", 0, LX+210, y, 40, 20, 0);
    mk(L"EDIT", L"", WS_BORDER, LX+255, y-2, 110, 24, IDC_HK_EDIT);
    y += 28;
    mk(L"STATIC", L"Wheel:", 0, LX+16, y, 60, 20, 0);
    mk(L"EDIT", L"", WS_BORDER, LX+80, y-2, 110, 24, IDC_HK_WHEEL);
    y += 32 + 8;

    // ═══ Buttons ═══
    mk(L"BUTTON", L"💾 Save", BS_DEFPUSHBUTTON, LX+120, y, 120, 38, IDC_SAVE_BTN);
    mk(L"BUTTON", L"🔄 Reset", 0, LX+260, y, 120, 38, IDC_RESET_BTN);
    y += 48;

    // ═══ Status Bar ═══
    mk(L"STATIC", L"Zone: OFF | Mode 1 | Ready",
       SS_CENTER | SS_SUNKEN, 0, y, 520, 24, IDC_STATUS_BAR);

    // Apply font to all children
    EnumChildWindows(hwnd_, [](HWND child, LPARAM lp) -> BOOL {
        SendMessage(child, WM_SETFONT, (WPARAM)lp, TRUE);
        return TRUE;
    }, (LPARAM)hFont_);

    // Status update timer (every 2s)
    SetTimer(hwnd_, IDC_TIMER_STATUS, 2000, nullptr);
}

// ─── SyncFromConfig ───
void WinMainWindow::SyncFromConfig(const AppConfig& cfg) {
    if (!hwnd_) return;

    CheckDlgButton(hwnd_, IDC_ZONE_ENABLE,  cfg.enabled ? BST_CHECKED : BST_UNCHECKED);

    ScrollMode m = ScrollModeFromString(cfg.scroll.mode);
    int idx = 0;
    if (m == ScrollMode::SplitHold) idx = 1;
    if (m == ScrollMode::HoverAuto) idx = 2;
    SendDlgItemMessage(hwnd_, IDC_MODE_COMBO, CB_SETCURSEL, idx, 0);

    SetDlgItemInt(hwnd_, IDC_ZONE_X, cfg.zone.x, TRUE);
    SetDlgItemInt(hwnd_, IDC_ZONE_Y, cfg.zone.y, TRUE);
    SetDlgItemInt(hwnd_, IDC_ZONE_W, cfg.zone.width, FALSE);
    SetDlgItemInt(hwnd_, IDC_ZONE_H, cfg.zone.height, FALSE);

    int opPct = (int)(cfg.zone.opacity * 100);
    SendDlgItemMessage(hwnd_, IDC_ZONE_OPACITY, TBM_SETPOS, TRUE, opPct);
    wchar_t opBuf[16]; swprintf_s(opBuf, L"%d%%", opPct);
    SetDlgItemTextW(hwnd_, IDC_ZONE_OPACITY_LBL, opBuf);

    CheckDlgButton(hwnd_, IDC_ZONE_LOCKED,    cfg.zone.locked        ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd_, IDC_WHEEL_BLOCK,     cfg.wheel_block        ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd_, IDC_START_WINDOWS,   cfg.start_with_windows ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd_, IDC_SOUND_ENABLED,   cfg.sound.enabled      ? BST_CHECKED : BST_UNCHECKED);

    SetDlgItemInt(hwnd_, IDC_SCROLL_AMOUNT, cfg.scroll.scroll_amount, FALSE);
    SetDlgItemInt(hwnd_, IDC_HOLD_SPEED,    cfg.scroll.continuous_speed, FALSE);
    SetDlgItemInt(hwnd_, IDC_HOLD_ACCEL,    cfg.scroll.continuous_accel, FALSE);
    SetDlgItemInt(hwnd_, IDC_HOVER_SPEED,   cfg.scroll.hover_speed, FALSE);

    auto toW = [](const std::string& s) { return std::wstring(s.begin(), s.end()); };
    SetDlgItemTextW(hwnd_, IDC_HK_TOGGLE, toW(cfg.hotkeys.toggle_enabled).c_str());
    SetDlgItemTextW(hwnd_, IDC_HK_EDIT,   toW(cfg.hotkeys.toggle_edit).c_str());
    SetDlgItemTextW(hwnd_, IDC_HK_WHEEL,  toW(cfg.hotkeys.toggle_wheel).c_str());
}

// ─── ReadControls ───
void WinMainWindow::ReadControls(AppConfig& cfg) {
    if (!hwnd_) return;

    cfg.enabled = IsDlgButtonChecked(hwnd_, IDC_ZONE_ENABLE) == BST_CHECKED;

    int modeIdx = (int)SendDlgItemMessage(hwnd_, IDC_MODE_COMBO, CB_GETCURSEL, 0, 0);
    if (modeIdx == 0) cfg.scroll.mode = "click_hold";
    if (modeIdx == 1) cfg.scroll.mode = "split_hold";
    if (modeIdx == 2) cfg.scroll.mode = "hover_auto";

    cfg.zone.x      = (int)GetDlgItemInt(hwnd_, IDC_ZONE_X, nullptr, TRUE);
    cfg.zone.y      = (int)GetDlgItemInt(hwnd_, IDC_ZONE_Y, nullptr, TRUE);
    cfg.zone.width  = GetDlgItemInt(hwnd_, IDC_ZONE_W, nullptr, FALSE);
    cfg.zone.height = GetDlgItemInt(hwnd_, IDC_ZONE_H, nullptr, FALSE);
    if (cfg.zone.width  < 60) cfg.zone.width  = 60;
    if (cfg.zone.height < 60) cfg.zone.height = 60;

    int opVal = (int)SendDlgItemMessage(hwnd_, IDC_ZONE_OPACITY, TBM_GETPOS, 0, 0);
    cfg.zone.opacity = opVal / 100.0;

    cfg.zone.locked        = IsDlgButtonChecked(hwnd_, IDC_ZONE_LOCKED)  == BST_CHECKED;
    cfg.wheel_block        = IsDlgButtonChecked(hwnd_, IDC_WHEEL_BLOCK)  == BST_CHECKED;
    cfg.start_with_windows = IsDlgButtonChecked(hwnd_, IDC_START_WINDOWS)== BST_CHECKED;
    cfg.sound.enabled      = IsDlgButtonChecked(hwnd_, IDC_SOUND_ENABLED)== BST_CHECKED;

    cfg.scroll.scroll_amount    = GetDlgItemInt(hwnd_, IDC_SCROLL_AMOUNT, nullptr, FALSE);
    cfg.scroll.continuous_speed = GetDlgItemInt(hwnd_, IDC_HOLD_SPEED,    nullptr, FALSE);
    cfg.scroll.continuous_accel = GetDlgItemInt(hwnd_, IDC_HOLD_ACCEL,    nullptr, FALSE);
    cfg.scroll.hover_speed      = GetDlgItemInt(hwnd_, IDC_HOVER_SPEED,   nullptr, FALSE);
    if (cfg.scroll.scroll_amount < 10) cfg.scroll.scroll_amount = 10;

    wchar_t buf[128];
    GetDlgItemTextW(hwnd_, IDC_HK_TOGGLE, buf, 128);
    cfg.hotkeys.toggle_enabled = std::string(buf, buf + wcslen(buf));
    GetDlgItemTextW(hwnd_, IDC_HK_EDIT, buf, 128);
    cfg.hotkeys.toggle_edit = std::string(buf, buf + wcslen(buf));
    GetDlgItemTextW(hwnd_, IDC_HK_WHEEL, buf, 128);
    cfg.hotkeys.toggle_wheel = std::string(buf, buf + wcslen(buf));
}

void WinMainWindow::SetStatusText(const std::wstring& text) {
    if (hwnd_) SetDlgItemTextW(hwnd_, IDC_STATUS_BAR, text.c_str());
}

// ─── Window Procedure ───
LRESULT CALLBACK WinMainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = g_mainWnd;

    switch (msg) {
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc; GetClientRect(hwnd, &rc);
        if (self && self->hBrushBg_)
            FillRect(hdc, &rc, self->hBrushBg_);
        return 1;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, CLR_TEXT);
        SetBkColor(hdc, CLR_BG);
        if (self) return (LRESULT)self->hBrushBg_;
        break;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, CLR_TEXT);
        SetBkColor(hdc, CLR_SURFACE);
        if (self) return (LRESULT)self->hBrushSurface_;
        break;
    }

    // ── Opacity slider: live preview ──
    case WM_HSCROLL: {
        if (self && (HWND)lParam == GetDlgItem(hwnd, IDC_ZONE_OPACITY)) {
            int pos = (int)SendDlgItemMessage(hwnd, IDC_ZONE_OPACITY, TBM_GETPOS, 0, 0);
            wchar_t buf[16]; swprintf_s(buf, L"%d%%", pos);
            SetDlgItemTextW(hwnd, IDC_ZONE_OPACITY_LBL, buf);
            if (self->onEvent_) self->onEvent_(WinMainWindow::EVT_OPACITY_CHANGED);
        }
        return 0;
    }

    // ── Status timer: update status bar ──
    case WM_TIMER:
        if (self && wParam == IDC_TIMER_STATUS) {
            bool zoneOn = self->cfg_ && self->cfg_->enabled;
            std::wstring status = zoneOn ? L"Zone: ON" : L"Zone: OFF";

            int modeIdx = (int)SendDlgItemMessage(hwnd, IDC_MODE_COMBO, CB_GETCURSEL, 0, 0);
            status += L" | Mode ";
            status += std::to_wstring(modeIdx + 1);

            status += L" | ScrollNice v1.0";
            self->SetStatusText(status);
        }
        return 0;

    case WM_COMMAND: {
        if (!self) break;
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        // Zone checkbox changed → immediate effect
        if (id == IDC_ZONE_ENABLE && code == BN_CLICKED) {
            if (self->onEvent_) self->onEvent_(EVT_ZONE_TOGGLED);
            return 0;
        }

        // Mode combo changed → immediate effect
        if (id == IDC_MODE_COMBO && code == CBN_SELCHANGE) {
            if (self->onEvent_) self->onEvent_(EVT_MODE_CHANGED);
            return 0;
        }

        // Save button
        if (id == IDC_SAVE_BTN) {
            if (self->cfg_) {
                self->ReadControls(*self->cfg_);
                if (self->onSave_) self->onSave_(*self->cfg_);
            }
            return 0;
        }

        // Reset button
        if (id == IDC_RESET_BTN) {
            if (self->cfg_) {
                AppConfig def = ConfigStore::GetDefault();
                *self->cfg_ = def;
                self->SyncFromConfig(def);
                if (self->onEvent_) self->onEvent_(EVT_RESET);
            }
            return 0;
        }
        break;
    }

    // ── X button = minimize to tray ──
    case WM_CLOSE:
        if (self) {
            self->Hide();
            if (self->onEvent_) self->onEvent_(EVT_MINIMIZE);
        }
        return 0;  // don't destroy

    case WM_DESTROY:
        KillTimer(hwnd, IDC_TIMER_STATUS);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace sn
