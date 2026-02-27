// ScrollNice — main application entry point (v2: click-to-scroll)
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

// Continuous scroll state
static UINT_PTR g_continuousTimer = 0;
static const UINT_PTR TIMER_ID_CONTINUOUS = 200;
static int  g_continuousDirection = 0;
static DWORD g_holdStartTime = 0;
static bool g_leftHeld = false;
static bool g_rightHeld = false;

// ─────────── Forward declarations ───────────
static void HandleZoneClick(int button, bool isDown, POINT clickPos, int zoneW, int zoneH);
static void StartContinuousScroll(int direction);
static void StopContinuousScroll();
static void PlayClickSound();
static void ApplyConfig();
static void OpenSettings();
static void SetStartWithWindows(bool enable);
static std::string GetConfigPath();

// ─────────── Hidden message window ───────────
static const wchar_t* kMsgWindowClass = L"ScrollNice_MsgWnd";
static HWND g_msgWnd = nullptr;

static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_HOTKEY:
        g_hotkeys.HandleMessage(wParam);
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ID_CONTINUOUS) {
            auto& cfg = g_configStore.Get();
            double holdSec = (GetTickCount() - g_holdStartTime) / 1000.0;
            g_scrollEngine.ContinuousScrollTick(g_continuousDirection,
                cfg.scroll.continuous_speed, cfg.scroll.continuous_accel, holdSec);
        }
        return 0;

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
        HandleZoneClick(0, true, e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    case sn::ZoneEvent::LeftClickUp:
        HandleZoneClick(0, false, e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    case sn::ZoneEvent::RightClickDown:
        HandleZoneClick(1, true, e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    case sn::ZoneEvent::RightClickUp:
        HandleZoneClick(1, false, e.clickPos, e.zoneWidth, e.zoneHeight);
        break;
    default:
        break;
    }
}

// ─────────── Click handling logic ───────────
static void HandleZoneClick(int button, bool isDown, POINT clickPos, int zoneW, int zoneH) {
    auto& cfg = g_configStore.Get();
    sn::ScrollMode mode = sn::ScrollModeFromString(cfg.scroll.mode);

    if (mode == sn::ScrollMode::Continuous) {
        // Hold-to-scroll
        if (isDown) {
            int dir = (button == 0) ? 1 : -1; // left=up, right=down
            g_continuousDirection = dir;
            g_holdStartTime = GetTickCount();
            if (button == 0) g_leftHeld = true; else g_rightHeld = true;
            StartContinuousScroll(dir);
            PlayClickSound();
        } else {
            if (button == 0) g_leftHeld = false; else g_rightHeld = false;
            if (!g_leftHeld && !g_rightHeld) StopContinuousScroll();
        }
        return;
    }

    // Click-based modes: scroll on mousedown
    if (!isDown) return;

    int direction = 0; // 1=up, -1=down

    switch (mode) {
    case sn::ScrollMode::ClickLR:
        direction = (button == 0) ? 1 : -1;
        break;
    case sn::ScrollMode::ClickRL:
        direction = (button == 0) ? -1 : 1;
        break;
    case sn::ScrollMode::SplitTB:
        direction = (clickPos.y < zoneH / 2) ? 1 : -1;
        break;
    case sn::ScrollMode::SplitLR:
        direction = (clickPos.x < zoneW / 2) ? 1 : -1;
        break;
    default:
        break;
    }

    if (direction != 0) {
        g_scrollEngine.ClickScroll(direction, cfg.scroll.scroll_amount);
        PlayClickSound();
    }
}

// ─────────── Helpers ───────────
static void StartContinuousScroll(int direction) {
    g_continuousDirection = direction;
    g_scrollEngine.Reset();
    if (!g_continuousTimer && g_msgWnd)
        g_continuousTimer = SetTimer(g_msgWnd, TIMER_ID_CONTINUOUS, 16, nullptr);
}

static void StopContinuousScroll() {
    if (g_continuousTimer && g_msgWnd) {
        KillTimer(g_msgWnd, TIMER_ID_CONTINUOUS);
        g_continuousTimer = 0;
    }
    g_scrollEngine.Reset();
}

static void PlayClickSound() {
    auto& cfg = g_configStore.Get();
    if (!cfg.sound.enabled) return;

    if (cfg.sound.click_sound.empty()) {
        // Built-in system sound
        MessageBeep(0xFFFFFFFF);
    } else {
        std::wstring wp(cfg.sound.click_sound.begin(), cfg.sound.click_sound.end());
        PlaySoundW(wp.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    }
}

static void ApplyConfig() {
    auto& cfg = g_configStore.Get();
    g_zoneManager.LoadFromConfig(cfg.zone);
    g_overlay.SetScrollMode(sn::ScrollModeFromString(cfg.scroll.mode));
    g_overlay.SetPosition(cfg.zone.x, cfg.zone.y);
    g_overlay.SetSize(cfg.zone.width, cfg.zone.height);
    g_overlay.SetOpacity(cfg.zone.opacity);
    g_overlay.SetLocked(cfg.zone.locked);
    g_overlay.SetEnabled(cfg.enabled);
    g_stateMachine.SetEnabled(cfg.enabled);
    g_tray.SetEnabled(cfg.enabled);
    SetStartWithWindows(cfg.start_with_windows);
}

static void OpenSettings() {
    auto& cfg = g_configStore.Get();

    // Save current zone position (may have been dragged)
    cfg.zone.x = g_overlay.Config().x;
    cfg.zone.y = g_overlay.Config().y;
    cfg.zone.width = g_overlay.Config().width;
    cfg.zone.height = g_overlay.Config().height;

    g_settingsDlg.Show(g_hInstance, g_msgWnd, cfg, [](const sn::AppConfig& newCfg) {
        g_configStore.Get() = newCfg;
        g_configStore.Save(g_configPath);
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

// ─────────── Mouse hook for wheel blocking ───────────
static bool OnMouseEvent(POINT pt, DWORD mouseMsg, MSLLHOOKSTRUCT* /*data*/) {
    if (mouseMsg == WM_MOUSEWHEEL || mouseMsg == WM_MOUSEHWHEEL) {
        if (g_configStore.Get().wheel_block) {
            // Bypass when Alt held
            if (GetAsyncKeyState(VK_MENU) & 0x8000) return false;
            return true; // block wheel
        }
    }
    return false;
}

// ─────────── Hotkey handler ───────────
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
        break;
    }
    }
}

// ─────────── Entry Point ───────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInstance = hInstance;

    // Single instance
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"ScrollNice_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"ScrollNice is already running.", L"ScrollNice", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    // Init common controls for settings dialog
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    // 1) Load config
    g_configPath = GetConfigPath();
    if (!g_configStore.Load(g_configPath)) {
        g_configStore.Save(g_configPath);
    }
    auto& cfg = g_configStore.Get();

    // 2) Create hidden message window
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = MsgWndProc;
    wc.hInstance      = hInstance;
    wc.lpszClassName  = kMsgWindowClass;
    RegisterClassExW(&wc);
    g_msgWnd = CreateWindowExW(0, kMsgWindowClass, L"ScrollNice_Msg",
        0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);

    // 3) Init modules
    g_zoneManager.LoadFromConfig(cfg.zone);
    g_stateMachine.SetEnabled(cfg.enabled);

    // 4) Create zone overlay window
    g_overlay.Create(hInstance, cfg.zone, OnZoneEvent);
    g_overlay.SetScrollMode(sn::ScrollModeFromString(cfg.scroll.mode));

    // 5) Tray icon
    g_tray.Create(g_msgWnd, hInstance, [](sn::WinTray::MenuItem item) {
        if (g_msgWnd) PostMessage(g_msgWnd, WM_COMMAND, MAKEWPARAM(item, 0), 0);
    });

    // 6) Hotkeys
    g_hotkeys.Register(g_msgWnd,
        cfg.hotkeys.toggle_enabled,
        cfg.hotkeys.toggle_edit,
        "", // no zone toggle needed
        cfg.hotkeys.toggle_wheel,
        OnHotkey);

    // 7) Mouse hook (for wheel blocking)
    if (cfg.wheel_block) {
        sn::WinMouseHook::Instance().Install(OnMouseEvent);
    }

    // 8) Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    StopContinuousScroll();
    sn::WinMouseHook::Instance().Uninstall();
    g_hotkeys.Unregister(g_msgWnd);
    g_tray.Destroy();
    g_overlay.Destroy();

    // Save zone position on exit
    auto& exitCfg = g_configStore.Get();
    exitCfg.zone.x = g_overlay.Config().x;
    exitCfg.zone.y = g_overlay.Config().y;
    exitCfg.zone.width = g_overlay.Config().width;
    exitCfg.zone.height = g_overlay.Config().height;
    g_configStore.Save(g_configPath);

    DestroyWindow(g_msgWnd);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}
