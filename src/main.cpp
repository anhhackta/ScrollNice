// ScrollNice — main application entry point (v2.1: 3-mode redesign)
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <string>
#include <filesystem>
#include <cmath>

#pragma comment(lib, "comctl32.lib")

#include "core/Config.h"
#include "core/Zone.h"
#include "core/ScrollEngine.h"
#include "core/StateMachine.h"
#include "platform/win/WinMouseHook.h"
#include "platform/win/WinOverlay.h"
#include "platform/win/WinTray.h"
#include "platform/win/WinHotkeys.h"
#include "platform/win/WinSettings.h"

#pragma comment(lib, "winmm.lib")

// ─────────── Globals ───────────
static sn::ConfigStore    g_configStore;
static sn::ZoneManager    g_zoneManager;
static sn::ScrollEngine   g_scrollEngine;
static sn::StateMachine   g_stateMachine;
static sn::WinOverlay     g_overlay;
static sn::WinTray        g_tray;
static sn::WinHotkeys     g_hotkeys;
static sn::WinSettings    g_settingsDlg;

static std::string g_configPath;
static HINSTANCE   g_hInstance = nullptr;

// Continuous/hold scroll state
static UINT_PTR g_holdTimer = 0;          // timer for hold & hover scroll
static const UINT_PTR TIMER_ID_HOLD = 201;
static int  g_holdDirection = 0;           // 1=up, -1=down, 0=none
static ULONGLONG g_holdStartTime = 0;
static bool g_leftHeld = false;
static bool g_rightHeld = false;

// Hover scroll state (Mode 3)
static UINT_PTR g_hoverTimer = 0;
static const UINT_PTR TIMER_ID_HOVER = 202;
static int  g_hoverDirection = 0;

// Last known cursor position OUTSIDE the zone window.
// Updated via WM_MOUSELEAVE from the overlay. Used by FindScrollTarget
// to identify the scrollable window the user is trying to scroll.
static POINT g_lastOutsidePos = {-1, -1};

// ─────────── Forward declarations ───────────
static void HandleZoneClick(int button, bool isDown, POINT clickPos, int zoneW, int zoneH);
static void HandleZoneHover(POINT clientPos, int zoneW, int zoneH);
static void StartHoldScroll(int direction);
static void StopHoldScroll();
static void StartHoverScroll(int direction);
static void StopHoverScroll();
static void PlayClickSound();
static void ApplyConfig();
static void OpenSettings();
static void SetStartWithWindows(bool enable);
static std::string GetConfigPath();
static void UpdateWheelBlockHook(bool enable);
static void OnHotkey(int id);
static HWND FindScrollTarget();


// ─────────── Hidden message window ───────────
static const wchar_t* kMsgWindowClass = L"ScrollNice_MsgWnd";
static HWND g_msgWnd = nullptr;

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
            g_tray.SetEnabled(g_stateMachine.IsEnabled());
            g_overlay.SetEnabled(g_stateMachine.IsEnabled());
            break;
        case sn::WinTray::ID_EDIT:
            g_stateMachine.ToggleEdit();
            g_overlay.SetEditMode(g_stateMachine.IsEditing());
            break;
        case sn::WinTray::ID_SETTINGS:
            OpenSettings();
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
        // Identify scroll target BEFORE handling click (zone gains focus on click)
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
        // Set target on first hover (needed for Mode 3 continuous scroll)
        if (!g_scrollEngine.GetTargetHwnd())
            g_scrollEngine.SetTargetHwnd(FindScrollTarget());
        HandleZoneHover(e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    case sn::ZoneEvent::HoverLeave:
        // Record cursor position when leaving zone — used as anchor for FindScrollTarget
        GetCursorPos(&g_lastOutsidePos);
        g_scrollEngine.SetTargetHwnd(nullptr);  // clear target on leave
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

    // Mode 3 (HoverAuto): click has no effect
    if (mode == sn::ScrollMode::HoverAuto) return;

    if (isDown) {
        int direction = 0;

        if (mode == sn::ScrollMode::ClickHold) {
            // Mode 1: L-button=up, R-button=down (for both click and hold)
            direction = (button == 0) ? 1 : -1;
        } else if (mode == sn::ScrollMode::SplitHold) {
            // Mode 2: Top half=up, Bottom half=down (for both click and hold)
            direction = (clickPos.y < zoneH / 2) ? 1 : -1;
        }

        if (direction != 0) {
            // Immediate single-click scroll
            g_scrollEngine.ClickScroll(direction, cfg.scroll.scroll_amount);
            PlayClickSound();

            // Start hold timer for continuous scroll while button held
            if (button == 0) g_leftHeld = true; else g_rightHeld = true;
            g_holdStartTime = GetTickCount64() + 300; // 300ms delay before continuous starts
            StartHoldScroll(direction);
        }
    } else {
        // Button released
        if (button == 0) g_leftHeld = false; else g_rightHeld = false;
        if (!g_leftHeld && !g_rightHeld) {
            StopHoldScroll();
        }
    }
}

// ─────────── Mode 3: Hover logic ───────────
static void HandleZoneHover(POINT clientPos, int zoneW, int zoneH) {
    auto& cfg = g_configStore.Get();
    if (sn::ScrollModeFromString(cfg.scroll.mode) != sn::ScrollMode::HoverAuto) return;

    int newDir = (clientPos.y < zoneH / 2) ? 1 : -1;
    if (newDir != g_hoverDirection) {
        g_hoverDirection = newDir;
        if (newDir != 0) {
            StartHoverScroll(newDir);
        }
    }
}

// ─────────── Hold & Hover timers ───────────
static void StartHoldScroll(int direction) {
    g_holdDirection = direction;
    g_scrollEngine.Reset();
    if (!g_holdTimer && g_msgWnd)
        g_holdTimer = SetTimer(g_msgWnd, TIMER_ID_HOLD, 16, nullptr);
}

static void StopHoldScroll() {
    if (g_holdTimer && g_msgWnd) {
        KillTimer(g_msgWnd, TIMER_ID_HOLD);
        g_holdTimer = 0;
    }
    g_holdDirection = 0;
    g_scrollEngine.Reset();
}

static void StartHoverScroll(int direction) {
    g_hoverDirection = direction;
    g_scrollEngine.Reset();
    if (!g_hoverTimer && g_msgWnd)
        g_hoverTimer = SetTimer(g_msgWnd, TIMER_ID_HOVER, 16, nullptr);
}

static void StopHoverScroll() {
    if (g_hoverTimer && g_msgWnd) {
        KillTimer(g_msgWnd, TIMER_ID_HOVER);
        g_hoverTimer = 0;
    }
    g_hoverDirection = 0;
    g_scrollEngine.Reset();
}

// ─────────── Sound ───────────
static void PlayClickSound() {
    auto& cfg = g_configStore.Get();
    if (!cfg.sound.enabled) return;
    if (cfg.sound.click_sound.empty()) {
        MessageBeep(0xFFFFFFFF);
    } else {
        std::wstring wp(cfg.sound.click_sound.begin(), cfg.sound.click_sound.end());
        PlaySoundW(wp.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    }
}

// ─────────── FindScrollTarget ───────────
// Finds the scrollable window under the cursor to send WM_MOUSEWHEEL to.
//
// Why needed:
//   When the user clicks inside the zone, the zone window receives focus.
//   SendInput(MOUSEEVENTF_WHEEL) sends scroll to the focused window (=zone).
//   By using PostMessage(targetHwnd, WM_MOUSEWHEEL) we bypass focus entirely.
//
// Algorithm:
//   1. Use g_lastOutsidePos (set when cursor leaves the zone)
//   2. WindowFromPoint — gets top-level window at that screen position
//   3. ChildWindowFromPointEx — drills into children, skipping invisible/disabled
//   4. Skip the zone hwnd itself
static HWND FindScrollTarget() {
    POINT pos = g_lastOutsidePos;

    // If no outside position recorded yet, use current cursor position
    if (pos.x < 0 && pos.y < 0) GetCursorPos(&pos);

    HWND zoneHwnd = g_overlay.Handle();

    // Get window at cursor position, excluding our own zone overlay
    HWND top = WindowFromPoint(pos);
    if (top == zoneHwnd || !top) {
        // Try 4px offsets to step just outside the zone boundary
        POINT offsets[4] = {{pos.x+4,pos.y},{pos.x-4,pos.y},
                            {pos.x,pos.y+4},{pos.x,pos.y-4}};
        for (auto& op : offsets) {
            top = WindowFromPoint(op);
            if (top && top != zoneHwnd) { pos = op; break; }
        }
    }
    if (!top || top == zoneHwnd) return nullptr;

    // Drill into child windows to find the innermost scrollable control
    POINT clientPos = pos;
    ScreenToClient(top, &clientPos);
    HWND child = ChildWindowFromPointEx(top, clientPos,
                    CWP_SKIPTRANSPARENT | CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);

    return (child && child != top) ? child : top;
}

static void ApplyConfig() {
    auto& cfg = g_configStore.Get();
    g_zoneManager.LoadFromConfig(cfg.zone);
    g_overlay.SetScrollMode(sn::ScrollModeFromString(cfg.scroll.mode));
    g_overlay.SetPosition(cfg.zone.x, cfg.zone.y);
    g_overlay.SetSize(cfg.zone.width, cfg.zone.height);
    g_overlay.SetOpacity(cfg.zone.opacity);
    g_overlay.SetLocked(cfg.zone.locked);
    g_overlay.SetEnabled(g_stateMachine.IsEnabled());
    g_stateMachine.SetEnabled(cfg.enabled);
    g_tray.SetEnabled(g_stateMachine.IsEnabled());
    g_tray.SetModeName(cfg.scroll.mode);   // update tray tooltip mode
    SetStartWithWindows(cfg.start_with_windows);
    UpdateWheelBlockHook(cfg.wheel_block);

    if (g_msgWnd) {
        g_hotkeys.Unregister(g_msgWnd);
        g_hotkeys.Register(g_msgWnd,
            cfg.hotkeys.toggle_enabled,
            cfg.hotkeys.toggle_edit,
            "",
            cfg.hotkeys.toggle_wheel,
            OnHotkey);
    }
}

static void OpenSettings() {
    auto& cfg = g_configStore.Get();
    // Sync current zone position back to config
    cfg.zone.x = g_overlay.Config().x;
    cfg.zone.y = g_overlay.Config().y;
    cfg.zone.width = g_overlay.Config().width;
    cfg.zone.height = g_overlay.Config().height;

    g_settingsDlg.Show(g_hInstance, g_msgWnd, cfg, [](const sn::AppConfig& newCfg) {
        g_configStore.Get() = newCfg;
        g_configStore.Save(g_configPath);
        StopHoldScroll();
        StopHoverScroll();
        ApplyConfig();
    });
}

static void SetStartWithWindows(bool enable) {
    HKEY hKey;
    const wchar_t* runKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyExW(HKEY_CURRENT_USER, runKey, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            RegSetValueExW(hKey, L"ScrollNice", 0, REG_SZ, (BYTE*)exePath,
                          (DWORD)(wcslen(exePath) + 1) * sizeof(wchar_t));
        } else {
            RegDeleteValueW(hKey, L"ScrollNice");
        }
        RegCloseKey(hKey);
    }
}

static std::string GetConfigPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    p = p.parent_path() / "config.json";
    return p.string();
}

// ─────────── Mouse hook (wheel block) ───────────
static bool OnMouseEvent(POINT, DWORD mouseMsg, MSLLHOOKSTRUCT*) {
    if (mouseMsg == WM_MOUSEWHEEL || mouseMsg == WM_MOUSEHWHEEL) {
        if (g_configStore.Get().wheel_block) {
            if (GetAsyncKeyState(VK_MENU) & 0x8000) return false;
            return true;
        }
    }
    return false;
}

static void UpdateWheelBlockHook(bool enable) {
    auto& hook = sn::WinMouseHook::Instance();
    if (enable) {
        if (!hook.IsInstalled())
            hook.Install(OnMouseEvent);
    } else {
        if (hook.IsInstalled())
            hook.Uninstall();
    }
}

// ─────────── Hotkeys ───────────
static void OnHotkey(int id) {
    switch (id) {
    case sn::WinHotkeys::HK_TOGGLE_ENABLED:
        g_stateMachine.ToggleEnabled();
        g_tray.SetEnabled(g_stateMachine.IsEnabled());
        g_overlay.SetEnabled(g_stateMachine.IsEnabled());
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
        break;
    }
    }
}

// ─────────── Entry Point ───────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInstance = hInstance;

    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"ScrollNice_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"ScrollNice is already running.", L"ScrollNice", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    g_configPath = GetConfigPath();
    if (!g_configStore.Load(g_configPath)) {
        g_configStore.Save(g_configPath);
    }
    auto& cfg = g_configStore.Get();

    // Message window
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = MsgWndProc;
    wc.hInstance      = hInstance;
    wc.lpszClassName  = kMsgWindowClass;
    RegisterClassExW(&wc);
    g_msgWnd = CreateWindowExW(0, kMsgWindowClass, L"ScrollNice_Msg",
        0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);

    // Create zone overlay window
    g_overlay.Create(hInstance, cfg.zone, OnZoneEvent);
    g_overlay.SetScrollMode(sn::ScrollModeFromString(cfg.scroll.mode));

    // Tray icon
    g_tray.Create(g_msgWnd, hInstance, [](sn::WinTray::MenuItem item) {
        if (g_msgWnd) PostMessage(g_msgWnd, WM_COMMAND, MAKEWPARAM(item, 0), 0);
    });

    // Apply config (hotkeys, hooks, overlay state)
    ApplyConfig();

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    StopHoldScroll();
    StopHoverScroll();
    sn::WinMouseHook::Instance().Uninstall();
    g_hotkeys.Unregister(g_msgWnd);
    g_tray.Destroy();

    // Save zone position
    auto& exitCfg = g_configStore.Get();
    exitCfg.zone.x = g_overlay.Config().x;
    exitCfg.zone.y = g_overlay.Config().y;
    exitCfg.zone.width = g_overlay.Config().width;
    exitCfg.zone.height = g_overlay.Config().height;
    g_configStore.Save(g_configPath);

    g_overlay.Destroy();
    DestroyWindow(g_msgWnd);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}
