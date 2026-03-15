#pragma once
#include <windows.h>
#include <shellapi.h>
#include <functional>
#include <string>

namespace sn {

// ─────────────────────────────────────────────────────────
// WinTray — System tray icon + context menu + balloon tips
//
// On startup: shows balloon tip explaining Ctrl+Alt+S
// Tooltip: shows current mode name and enabled state
// Left click: toggles zone (quick action)
// Right click / double-click: context menu / settings
// ─────────────────────────────────────────────────────────
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

    // Update tooltip & icon state when zone is enabled/disabled
    void SetEnabled(bool enabled);

    // Update tooltip with current scroll mode name (e.g. "click_hold")
    void SetModeName(const std::string& mode);

    // Show a balloon notification (shown on startup to guide first-time users)
    void ShowBalloon(const wchar_t* title, const wchar_t* text,
                     DWORD icon = NIIF_INFO, UINT timeoutMs = 4000);

    void HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    static const UINT WM_TRAYICON = WM_USER + 100;

private:
    void ShowContextMenu();
    void UpdateTooltip();   // rebuild szTip from enabled_ + modeName_

    NOTIFYICONDATAW nid_ = {};
    HWND hwnd_           = nullptr;
    HINSTANCE hInst_     = nullptr;
    bool enabled_        = false;
    std::string modeName_;
    MenuCallback callback_;

};

} // namespace sn
