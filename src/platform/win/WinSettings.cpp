#include "WinSettings.h"
#include <commctrl.h>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

namespace sn {

static WinSettings* g_settings = nullptr;

// ─── Color constants (dark theme) ───
static const COLORREF CLR_BG      = RGB(28, 28, 38);     // dark navy
static const COLORREF CLR_SURFACE = RGB(38, 38, 52);     // slightly lighter surface
static const COLORREF CLR_TEXT    = RGB(230, 230, 240);  // near-white text
static const COLORREF CLR_ACCENT  = RGB(99, 130, 246);   // blue accent
static const COLORREF CLR_BORDER  = RGB(70, 70, 90);

static HBRUSH hBrushBg      = nullptr;
static HBRUSH hBrushSurface = nullptr;

// Custom message to end the settings dialog without touching main loop
static const UINT WM_SETTINGS_DONE = WM_APP + 100;

// Control IDs
enum {
    IDC_ENABLED        = 101,
    IDC_START_WINDOWS  = 102,
    IDC_WHEEL_BLOCK    = 103,
    IDC_MODE_CLICK_HOLD = 110,
    IDC_MODE_SPLIT_HOLD = 111,
    IDC_MODE_HOVER_AUTO = 112,
    IDC_SCROLL_AMOUNT  = 120,
    IDC_ZONE_WIDTH     = 130,
    IDC_ZONE_HEIGHT    = 131,
    IDC_ZONE_OPACITY   = 132,
    IDC_ZONE_OPACITY_LABEL = 133,
    IDC_ZONE_LOCKED    = 134,
    IDC_SOUND_ENABLED  = 140,
    IDC_HOTKEY_TOGGLE  = 150,
    IDC_HOTKEY_EDIT    = 151,
    IDC_OK_BTN         = 200,
    IDC_CANCEL_BTN     = 201,
};

// ─── Helper: create child control ───
static HWND Mk(HWND dlg, const wchar_t* cls, const wchar_t* text
               , DWORD style, int x, int y, int w, int h, int id, HINSTANCE hInst) {
    return CreateWindowExW(0, cls, text,
        WS_CHILD | WS_VISIBLE | style,
        x, y, w, h, dlg, (HMENU)(INT_PTR)id, hInst, nullptr);
}

// ─── Show ───
void WinSettings::Show(HINSTANCE hInst, HWND parent, AppConfig& cfg, SettingsCallback onSave) {
    g_settings = this;
    cfg_       = &cfg;
    onSave_    = onSave;
    hInst_     = hInst;

    hBrushBg      = CreateSolidBrush(CLR_BG);
    hBrushSurface = CreateSolidBrush(CLR_SURFACE);

    const int DW = 400, DH = 548;
    RECT parentRect;
    GetWindowRect(parent ? parent : GetDesktopWindow(), &parentRect);
    int dx = (parentRect.left + parentRect.right  - DW) / 2;
    int dy = (parentRect.top  + parentRect.bottom - DH) / 2;

    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = DlgProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = L"ScrollNice_Settings";
    wc.hbrBackground  = nullptr;            // painted via WM_CTLCOLOR*
    wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon          = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    dlg_ = CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"ScrollNice_Settings", L"ScrollNice - Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, DW, DH, parent, nullptr, hInst, nullptr);

    InitControls(dlg_);

    // Modal loop — WM_SETTINGS_DONE exits it safely.
    // NOTE: We use PostThreadMessage (not PostMessage) because the dialog HWND
    // is destroyed before posting, so PostMessage to it is silently dropped.
    // PostThreadMessage delivers directly to this thread's queue (no HWND needed).
    DWORD threadId = GetCurrentThreadId();
    EnableWindow(parent, FALSE);
    done_ = false;
    MSG msg;
    while (!done_ && GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_SETTINGS_DONE) { done_ = true; break; }
        if (msg.message == WM_QUIT)          { done_ = true; PostQuitMessage(0); break; }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    (void)threadId; // used in lambda captures below via PostThreadMessage

    if (hBrushBg)      { DeleteObject(hBrushBg);      hBrushBg      = nullptr; }
    if (hBrushSurface) { DeleteObject(hBrushSurface); hBrushSurface = nullptr; }
}

// ─── InitControls ───
void WinSettings::InitControls(HWND dlg) {
    auto hInst = hInst_;
    auto mk = [&](const wchar_t* cls, const wchar_t* text,
                  DWORD style, int x, int y, int w, int h, int id) {
        return Mk(dlg, cls, text, style, x, y, w, h, id, hInst);
    };

    // Set dark background on the window itself
    SetClassLongPtrW(dlg, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushBg);
    InvalidateRect(dlg, nullptr, TRUE);

    // Bold font for the dialog
    HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SendMessage(dlg, WM_SETFONT, (WPARAM)hFont, TRUE);

    int y = 10, PW = 364;

    // ═══ General ═══
    HWND grp1 = mk(L"BUTTON", L"  General", BS_GROUPBOX, 10, y, PW, 82, 0); y += 24;
    SendMessage(grp1, WM_SETFONT, (WPARAM)hFont, TRUE);
    mk(L"BUTTON", L"Enable ScrollNice",   BS_AUTOCHECKBOX, 22, y, 180, 20, IDC_ENABLED);        y += 22;
    mk(L"BUTTON", L"Start with Windows",  BS_AUTOCHECKBOX, 22, y, 180, 20, IDC_START_WINDOWS);  y += 22;
    mk(L"BUTTON", L"Block Mouse Wheel (Ctrl+Alt+W)", BS_AUTOCHECKBOX, 22, y, 280, 20, IDC_WHEEL_BLOCK);
    y += 24 + 12; // gap after groupbox

    // ═══ Scroll Mode ═══
    HWND grp2 = mk(L"BUTTON", L"  Scroll Mode", BS_GROUPBOX, 10, y, PW, 90, 0); y += 24;
    SendMessage(grp2, WM_SETFONT, (WPARAM)hFont, TRUE);
    mk(L"BUTTON", L"Mode 1 \u2014 Click/Hold:  L\u2191  R\u2193",
       BS_AUTORADIOBUTTON | WS_GROUP, 22, y, PW - 24, 20, IDC_MODE_CLICK_HOLD); y += 22;
    mk(L"BUTTON", L"Mode 2 \u2014 Click/Hold:  Top\u2191  Bottom\u2193",
       BS_AUTORADIOBUTTON, 22, y, PW - 24, 20, IDC_MODE_SPLIT_HOLD); y += 22;
    mk(L"BUTTON", L"Mode 3 \u2014 Hover auto: move cursor, no click",
       BS_AUTORADIOBUTTON, 22, y, PW - 24, 20, IDC_MODE_HOVER_AUTO);
    y += 22 + 12;

    // ═══ Sensitivity ═══
    HWND grp3 = mk(L"BUTTON", L"  Sensitivity", BS_GROUPBOX, 10, y, PW, 46, 0); y += 24;
    SendMessage(grp3, WM_SETFONT, (WPARAM)hFont, TRUE);
    mk(L"STATIC", L"Scroll Amount:", 0, 22, y, 100, 18, 0);
    mk(L"EDIT",   L"", WS_BORDER | ES_NUMBER, 130, y - 2, 60, 22, IDC_SCROLL_AMOUNT);
    mk(L"STATIC", L"px / click", 0, 196, y, 80, 18, 0);
    y += 22 + 12;

    // ═══ Zone ═══
    HWND grp4 = mk(L"BUTTON", L"  Zone", BS_GROUPBOX, 10, y, PW, 90, 0); y += 24;
    SendMessage(grp4, WM_SETFONT, (WPARAM)hFont, TRUE);
    mk(L"STATIC", L"Width:",  0, 22,  y, 50, 18, 0);
    mk(L"EDIT",   L"", WS_BORDER | ES_NUMBER, 72,  y - 2, 55, 22, IDC_ZONE_WIDTH);
    mk(L"STATIC", L"Height:", 0, 148, y, 50, 18, 0);
    mk(L"EDIT",   L"", WS_BORDER | ES_NUMBER, 198, y - 2, 55, 22, IDC_ZONE_HEIGHT);
    y += 26;
    mk(L"STATIC", L"Opacity:", 0, 22, y, 60, 18, 0);
    HWND slider = mk(TRACKBAR_CLASSW, L"", TBS_HORZ | TBS_AUTOTICKS, 88, y - 2, 180, 24, IDC_ZONE_OPACITY);
    SendMessage(slider, TBM_SETRANGE,    TRUE, MAKELPARAM(5, 100));
    SendMessage(slider, TBM_SETTICFREQ, 10,  0);
    mk(L"STATIC", L"25%", SS_CENTER, 278, y, 40, 20, IDC_ZONE_OPACITY_LABEL);
    y += 26;
    mk(L"BUTTON", L"Lock Position", BS_AUTOCHECKBOX, 22, y, 150, 20, IDC_ZONE_LOCKED);
    y += 22 + 12;

    // ═══ Sound ═══
    HWND grp5 = mk(L"BUTTON", L"  Sound", BS_GROUPBOX, 10, y, PW, 44, 0); y += 24;
    SendMessage(grp5, WM_SETFONT, (WPARAM)hFont, TRUE);
    mk(L"BUTTON", L"Click Sound (built-in beep or custom WAV)", BS_AUTOCHECKBOX,
       22, y, 300, 20, IDC_SOUND_ENABLED);
    y += 22 + 12;

    // ═══ Hotkeys ═══
    HWND grp6 = mk(L"BUTTON", L"  Hotkeys", BS_GROUPBOX, 10, y, PW, 50, 0); y += 24;
    SendMessage(grp6, WM_SETFONT, (WPARAM)hFont, TRUE);
    mk(L"STATIC", L"Enable:",      0,   22, y, 52, 18, 0);
    mk(L"EDIT",   L"", WS_BORDER, 76,  y - 2, 120, 22, IDC_HOTKEY_TOGGLE);
    mk(L"STATIC", L"Edit mode:",   0,  210, y, 70, 18, 0);
    mk(L"EDIT",   L"", WS_BORDER, 282, y - 2, 90, 22, IDC_HOTKEY_EDIT);
    y += 26 + 16;

    // ─── Buttons ───
    mk(L"BUTTON", L"Save",   BS_DEFPUSHBUTTON, 110, y, 90, 32, IDC_OK_BTN);
    mk(L"BUTTON", L"Cancel", 0,                210, y, 90, 32, IDC_CANCEL_BTN);

    // Apply font to all children
    EnumChildWindows(dlg, [](HWND child, LPARAM lParam) -> BOOL {
        SendMessage(child, WM_SETFONT, (WPARAM)lParam, TRUE);
        return TRUE;
    }, (LPARAM)hFont);

    // ─── Populate values ───
    if (cfg_) {
        CheckDlgButton(dlg, IDC_ENABLED,       cfg_->enabled            ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(dlg, IDC_START_WINDOWS, cfg_->start_with_windows ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(dlg, IDC_WHEEL_BLOCK,   cfg_->wheel_block        ? BST_CHECKED : BST_UNCHECKED);

        ScrollMode m = ScrollModeFromString(cfg_->scroll.mode);
        int modeId = IDC_MODE_CLICK_HOLD;
        if (m == ScrollMode::SplitHold) modeId = IDC_MODE_SPLIT_HOLD;
        if (m == ScrollMode::HoverAuto) modeId = IDC_MODE_HOVER_AUTO;
        CheckRadioButton(dlg, IDC_MODE_CLICK_HOLD, IDC_MODE_HOVER_AUTO, modeId);

        SetDlgItemInt(dlg, IDC_SCROLL_AMOUNT, cfg_->scroll.scroll_amount, FALSE);
        SetDlgItemInt(dlg, IDC_ZONE_WIDTH,    cfg_->zone.width,           FALSE);
        SetDlgItemInt(dlg, IDC_ZONE_HEIGHT,   cfg_->zone.height,          FALSE);

        int opPct = (int)(cfg_->zone.opacity * 100);
        SendDlgItemMessage(dlg, IDC_ZONE_OPACITY, TBM_SETPOS, TRUE, opPct);
        wchar_t opBuf[16]; swprintf_s(opBuf, L"%d%%", opPct);
        SetDlgItemTextW(dlg, IDC_ZONE_OPACITY_LABEL, opBuf);

        CheckDlgButton(dlg, IDC_ZONE_LOCKED,   cfg_->zone.locked     ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(dlg, IDC_SOUND_ENABLED, cfg_->sound.enabled   ? BST_CHECKED : BST_UNCHECKED);

        std::wstring hkToggle(cfg_->hotkeys.toggle_enabled.begin(), cfg_->hotkeys.toggle_enabled.end());
        std::wstring hkEdit  (cfg_->hotkeys.toggle_edit.begin(),    cfg_->hotkeys.toggle_edit.end());
        SetDlgItemTextW(dlg, IDC_HOTKEY_TOGGLE, hkToggle.c_str());
        SetDlgItemTextW(dlg, IDC_HOTKEY_EDIT,   hkEdit.c_str());
    }
}

// ─── ReadControls ───
void WinSettings::ReadControls(HWND dlg) {
    if (!cfg_) return;

    cfg_->enabled            = IsDlgButtonChecked(dlg, IDC_ENABLED)       == BST_CHECKED;
    cfg_->start_with_windows = IsDlgButtonChecked(dlg, IDC_START_WINDOWS) == BST_CHECKED;
    cfg_->wheel_block        = IsDlgButtonChecked(dlg, IDC_WHEEL_BLOCK)   == BST_CHECKED;

    if (IsDlgButtonChecked(dlg, IDC_MODE_CLICK_HOLD)) cfg_->scroll.mode = "click_hold";
    if (IsDlgButtonChecked(dlg, IDC_MODE_SPLIT_HOLD)) cfg_->scroll.mode = "split_hold";
    if (IsDlgButtonChecked(dlg, IDC_MODE_HOVER_AUTO)) cfg_->scroll.mode = "hover_auto";

    cfg_->scroll.scroll_amount = GetDlgItemInt(dlg, IDC_SCROLL_AMOUNT, nullptr, FALSE);
    if (cfg_->scroll.scroll_amount < 10) cfg_->scroll.scroll_amount = 10;

    cfg_->zone.width  = GetDlgItemInt(dlg, IDC_ZONE_WIDTH,  nullptr, FALSE);
    cfg_->zone.height = GetDlgItemInt(dlg, IDC_ZONE_HEIGHT, nullptr, FALSE);
    if (cfg_->zone.width  < 60) cfg_->zone.width  = 60;
    if (cfg_->zone.height < 60) cfg_->zone.height = 60;

    int opVal = (int)SendDlgItemMessage(dlg, IDC_ZONE_OPACITY, TBM_GETPOS, 0, 0);
    cfg_->zone.opacity = opVal / 100.0;

    cfg_->zone.locked    = IsDlgButtonChecked(dlg, IDC_ZONE_LOCKED)   == BST_CHECKED;
    cfg_->sound.enabled  = IsDlgButtonChecked(dlg, IDC_SOUND_ENABLED) == BST_CHECKED;

    wchar_t buf[128];
    GetDlgItemTextW(dlg, IDC_HOTKEY_TOGGLE, buf, 128);
    cfg_->hotkeys.toggle_enabled = std::string(buf, buf + wcslen(buf));
    GetDlgItemTextW(dlg, IDC_HOTKEY_EDIT, buf, 128);
    cfg_->hotkeys.toggle_edit    = std::string(buf, buf + wcslen(buf));
}

// ─── Dialog Procedure ───
INT_PTR CALLBACK WinSettings::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = g_settings;
    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {

    // ── Dark theme: background ──
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc; GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, hBrushBg);
        return 1;
    }

    // ── Dark theme: child controls ──
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, CLR_TEXT);
        SetBkColor(hdc, CLR_BG);
        return (INT_PTR)hBrushBg;
    }

    // ── Opacity slider: update label live ──
    case WM_HSCROLL: {
        if ((HWND)lParam == GetDlgItem(hwnd, IDC_ZONE_OPACITY)) {
            int pos = (int)SendDlgItemMessage(hwnd, IDC_ZONE_OPACITY, TBM_GETPOS, 0, 0);
            wchar_t buf[16]; swprintf_s(buf, L"%d%%", pos);
            SetDlgItemTextW(hwnd, IDC_ZONE_OPACITY_LABEL, buf);
        }
        return 0;
    }

    // ── Keyboard: Escape = Cancel, Enter = Save ──
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            self->dlg_ = nullptr;
            PostThreadMessage(GetCurrentThreadId(), WM_SETTINGS_DONE, 0, 0);
            return 0;
        }
        if (wParam == VK_RETURN) {
            self->ReadControls(hwnd);
            if (self->onSave_ && self->cfg_) self->onSave_(*self->cfg_);
            DestroyWindow(hwnd);
            self->dlg_ = nullptr;
            PostThreadMessage(GetCurrentThreadId(), WM_SETTINGS_DONE, 0, 0);
            return 0;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_OK_BTN) {
            self->ReadControls(hwnd);
            if (self->onSave_ && self->cfg_) self->onSave_(*self->cfg_);
            DestroyWindow(hwnd);
            self->dlg_ = nullptr;
            PostThreadMessage(GetCurrentThreadId(), WM_SETTINGS_DONE, 0, 0);
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_CANCEL_BTN) {
            DestroyWindow(hwnd);
            self->dlg_ = nullptr;
            PostThreadMessage(GetCurrentThreadId(), WM_SETTINGS_DONE, 0, 0);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        self->dlg_ = nullptr;
        PostThreadMessage(GetCurrentThreadId(), WM_SETTINGS_DONE, 0, 0);
        return TRUE;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace sn
