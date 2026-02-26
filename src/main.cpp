// ScrollNice — main application entry point
// Wires together: Config, ZoneManager, ScrollEngine, StateMachine,
// WinMouseHook, WinInputInjector, WinOverlay, WinTray, WinHotkeys.

#include <windows.h>
#include <string>
#include <filesystem>

#include "core/Config.h"
#include "core/Zone.h"
#include "core/ScrollEngine.h"
#include "core/StateMachine.h"
#include "platform/win/WinMouseHook.h"
#include "platform/win/WinInputInjector.h"
#include "platform/win/WinOverlay.h"
#include "platform/win/WinTray.h"
#include "platform/win/WinHotkeys.h"

// ─────────── Globals ───────────
static sn::ConfigStore    g_configStore;
static sn::ZoneManager    g_zoneManager;
static sn::ScrollEngine   g_scrollEngine;
static sn::StateMachine   g_stateMachine;
static sn::WinInputInjector g_injector;
static sn::WinOverlay     g_overlay;
static sn::WinTray        g_tray;
static sn::WinHotkeys     g_hotkeys;

static UINT_PTR g_scrollTimer = 0;
static const UINT_PTR TIMER_ID_SCROLL = 100;
static POINT g_lastCursorPos = {};
static std::string g_configPath;
static bool g_wheelBlockEnabled = false;
static const sn::Zone* g_currentZone = nullptr;

// ─────────── Forward declarations ───────────
static void StartScrollTimer(HWND hwnd);
static void StopScrollTimer(HWND hwnd);
static void UpdateOverlay();
static std::string GetConfigPath();
static bool IsFullscreenApp();

// ─────────── Hidden message-only window ───────────
static const wchar_t* kMsgWindowClass = L"ScrollNice_MsgWnd";

static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_HOTKEY:
            g_hotkeys.HandleMessage(wParam);
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_ID_SCROLL) {
                // Scroll tick: read current cursor position, compute delta
                POINT pt;
                GetCursorPos(&pt);
                int dy = pt.y - g_lastCursorPos.y;
                g_lastCursorPos = pt;

                double dt = g_scrollEngine.TickMs() / 1000.0;
                int wheelUnits = g_scrollEngine.Update(dy, dt);
                if (wheelUnits != 0) {
                    g_injector.SendWheel(wheelUnits);
                }
            }
            return 0;

        case WM_COMMAND:
            // Tray menu
            switch (LOWORD(wParam)) {
                case sn::WinTray::ID_TOGGLE:
                    g_stateMachine.ToggleEnabled();
                    g_tray.SetEnabled(g_stateMachine.IsEnabled());
                    UpdateOverlay();
                    break;
                case sn::WinTray::ID_EDIT:
                    g_stateMachine.ToggleEdit();
                    g_overlay.SetEditMode(g_stateMachine.IsEditing());
                    UpdateOverlay();
                    break;
                case sn::WinTray::ID_QUIT:
                    PostQuitMessage(0);
                    break;
            }
            return 0;

        default:
            // Check for tray messages
            if (msg == sn::WinTray::WM_TRAYICON) {
                g_tray.HandleMessage(msg, wParam, lParam);
                return 0;
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// ─────────── Mouse hook callback ───────────
static bool OnMouseEvent(POINT pt, DWORD mouseMsg, MSLLHOOKSTRUCT* /*data*/) {
    auto& cfg = g_configStore.Get();

    // Wheel blocking
    if (mouseMsg == WM_MOUSEWHEEL || mouseMsg == WM_MOUSEHWHEEL) {
        if (g_wheelBlockEnabled && cfg.wheel_block_mode != "off") {
            // Check bypass modifier (Alt)
            bool bypass = false;
            if (cfg.bypass_modifier == "Alt" && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
                bypass = true;
            }
            if (!bypass) {
                bool inZone = g_zoneManager.HitTest(pt) != nullptr;
                if (cfg.wheel_block_mode == "global") return true;
                if (cfg.wheel_block_mode == "outside_zone_only" && !inZone) return true;
                if (cfg.wheel_block_mode == "inside_zone_only" && inZone) return true;
            }
        }
        return false; // don't eat wheel if no block rule matched
    }

    // Only process mouse move
    if (mouseMsg != WM_MOUSEMOVE) return false;

    // Skip if disabled or editing
    if (!g_stateMachine.IsEnabled() || g_stateMachine.IsEditing()) return false;

    // Auto-suspend in fullscreen
    if (cfg.exclusions.auto_suspend_fullscreen && IsFullscreenApp()) return false;

    // Hit test
    const sn::Zone* zone = g_zoneManager.HitTest(pt);

    if (zone) {
        if (g_stateMachine.State() == sn::AppState::Idle) {
            g_stateMachine.OnEnterZone();
            g_currentZone = zone;
            g_lastCursorPos = pt;
            UpdateOverlay();
        }
        // Transition hover → active on movement
        if (g_stateMachine.State() == sn::AppState::Hover) {
            int dy = pt.y - g_lastCursorPos.y;
            if (std::abs(dy) >= cfg.engine.dead_zone_px) {
                g_stateMachine.OnMovementInZone();
            }
        }
        g_lastCursorPos = pt;
    } else {
        if (g_stateMachine.State() == sn::AppState::Hover ||
            g_stateMachine.State() == sn::AppState::Active) {
            g_stateMachine.OnLeaveZone();
            g_currentZone = nullptr;
            UpdateOverlay();
        }
    }

    return false; // never eat mouse move
}

// ─────────── Hotkey callback ───────────
static void OnHotkey(int id) {
    switch (id) {
        case sn::WinHotkeys::HK_TOGGLE_ENABLED:
            g_stateMachine.ToggleEnabled();
            g_tray.SetEnabled(g_stateMachine.IsEnabled());
            UpdateOverlay();
            break;
        case sn::WinHotkeys::HK_TOGGLE_EDIT:
            g_stateMachine.ToggleEdit();
            g_overlay.SetEditMode(g_stateMachine.IsEditing());
            UpdateOverlay();
            break;
        case sn::WinHotkeys::HK_TOGGLE_ZONE:
            g_zoneManager.ToggleNextZone();
            UpdateOverlay();
            break;
        case sn::WinHotkeys::HK_TOGGLE_WHEEL:
            g_wheelBlockEnabled = !g_wheelBlockEnabled;
            break;
    }
}

// ─────────── State change callback ───────────
static void OnStateChange(sn::AppState oldState, sn::AppState newState) {
    // Find our message window for timer management
    HWND hw = FindWindowW(kMsgWindowClass, nullptr);

    if (newState == sn::AppState::Active) {
        StartScrollTimer(hw);
    } else {
        if (oldState == sn::AppState::Active) {
            StopScrollTimer(hw);
            g_scrollEngine.Reset();
        }
    }
}

// ─────────── Helpers ───────────
static void StartScrollTimer(HWND hwnd) {
    if (g_scrollTimer == 0 && hwnd) {
        g_scrollTimer = SetTimer(hwnd, TIMER_ID_SCROLL, g_scrollEngine.TickMs(), nullptr);
    }
}

static void StopScrollTimer(HWND hwnd) {
    if (g_scrollTimer != 0 && hwnd) {
        KillTimer(hwnd, TIMER_ID_SCROLL);
        g_scrollTimer = 0;
    }
}

static void UpdateOverlay() {
    g_overlay.UpdateZones(g_zoneManager.Zones(), g_stateMachine.State(), g_currentZone);
}

static std::string GetConfigPath() {
    // Look for config.json next to exe (portable)
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    p = p.parent_path() / "config.json";
    return p.string();
}

static bool IsFullscreenApp() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;

    RECT rc;
    GetWindowRect(fg, &rc);
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    // Simple heuristic: if foreground window covers entire primary screen
    return (rc.left <= 0 && rc.top <= 0 &&
            rc.right >= sw && rc.bottom >= sh);
}

// ─────────── Entry Point ───────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Prevent multiple instances
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"ScrollNice_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"ScrollNice is already running.", L"ScrollNice", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    // 1) Load config
    g_configPath = GetConfigPath();
    if (!g_configStore.Load(g_configPath)) {
        // Config didn't exist or was invalid — save defaults
        g_configStore.Save(g_configPath);
    }
    auto& cfg = g_configStore.Get();

    // 2) Init modules
    g_scrollEngine.Configure(cfg.engine);
    g_injector.SetMaxEventsPerSec(cfg.engine.max_events_per_sec);
    g_zoneManager.LoadFromConfig(cfg.zones);

    // 3) State machine
    g_stateMachine.SetCallback(OnStateChange);
    g_stateMachine.SetEnabled(cfg.enabled);

    // 4) Create hidden message window (for hotkeys + timers)
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = MsgWndProc;
    wc.hInstance      = hInstance;
    wc.lpszClassName  = kMsgWindowClass;
    RegisterClassExW(&wc);

    HWND msgWnd = CreateWindowExW(0, kMsgWindowClass, L"ScrollNice_Msg",
        0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);

    // 5) Overlay window
    g_overlay.Create(hInstance);
    UpdateOverlay();

    // 6) Tray icon
    g_tray.Create(msgWnd, hInstance, [](sn::WinTray::MenuItem item) {
        // Forward to WM_COMMAND
        HWND hw = FindWindowW(kMsgWindowClass, nullptr);
        if (hw) PostMessage(hw, WM_COMMAND, MAKEWPARAM(item, 0), 0);
    });

    // 7) Hotkeys
    g_hotkeys.Register(msgWnd,
        cfg.hotkeys.toggle_enabled,
        cfg.hotkeys.toggle_edit,
        cfg.hotkeys.toggle_zone,
        cfg.hotkeys.toggle_wheel,
        OnHotkey);

    // 8) Mouse hook
    sn::WinMouseHook::Instance().Install(OnMouseEvent);

    // 9) Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    sn::WinMouseHook::Instance().Uninstall();
    g_hotkeys.Unregister(msgWnd);
    g_tray.Destroy();
    g_overlay.Destroy();
    DestroyWindow(msgWnd);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return 0;
}
