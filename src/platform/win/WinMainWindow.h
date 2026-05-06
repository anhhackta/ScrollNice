#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include "../../core/Config.h"

namespace sn {

// ─────────────────────────────────────────────────────────
// WinMainWindow — The primary application window.
//
// Replaces the old WinSettings modal dialog. All controls are
// inline: zone toggle, mode selector, sliders, hotkey display.
//
// Behavior:
//   - Shown on startup (centered on screen)
//   - X button minimizes to tray (does NOT quit)
//   - Tray double-click restores the window
//   - Zone ON/OFF toggle takes effect immediately
//   - Mode dropdown takes effect immediately
//   - Opacity slider updates zone live
//   - [Save] persists to config.json
// ─────────────────────────────────────────────────────────

// Callback signatures
using MainWindowSaveCallback  = std::function<void(const AppConfig&)>;
using MainWindowEventCallback = std::function<void(int eventId)>;

class WinMainWindow {
public:
    // Event IDs sent via eventCallback
    enum Event {
        EVT_ZONE_TOGGLED    = 1,  // zone checkbox changed
        EVT_MODE_CHANGED    = 2,  // mode dropdown changed
        EVT_OPACITY_CHANGED = 3,  // opacity slider moved
        EVT_SAVE            = 10, // Save button
        EVT_RESET           = 11, // Reset button
        EVT_MINIMIZE        = 20, // X button (minimize to tray)
        EVT_QUIT            = 21, // Quit from menu/tray
    };

    bool Create(HINSTANCE hInst, AppConfig& cfg,
                MainWindowSaveCallback onSave,
                MainWindowEventCallback onEvent);
    void Destroy();

    void Show();
    void Hide();
    bool IsVisible() const;

    // Update controls from config (after external changes)
    void SyncFromConfig(const AppConfig& cfg);

    // Read current control values into config
    void ReadControls(AppConfig& cfg);

    // Update the status bar text
    void SetStatusText(const std::wstring& text);

    HWND Handle() const { return hwnd_; }

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    void InitControls();
    void ApplyDarkTheme(HWND hwnd);

    HWND       hwnd_    = nullptr;
    HINSTANCE  hInst_   = nullptr;
    AppConfig* cfg_     = nullptr;
    HFONT      hFont_   = nullptr;
    HFONT      hFontBold_ = nullptr;
    HFONT      hFontSmall_ = nullptr;
    HBRUSH     hBrushBg_  = nullptr;
    HBRUSH     hBrushSurface_ = nullptr;
    HBRUSH     hBrushCard_ = nullptr;

    MainWindowSaveCallback  onSave_;
    MainWindowEventCallback onEvent_;
};

} // namespace sn
