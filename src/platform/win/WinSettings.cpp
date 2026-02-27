#include "WinSettings.h"
#include <commctrl.h>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

namespace sn {

static WinSettings* g_settings = nullptr;

// Control IDs
enum {
    IDC_ENABLED = 101,
    IDC_START_WINDOWS = 102,
    IDC_WHEEL_BLOCK = 103,
    IDC_MODE_CLICK_LR = 110,
    IDC_MODE_CLICK_RL = 111,
    IDC_MODE_SPLIT_LR = 112,
    IDC_MODE_SPLIT_TB = 113,
    IDC_MODE_CONTINUOUS = 114,
    IDC_SCROLL_AMOUNT = 120,
    IDC_SCROLL_AMOUNT_LABEL = 121,
    IDC_ZONE_WIDTH = 130,
    IDC_ZONE_HEIGHT = 131,
    IDC_ZONE_OPACITY = 132,
    IDC_ZONE_LOCKED = 133,
    IDC_SOUND_ENABLED = 140,
    IDC_HOTKEY_TOGGLE = 150,
    IDC_HOTKEY_EDIT = 151,
    IDC_OK_BTN = 200,
    IDC_CANCEL_BTN = 201
};

void WinSettings::Show(HINSTANCE hInst, HWND parent, AppConfig& cfg, SettingsCallback onSave) {
    g_settings = this;
    cfg_ = &cfg;
    onSave_ = onSave;

    // Create dialog from scratch using CreateWindowEx
    const int DW = 380, DH = 520;
    RECT parentRect;
    GetWindowRect(parent ? parent : GetDesktopWindow(), &parentRect);
    int dx = (parentRect.left + parentRect.right - DW) / 2;
    int dy = (parentRect.top + parentRect.bottom - DH) / 2;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DlgProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"ScrollNice_Settings";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    dlg_ = CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"ScrollNice_Settings", L"ScrollNice Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        dx, dy, DW, DH, parent, nullptr, hInst, nullptr);

    InitControls(dlg_);

    // Modal loop
    EnableWindow(parent, FALSE);
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsWindow(dlg_)) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
}

void WinSettings::InitControls(HWND dlg) {
    auto mk = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id) {
        return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                               x, y, w, h, dlg, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    };
    int y = 10;

    // ─── General ───
    mk(L"STATIC", L"── General ──", SS_CENTER, 10, y, 340, 18, 0); y += 24;
    mk(L"BUTTON", L"Enabled", BS_AUTOCHECKBOX, 20, y, 150, 20, IDC_ENABLED); y += 22;
    mk(L"BUTTON", L"Start with Windows", BS_AUTOCHECKBOX, 20, y, 200, 20, IDC_START_WINDOWS); y += 22;
    mk(L"BUTTON", L"Block Mouse Wheel", BS_AUTOCHECKBOX, 20, y, 200, 20, IDC_WHEEL_BLOCK); y += 28;

    // ─── Scroll Mode ───
    mk(L"STATIC", L"── Scroll Mode ──", SS_CENTER, 10, y, 340, 18, 0); y += 24;
    mk(L"BUTTON", L"Click: Left\u2191 Right\u2193", BS_AUTORADIOBUTTON | WS_GROUP, 20, y, 200, 20, IDC_MODE_CLICK_LR); y += 20;
    mk(L"BUTTON", L"Click: Left\u2193 Right\u2191", BS_AUTORADIOBUTTON, 20, y, 200, 20, IDC_MODE_CLICK_RL); y += 20;
    mk(L"BUTTON", L"Split Left/Right", BS_AUTORADIOBUTTON, 20, y, 200, 20, IDC_MODE_SPLIT_LR); y += 20;
    mk(L"BUTTON", L"Split Top/Bottom", BS_AUTORADIOBUTTON, 20, y, 200, 20, IDC_MODE_SPLIT_TB); y += 20;
    mk(L"BUTTON", L"Continuous (Hold)", BS_AUTORADIOBUTTON, 20, y, 200, 20, IDC_MODE_CONTINUOUS); y += 28;

    // ─── Scroll Sensitivity ───
    mk(L"STATIC", L"── Sensitivity ──", SS_CENTER, 10, y, 340, 18, 0); y += 24;
    mk(L"STATIC", L"Scroll Amount:", 0, 20, y, 100, 18, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, 130, y - 2, 60, 22, IDC_SCROLL_AMOUNT);
    mk(L"STATIC", L"px", 0, 195, y, 30, 18, 0); y += 28;

    // ─── Zone ───
    mk(L"STATIC", L"── Zone ──", SS_CENTER, 10, y, 340, 18, 0); y += 24;
    mk(L"STATIC", L"Width:", 0, 20, y, 50, 18, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, 75, y - 2, 50, 22, IDC_ZONE_WIDTH);
    mk(L"STATIC", L"Height:", 0, 140, y, 50, 18, 0);
    mk(L"EDIT", L"", WS_BORDER | ES_NUMBER, 195, y - 2, 50, 22, IDC_ZONE_HEIGHT); y += 26;
    mk(L"STATIC", L"Opacity:", 0, 20, y, 60, 18, 0);
    HWND slider = mk(TRACKBAR_CLASSW, L"", TBS_HORZ | TBS_AUTOTICKS, 85, y - 2, 180, 24, IDC_ZONE_OPACITY); y += 26;
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(5, 100));
    mk(L"BUTTON", L"Lock Position", BS_AUTOCHECKBOX, 20, y, 150, 20, IDC_ZONE_LOCKED); y += 28;

    // ─── Sound ───
    mk(L"STATIC", L"── Sound ──", SS_CENTER, 10, y, 340, 18, 0); y += 24;
    mk(L"BUTTON", L"Click Sound", BS_AUTOCHECKBOX, 20, y, 150, 20, IDC_SOUND_ENABLED); y += 28;

    // ─── Hotkeys ───
    mk(L"STATIC", L"── Hotkeys ──", SS_CENTER, 10, y, 340, 18, 0); y += 24;
    mk(L"STATIC", L"Toggle:", 0, 20, y, 50, 18, 0);
    mk(L"EDIT", L"", WS_BORDER, 75, y - 2, 120, 22, IDC_HOTKEY_TOGGLE);
    mk(L"STATIC", L"Edit:", 0, 210, y, 40, 18, 0);
    mk(L"EDIT", L"", WS_BORDER, 255, y - 2, 100, 22, IDC_HOTKEY_EDIT); y += 34;

    // ─── Buttons ───
    mk(L"BUTTON", L"Save", BS_DEFPUSHBUTTON, 100, y, 80, 30, IDC_OK_BTN);
    mk(L"BUTTON", L"Cancel", 0, 200, y, 80, 30, IDC_CANCEL_BTN);

    // ─── Set values from config ───
    if (cfg_) {
        CheckDlgButton(dlg, IDC_ENABLED, cfg_->enabled ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(dlg, IDC_START_WINDOWS, cfg_->start_with_windows ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(dlg, IDC_WHEEL_BLOCK, cfg_->wheel_block ? BST_CHECKED : BST_UNCHECKED);

        ScrollMode m = ScrollModeFromString(cfg_->scroll.mode);
        int modeId = IDC_MODE_CLICK_LR;
        if (m == ScrollMode::ClickRL) modeId = IDC_MODE_CLICK_RL;
        if (m == ScrollMode::SplitLR) modeId = IDC_MODE_SPLIT_LR;
        if (m == ScrollMode::SplitTB) modeId = IDC_MODE_SPLIT_TB;
        if (m == ScrollMode::Continuous) modeId = IDC_MODE_CONTINUOUS;
        CheckRadioButton(dlg, IDC_MODE_CLICK_LR, IDC_MODE_CONTINUOUS, modeId);

        SetDlgItemInt(dlg, IDC_SCROLL_AMOUNT, cfg_->scroll.scroll_amount, FALSE);
        SetDlgItemInt(dlg, IDC_ZONE_WIDTH, cfg_->zone.width, FALSE);
        SetDlgItemInt(dlg, IDC_ZONE_HEIGHT, cfg_->zone.height, FALSE);
        SendDlgItemMessage(dlg, IDC_ZONE_OPACITY, TBM_SETPOS, TRUE, (int)(cfg_->zone.opacity * 100));
        CheckDlgButton(dlg, IDC_ZONE_LOCKED, cfg_->zone.locked ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(dlg, IDC_SOUND_ENABLED, cfg_->sound.enabled ? BST_CHECKED : BST_UNCHECKED);

        std::wstring hkToggle(cfg_->hotkeys.toggle_enabled.begin(), cfg_->hotkeys.toggle_enabled.end());
        std::wstring hkEdit(cfg_->hotkeys.toggle_edit.begin(), cfg_->hotkeys.toggle_edit.end());
        SetDlgItemTextW(dlg, IDC_HOTKEY_TOGGLE, hkToggle.c_str());
        SetDlgItemTextW(dlg, IDC_HOTKEY_EDIT, hkEdit.c_str());
    }
}

void WinSettings::ReadControls(HWND dlg) {
    if (!cfg_) return;

    cfg_->enabled = IsDlgButtonChecked(dlg, IDC_ENABLED) == BST_CHECKED;
    cfg_->start_with_windows = IsDlgButtonChecked(dlg, IDC_START_WINDOWS) == BST_CHECKED;
    cfg_->wheel_block = IsDlgButtonChecked(dlg, IDC_WHEEL_BLOCK) == BST_CHECKED;

    if (IsDlgButtonChecked(dlg, IDC_MODE_CLICK_LR))   cfg_->scroll.mode = "click_lr";
    if (IsDlgButtonChecked(dlg, IDC_MODE_CLICK_RL))   cfg_->scroll.mode = "click_rl";
    if (IsDlgButtonChecked(dlg, IDC_MODE_SPLIT_LR))   cfg_->scroll.mode = "split_lr";
    if (IsDlgButtonChecked(dlg, IDC_MODE_SPLIT_TB))   cfg_->scroll.mode = "split_tb";
    if (IsDlgButtonChecked(dlg, IDC_MODE_CONTINUOUS)) cfg_->scroll.mode = "continuous";

    cfg_->scroll.scroll_amount = GetDlgItemInt(dlg, IDC_SCROLL_AMOUNT, nullptr, FALSE);
    if (cfg_->scroll.scroll_amount < 10) cfg_->scroll.scroll_amount = 10;

    cfg_->zone.width = GetDlgItemInt(dlg, IDC_ZONE_WIDTH, nullptr, FALSE);
    cfg_->zone.height = GetDlgItemInt(dlg, IDC_ZONE_HEIGHT, nullptr, FALSE);
    if (cfg_->zone.width < 60) cfg_->zone.width = 60;
    if (cfg_->zone.height < 60) cfg_->zone.height = 60;

    int opVal = (int)SendDlgItemMessage(dlg, IDC_ZONE_OPACITY, TBM_GETPOS, 0, 0);
    cfg_->zone.opacity = opVal / 100.0;

    cfg_->zone.locked = IsDlgButtonChecked(dlg, IDC_ZONE_LOCKED) == BST_CHECKED;
    cfg_->sound.enabled = IsDlgButtonChecked(dlg, IDC_SOUND_ENABLED) == BST_CHECKED;

    wchar_t buf[128];
    GetDlgItemTextW(dlg, IDC_HOTKEY_TOGGLE, buf, 128);
    cfg_->hotkeys.toggle_enabled = std::string(buf, buf + wcslen(buf));
    GetDlgItemTextW(dlg, IDC_HOTKEY_EDIT, buf, 128);
    cfg_->hotkeys.toggle_edit = std::string(buf, buf + wcslen(buf));
}

INT_PTR CALLBACK WinSettings::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = g_settings;
    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_OK_BTN) {
            self->ReadControls(hwnd);
            if (self->onSave_ && self->cfg_) self->onSave_(*self->cfg_);
            DestroyWindow(hwnd);
            self->dlg_ = nullptr;
            PostQuitMessage(0);
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_CANCEL_BTN) {
            DestroyWindow(hwnd);
            self->dlg_ = nullptr;
            PostQuitMessage(0);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        self->dlg_ = nullptr;
        PostQuitMessage(0);
        return TRUE;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace sn
