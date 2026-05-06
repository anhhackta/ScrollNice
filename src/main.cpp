// ScrollNice v2.1 — main application entry point
// Architecture:
//   WinMainWindow (visible GUI) → controls zone, settings, tray
//   WinOverlay    (floating zone) → scroll events → ScrollEngine
//   WinTray       (system tray icon) → minimize/restore main window
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <string>
#include <filesystem>
#include <cmath>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")

#include "core/Config.h"
#include "core/Zone.h"
#include "core/ScrollEngine.h"
#include "core/StateMachine.h"
#include "platform/win/WinMouseHook.h"
#include "platform/win/WinOverlay.h"
#include "platform/win/WinTray.h"
#include "platform/win/WinHotkeys.h"
#include "platform/win/WinMainWindow.h"

// ─────────── Globals ───────────
static sn::ConfigStore      g_configStore;
static sn::ZoneManager      g_zoneManager;
static sn::ScrollEngine     g_scrollEngine;
static sn::StateMachine     g_stateMachine;
static sn::WinOverlay       g_overlay;
static sn::WinTray          g_tray;
static sn::WinHotkeys       g_hotkeys;
static sn::WinMainWindow    g_mainWindow;

static std::string g_configPath;
static HINSTANCE   g_hInstance = nullptr;

// Continuous/hold scroll state
static UINT_PTR g_holdTimer = 0;
static const UINT_PTR TIMER_ID_HOLD = 501;
static int  g_holdDirection = 0;
static ULONGLONG g_holdStartTime = 0;
static bool g_leftHeld = false;
static bool g_rightHeld = false;

// Hover scroll state (Mode 3)
static UINT_PTR g_hoverTimer = 0;
static const UINT_PTR TIMER_ID_HOVER = 502;
static int  g_hoverDirection = 0;

// Last cursor pos outside zone (for FindScrollTarget)
static POINT g_lastOutsidePos = {-1, -1};

// Hidden message window (for hotkeys + timers)
static const wchar_t* kMsgWindowClass = L"ScrollNice_MsgWnd";
static HWND g_msgWnd = nullptr;

// ─────────── Forward declarations ───────────
static void HandleZoneClick(int button, bool isDown, POINT clickPos, int zoneW, int zoneH);
static void HandleZoneHover(POINT clientPos, int zoneW, int zoneH);
static void StartHoldScroll(int direction);
static void StopHoldScroll();
static void StartHoverScroll(int direction);
static void StopHoverScroll();
static void PlayClickSound();
static void ApplyConfig();
static void SetStartWithWindows(bool enable);
static std::string GetConfigPath();
static void UpdateWheelBlockHook(bool enable);
static void OnHotkey(int id);
static HWND FindScrollTarget();
static void OnMainWindowEvent(int eventId);
static bool OnMouseEvent(POINT, DWORD, MSLLHOOKSTRUCT*);

// ─────────── Message window proc (hotkeys + timers) ───────────
static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_HOTKEY:
        g_hotkeys.HandleMessage(wParam);
        return 0;

    case WM_TIMER: {
        auto& cfg = g_configStore.Get();
        if (wParam == TIMER_ID_HOLD && g_holdDirection != 0) {
            ULONGLONG now = GetTickCount64();
            if (now < g_holdStartTime) return 0;
            double holdSec = (now - g_holdStartTime) / 1000.0;
            g_scrollEngine.ContinuousScrollTick(g_holdDirection,
                cfg.scroll.continuous_speed, cfg.scroll.continuous_accel, holdSec);
        }
        if (wParam == TIMER_ID_HOVER && g_hoverDirection != 0) {
            g_scrollEngine.ContinuousScrollTick(g_hoverDirection,
                cfg.scroll.hover_speed, 1, 0.5);
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case sn::WinTray::ID_TOGGLE:
            g_stateMachine.ToggleEnabled();
            g_overlay.SetEnabled(g_stateMachine.IsEnabled());
            g_tray.SetEnabled(g_stateMachine.IsEnabled());
            // Sync main window checkbox
            {
                auto& cfg = g_configStore.Get();
                cfg.enabled = g_stateMachine.IsEnabled();
                g_mainWindow.SyncFromConfig(cfg);
            }
            break;
        case sn::WinTray::ID_EDIT:
            g_stateMachine.ToggleEdit();
            g_overlay.SetEditMode(g_stateMachine.IsEditing());
            break;
        case sn::WinTray::ID_SETTINGS:
            // Tray double-click / settings → show main window
            g_mainWindow.Show();
            break;
        case sn::WinTray::ID_QUIT:
            PostQuitMessage(0);
            break;
        }
        return 0;

    default:
        if (msg == sn::WinTray::WM_TRAYICON) {
            g_tray.HandleMessage(msg, wParam, lParam);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// ─────────── Zone event handler ───────────
static void OnZoneEvent(const sn::ZoneEventData& e) {
    switch (e.event) {
    case sn::ZoneEvent::LeftClickDown:
        g_scrollEngine.SetTargetHwnd(FindScrollTarget());
        HandleZoneClick(0, true, e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    case sn::ZoneEvent::LeftClickUp:
        HandleZoneClick(0, false, e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    case sn::ZoneEvent::RightClickDown:
        g_scrollEngine.SetTargetHwnd(FindScrollTarget());
        HandleZoneClick(1, true, e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    case sn::ZoneEvent::RightClickUp:
        HandleZoneClick(1, false, e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    case sn::ZoneEvent::HoverMove:
        if (!g_scrollEngine.GetTargetHwnd())
            g_scrollEngine.SetTargetHwnd(FindScrollTarget());
        HandleZoneHover(e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    case sn::ZoneEvent::HoverLeave:
        GetCursorPos(&g_lastOutsidePos);
        g_scrollEngine.SetTargetHwnd(nullptr);
        StopHoverScroll();
        break;
    default:
        break;
    }
}

// ─────────── 3-Mode click/hold logic ───────────
static void HandleZoneClick(int button, bool isDown, POINT clickPos, int zoneW, int zoneH) {
    auto& cfg = g_configStore.Get();
    sn::ScrollMode mode = sn::ScrollModeFromString(cfg.scroll.mode);

    if (mode == sn::ScrollMode::HoverAuto) return;

    if (isDown) {
        int direction = 0;

        if (mode == sn::ScrollMode::ClickHold) {
            direction = (button == 0) ? 1 : -1;
        } else if (mode == sn::ScrollMode::SplitHold) {
            bool topHalf = (clickPos.y < zoneH / 2);
            direction = topHalf ? 1 : -1;
        }

        if (direction != 0) {
            PlayClickSound();
            g_scrollEngine.ClickScroll(direction, cfg.scroll.scroll_amount);
            StartHoldScroll(direction);
        }
    } else {
        StopHoldScroll();
    }
}

// ─────────── Mode 3: Hover logic ───────────
static void HandleZoneHover(POINT clientPos, int /*zoneW*/, int zoneH) {
    auto& cfg = g_configStore.Get();
    sn::ScrollMode mode = sn::ScrollModeFromString(cfg.scroll.mode);
    if (mode != sn::ScrollMode::HoverAuto) return;

    bool topHalf = (clientPos.y < zoneH / 2);
    int dir = topHalf ? 1 : -1;

    if (dir != g_hoverDirection) {
        StopHoverScroll();
        StartHoverScroll(dir);
    }
}

// ─────────── Hold scroll timers ───────────
static void StartHoldScroll(int direction) {
    g_holdDirection = direction;
    g_holdStartTime = GetTickCount64();
    if (!g_holdTimer && g_msgWnd) {
        g_holdTimer = SetTimer(g_msgWnd, TIMER_ID_HOLD, 16, nullptr);
    }
}

static void StopHoldScroll() {
    g_holdDirection = 0;
    g_leftHeld = false;
    g_rightHeld = false;
    g_scrollEngine.Reset();
    if (g_holdTimer && g_msgWnd) {
        KillTimer(g_msgWnd, TIMER_ID_HOLD);
        g_holdTimer = 0;
    }
}

static void StartHoverScroll(int direction) {
    g_hoverDirection = direction;
    if (!g_hoverTimer && g_msgWnd) {
        g_hoverTimer = SetTimer(g_msgWnd, TIMER_ID_HOVER, 16, nullptr);
    }
}

static void StopHoverScroll() {
    g_hoverDirection = 0;
    g_scrollEngine.Reset();
    if (g_hoverTimer && g_msgWnd) {
        KillTimer(g_msgWnd, TIMER_ID_HOVER);
        g_hoverTimer = 0;
    }
}

// ─────────── Sound ───────────
static void PlayClickSound() {
    auto& cfg = g_configStore.Get();
    if (!cfg.sound.enabled) return;
    MessageBeep(MB_OK);
}

// ─────────── FindScrollTarget ───────────
// Finds the scrollable window behind the zone to send WM_MOUSEWHEEL to.
static HWND FindScrollTarget() {
    POINT pos = g_lastOutsidePos;
    if (pos.x < 0 && pos.y < 0) GetCursorPos(&pos);

    HWND zoneHwnd = g_overlay.Handle();

    // First try to get window at cursor position
    HWND top = WindowFromPoint(pos);

    // If we got the zone itself, try nearby positions
    if (top == zoneHwnd || !top) {
        POINT offsets[8] = {
            {pos.x+8,pos.y},{pos.x-8,pos.y},
            {pos.x,pos.y+8},{pos.x,pos.y-8},
            {pos.x+8,pos.y+8},{pos.x-8,pos.y-8},
            {pos.x+8,pos.y-8},{pos.x-8,pos.y+8}
        };
        for (auto& op : offsets) {
            top = WindowFromPoint(op);
            if (top && top != zoneHwnd) { pos = op; break; }
        }
    }

    // If still no valid window, return nullptr
    if (!top || top == zoneHwnd) return nullptr;

    // Convert to client coordinates for child window search
    POINT clientPos = pos;
    ScreenToClient(top, &clientPos);

    // Find the child window at the position (skip transparent/invisible/disabled)
    HWND child = ChildWindowFromPointEx(top, clientPos,
                    CWP_SKIPTRANSPARENT | CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);

    // Return child if valid and different from parent, otherwise return parent
    return (child && child != top) ? child : top;
}

// ─────────── ApplyConfig ───────────
static void ApplyConfig() {
    auto& cfg = g_configStore.Get();
    g_zoneManager.LoadFromConfig(cfg.zone);

    g_stateMachine.SetEnabled(cfg.enabled);

    g_overlay.SetScrollMode(sn::ScrollModeFromString(cfg.scroll.mode));
    g_overlay.SetPosition(cfg.zone.x, cfg.zone.y);
    g_overlay.SetSize(cfg.zone.width, cfg.zone.height);
    g_overlay.SetOpacity(cfg.zone.opacity);
    g_overlay.SetLocked(cfg.zone.locked);
    g_overlay.SetEnabled(g_stateMachine.IsEnabled());

    g_tray.SetEnabled(g_stateMachine.IsEnabled());
    g_tray.SetModeName(cfg.scroll.mode);
    SetStartWithWindows(cfg.start_with_windows);
    UpdateWheelBlockHook(cfg.wheel_block);

    if (g_msgWnd) {
        g_hotkeys.Unregister(g_msgWnd);
        int hotkeyCount = g_hotkeys.Register(g_msgWnd,
            cfg.hotkeys.toggle_enabled,
            cfg.hotkeys.toggle_edit,
            "",
            cfg.hotkeys.toggle_wheel,
            OnHotkey);
        // Log if hotkey registration failed (optional - could add logging system)
        (void)hotkeyCount;
    }
}

// ─────────── Main window events ───────────
static void OnMainWindowEvent(int eventId) {
    auto& cfg = g_configStore.Get();

    switch (eventId) {
    case sn::WinMainWindow::EVT_ZONE_TOGGLED: {
        bool checked = (IsDlgButtonChecked(g_mainWindow.Handle(), 101) == BST_CHECKED);
        cfg.enabled = checked;
        g_stateMachine.SetEnabled(checked);
        g_overlay.SetEnabled(checked);
        g_tray.SetEnabled(checked);
        break;
    }
    case sn::WinMainWindow::EVT_MODE_CHANGED: {
        int idx = (int)SendDlgItemMessage(g_mainWindow.Handle(), 102, CB_GETCURSEL, 0, 0);
        if (idx == 0) cfg.scroll.mode = "click_hold";
        if (idx == 1) cfg.scroll.mode = "split_hold";
        if (idx == 2) cfg.scroll.mode = "hover_auto";
        g_overlay.SetScrollMode(sn::ScrollModeFromString(cfg.scroll.mode));
        g_tray.SetModeName(cfg.scroll.mode);
        StopHoldScroll();
        StopHoverScroll();
        break;
    }
    case sn::WinMainWindow::EVT_OPACITY_CHANGED: {
        int pos = (int)SendDlgItemMessage(g_mainWindow.Handle(), 114, TBM_GETPOS, 0, 0);
        cfg.zone.opacity = pos / 100.0;
        g_overlay.SetOpacity(cfg.zone.opacity);
        break;
    }
    case sn::WinMainWindow::EVT_SAVE: {
        // Already read into cfg by WinMainWindow::ReadControls
        g_configStore.Save(g_configPath);
        StopHoldScroll();
        StopHoverScroll();
        ApplyConfig();
        g_mainWindow.SyncFromConfig(cfg);
        break;
    }
    case sn::WinMainWindow::EVT_RESET: {
        g_configStore.Save(g_configPath);
        StopHoldScroll();
        StopHoverScroll();
        ApplyConfig();
        break;
    }
    case sn::WinMainWindow::EVT_MINIMIZE:
        // X was pressed — window already hidden by WinMainWindow
        break;
    }
}

// ─────────── Config path ───────────
static std::string GetConfigPath() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::filesystem::path p(buf);
    return (p.parent_path() / "config.json").string();
}

// ─────────── Start with Windows ───────────
static void SetStartWithWindows(bool enable) {
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey);
    if (result == ERROR_SUCCESS) {
        if (enable) {
            wchar_t path[MAX_PATH];
            DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
            if (len > 0 && len < MAX_PATH) {
                RegSetValueExW(hKey, L"ScrollNice", 0, REG_SZ,
                    (BYTE*)path, (DWORD)(wcslen(path) + 1) * sizeof(wchar_t));
            }
        } else {
            RegDeleteValueW(hKey, L"ScrollNice");
        }
        RegCloseKey(hKey);
    }
    // If registry access fails, we silently continue - this is not critical functionality
}

// ─────────── Wheel block hook ───────────
static bool OnMouseEvent(POINT, DWORD, MSLLHOOKSTRUCT*) {
    // Wheel events blocked by hook — return true to eat the event
    return true;
}

static void UpdateWheelBlockHook(bool enable) {
    auto& hook = sn::WinMouseHook::Instance();
    if (enable) {
        if (!hook.IsInstalled()) {
            if (!hook.Install(OnMouseEvent)) {
                // Hook installation failed - could log this
                // For now, we'll just continue without the hook
            }
        }
    } else {
        if (hook.IsInstalled()) {
            hook.Uninstall();
        }
    }
}

// ─────────── Hotkeys ───────────
static void OnHotkey(int id) {
    switch (id) {
    case sn::WinHotkeys::HK_TOGGLE_ENABLED:
        g_stateMachine.ToggleEnabled();
        g_tray.SetEnabled(g_stateMachine.IsEnabled());
        g_overlay.SetEnabled(g_stateMachine.IsEnabled());
        {
            auto& cfg = g_configStore.Get();
            cfg.enabled = g_stateMachine.IsEnabled();
            g_mainWindow.SyncFromConfig(cfg);
        }
        break;
    case sn::WinHotkeys::HK_TOGGLE_EDIT:
        g_stateMachine.ToggleEdit();
        g_overlay.SetEditMode(g_stateMachine.IsEditing());
        break;
    case sn::WinHotkeys::HK_TOGGLE_WHEEL: {
        auto& cfg = g_configStore.Get();
        cfg.wheel_block = !cfg.wheel_block;
        UpdateWheelBlockHook(cfg.wheel_block);
        g_configStore.Save(g_configPath);
        g_mainWindow.SyncFromConfig(cfg);
        break;
    }
    }
}

// ─────────── Entry Point ───────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInstance = hInstance;

    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"ScrollNice_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Find existing main window and bring it to front
        HWND existing = FindWindowW(L"ScrollNice_MainWindow", nullptr);
        if (existing) {
            ShowWindow(existing, SW_SHOW);
            SetForegroundWindow(existing);
        } else {
            MessageBoxW(nullptr, L"ScrollNice is already running.", L"ScrollNice", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    g_configPath = GetConfigPath();
    if (!g_configStore.Load(g_configPath)) {
        // Config load failed - use defaults and save
        g_configStore.Save(g_configPath);
    }
    auto& cfg = g_configStore.Get();

    // ─── Hidden message window (for hotkeys + scroll timers) ───
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = MsgWndProc;
    wc.hInstance      = hInstance;
    wc.lpszClassName  = kMsgWindowClass;
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register message window class.", L"ScrollNice Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    g_msgWnd = CreateWindowExW(0, kMsgWindowClass, L"ScrollNice_Msg",
        0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);
    if (!g_msgWnd) {
        MessageBoxW(nullptr, L"Failed to create message window.", L"ScrollNice Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // ─── Main window (visible GUI) ───
    if (!g_mainWindow.Create(hInstance, cfg,
        // onSave callback
        [](const sn::AppConfig& newCfg) {
            g_configStore.Get() = newCfg;
            g_configStore.Save(g_configPath);
            StopHoldScroll();
            StopHoverScroll();
            ApplyConfig();
        },
        // onEvent callback
        OnMainWindowEvent
    )) {
        MessageBoxW(nullptr, L"Failed to create main window.", L"ScrollNice Error", MB_OK | MB_ICONERROR);
        DestroyWindow(g_msgWnd);
        return 1;
    }

    // ─── Zone overlay ───
    if (!g_overlay.Create(hInstance, cfg.zone, OnZoneEvent)) {
        MessageBoxW(nullptr, L"Failed to create zone overlay.", L"ScrollNice Error", MB_OK | MB_ICONERROR);
        g_mainWindow.Destroy();
        DestroyWindow(g_msgWnd);
        return 1;
    }

    // ─── Tray icon ───
    if (!g_tray.Create(g_msgWnd, hInstance, [](sn::WinTray::MenuItem item) {
        if (g_msgWnd) PostMessage(g_msgWnd, WM_COMMAND, MAKEWPARAM(item, 0), 0);
    })) {
        MessageBoxW(nullptr, L"Failed to create tray icon.", L"ScrollNice Error", MB_OK | MB_ICONERROR);
        g_overlay.Destroy();
        g_mainWindow.Destroy();
        DestroyWindow(g_msgWnd);
        return 1;
    }

    // ─── Apply config (registers hotkeys, hooks, shows overlay/tray state) ───
    ApplyConfig();

    // ─── Show main window ───
    g_mainWindow.Show();

    // ─── Message loop ───
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // ─── Cleanup ───
    StopHoldScroll();
    StopHoverScroll();
    sn::WinMouseHook::Instance().Uninstall();
    g_hotkeys.Unregister(g_msgWnd);
    g_tray.Destroy();

    // Save zone position on exit
    auto& exitCfg = g_configStore.Get();
    exitCfg.zone.x = g_overlay.Config().x;
    exitCfg.zone.y = g_overlay.Config().y;
    exitCfg.zone.width = g_overlay.Config().width;
    exitCfg.zone.height = g_overlay.Config().height;
    g_configStore.Save(g_configPath);

    g_overlay.Destroy();
    g_mainWindow.Destroy();
    DestroyWindow(g_msgWnd);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}
