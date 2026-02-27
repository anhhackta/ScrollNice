#pragma once
#include <windows.h>
#include <shellapi.h>
#include <functional>

namespace sn {

class WinTray {
public:
    enum MenuItem {
        ID_TOGGLE   = 1001,
        ID_EDIT     = 1002,
        ID_SETTINGS = 1003,
        ID_QUIT     = 1004
    };

    using MenuCallback = std::function<void(MenuItem)>;

    bool Create(HWND parentHwnd, HINSTANCE hInst, MenuCallback cb);
    void Destroy();
    void SetEnabled(bool enabled);
    void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    static const UINT WM_TRAYICON = WM_USER + 100;

private:
    void ShowContextMenu();

    NOTIFYICONDATAW nid_ = {};
    HWND hwnd_ = nullptr;
    bool enabled_ = true;
    MenuCallback callback_;
};

} // namespace sn
