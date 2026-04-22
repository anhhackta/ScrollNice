#include "WinTray.h"

namespace sn {

bool WinTray::Create(HWND parentHwnd, HINSTANCE hInst, MenuCallback cb) {
    hwnd_     = parentHwnd;
    hInst_    = hInst;
    callback_ = cb;

    nid_.cbSize           = sizeof(nid_);
    nid_.hWnd             = parentHwnd;
    nid_.uID              = 1;
    nid_.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAYICON;

    // IDI_APPLICATION is a SYSTEM resource: must use nullptr, NOT app HINSTANCE.
    // Passing app hInst here returns nullptr -> Shell_NotifyIconW fails silently.
    nid_.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    wcscpy_s(nid_.szTip, L"ScrollNice - Disabled (Ctrl+Alt+S)");

    bool ok = Shell_NotifyIconW(NIM_ADD, &nid_) == TRUE;

    if (ok) {
        // Show startup balloon to guide first-time users
        ShowBalloon(
            L"ScrollNice is running",
            L"Press Ctrl+Alt+S to show the scroll zone.\n"
            L"Right-click this icon for options.",
            NIIF_INFO
        );
    }
    return ok;
}

void WinTray::Destroy() {
    Shell_NotifyIconW(NIM_DELETE, &nid_);
}

void WinTray::SetEnabled(bool enabled) {
    enabled_ = enabled;
    UpdateTooltip();
}

void WinTray::SetModeName(const std::string& mode) {
    modeName_ = mode;
    UpdateTooltip();
}

void WinTray::UpdateTooltip() {
    std::wstring tip = L"ScrollNice";

    if (!modeName_.empty()) {
        if      (modeName_ == "click_hold") tip += L" - Mode 1: Click/Hold";
        else if (modeName_ == "split_hold") tip += L" - Mode 2: Top/Bottom";
        else if (modeName_ == "hover_auto") tip += L" - Mode 3: Hover Auto";
    }

    tip += enabled_ ? L" | ON" : L" | OFF";

    wcsncpy_s(nid_.szTip, tip.c_str(), 127);
    nid_.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void WinTray::ShowBalloon(const wchar_t* title, const wchar_t* text, DWORD icon, UINT /*timeoutMs*/) {
    // Phase 1: Show the balloon (NIF_INFO)
    nid_.uFlags      = NIF_INFO;
    nid_.dwInfoFlags = icon;
    wcsncpy_s(nid_.szInfoTitle, title, ARRAYSIZE(nid_.szInfoTitle) - 1);
    wcsncpy_s(nid_.szInfo,      text,  ARRAYSIZE(nid_.szInfo) - 1);
    Shell_NotifyIconW(NIM_MODIFY, &nid_);

    // Phase 2: Immediately restore NIF_ICON|NIF_MESSAGE|NIF_TIP so the icon
    // stays visible after the balloon auto-dismisses. If we skip this call,
    // the icon disappears when the balloon times out.
    nid_.uFlags      = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.szInfo[0]      = L'\0';
    nid_.szInfoTitle[0] = L'\0';
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void WinTray::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg != WM_TRAYICON) return;

    switch (LOWORD(lParam)) {
    case WM_RBUTTONUP:
    case WM_CONTEXTMENU:
        ShowContextMenu();
        break;
    case WM_LBUTTONDBLCLK:
        if (callback_) callback_(ID_SETTINGS);
        break;
    case WM_LBUTTONUP:
        // Quick toggle: single left-click on tray icon
        if (callback_) callback_(ID_TOGGLE);
        break;
    }
}

void WinTray::ShowContextMenu() {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING,    ID_TOGGLE,   enabled_ ? L"&Disable Zone" : L"&Enable Zone");
    AppendMenuW(hMenu, MF_STRING,    ID_EDIT,     L"&Edit Mode (move/resize)");
    AppendMenuW(hMenu, MF_STRING,    ID_SETTINGS, L"&Settings...");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING,    ID_QUIT,     L"&Quit ScrollNice");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd_);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
    DestroyMenu(hMenu);
    PostMessage(hwnd_, WM_NULL, 0, 0);
}

} // namespace sn
