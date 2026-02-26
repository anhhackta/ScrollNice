#include "WinTray.h"

namespace sn {

bool WinTray::Create(HWND parentHwnd, HINSTANCE hInst, MenuCallback cb) {
    hwnd_ = parentHwnd;
    callback_ = cb;

    nid_.cbSize = sizeof(nid_);
    nid_.hWnd   = parentHwnd;
    nid_.uID    = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAYICON;
    nid_.hIcon  = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nid_.szTip, L"ScrollNice - Enabled");

    return Shell_NotifyIconW(NIM_ADD, &nid_) == TRUE;
}

void WinTray::Destroy() {
    Shell_NotifyIconW(NIM_DELETE, &nid_);
}

void WinTray::SetEnabled(bool enabled) {
    enabled_ = enabled;
    wcscpy_s(nid_.szTip, enabled ? L"ScrollNice - Enabled" : L"ScrollNice - Disabled");
    nid_.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void WinTray::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON) {
        if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) {
            ShowContextMenu();
        } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            if (callback_) callback_(ID_TOGGLE);
        }
    }
}

void WinTray::ShowContextMenu() {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TOGGLE, enabled_ ? L"Disable" : L"Enable");
    AppendMenuW(hMenu, MF_STRING, ID_EDIT,   L"Edit Mode");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_QUIT,   L"Quit");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd_);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
    DestroyMenu(hMenu);
    PostMessage(hwnd_, WM_NULL, 0, 0);
}

} // namespace sn
