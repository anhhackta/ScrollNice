#include "WinOverlay.h"
#include <dwmapi.h>

namespace sn {

static WinOverlay* g_overlay = nullptr;
static const wchar_t* kOverlayClass = L"ScrollNice_Overlay";

bool WinOverlay::Create(HINSTANCE hInst) {
    g_overlay = this;

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = kOverlayClass;
    wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    // Full-screen overlay
    int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int sx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int sy = GetSystemMetrics(SM_YVIRTUALSCREEN);

    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        kOverlayClass,
        L"ScrollNice",
        WS_POPUP,
        sx, sy, sw, sh,
        nullptr, nullptr, hInst, nullptr
    );

    if (!hwnd_) return false;

    // Set up layered window for per-pixel alpha
    SetLayeredWindowAttributes(hwnd_, RGB(0, 0, 0), 0, LWA_COLORKEY);

    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    return true;
}

void WinOverlay::Destroy() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    g_overlay = nullptr;
}

void WinOverlay::UpdateZones(const std::vector<Zone>& zones, AppState state, const Zone* activeZone) {
    zones_ = zones;
    state_ = state;
    activeZone_ = activeZone;
    if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE);
}

void WinOverlay::SetEditMode(bool edit) {
    editMode_ = edit;
    if (!hwnd_) return;

    LONG exStyle = GetWindowLong(hwnd_, GWL_EXSTYLE);
    if (edit) {
        // Remove transparent so we can receive clicks
        exStyle &= ~WS_EX_TRANSPARENT;
    } else {
        exStyle |= WS_EX_TRANSPARENT;
    }
    SetWindowLong(hwnd_, GWL_EXSTYLE, exStyle);
    if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE);
}

void WinOverlay::Show() {
    if (hwnd_) ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
}

void WinOverlay::Hide() {
    if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
}

void WinOverlay::Paint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);

    // Clear with color key (black = transparent via LWA_COLORKEY)
    RECT rc;
    GetClientRect(hwnd_, &rc);
    HBRUSH bgBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    // Get the virtual screen offset for coordinate mapping
    int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);

    // Draw zones
    for (auto& z : zones_) {
        if (!z.enabled) continue;

        BYTE alpha = 25;  // default ~10% visible
        COLORREF color = RGB(60, 120, 255); // blue

        if (editMode_) {
            alpha = 100;
            color = RGB(255, 180, 0); // orange in edit
        } else if (activeZone_ && z.cfg.id == activeZone_->cfg.id) {
            alpha = (BYTE)(activeAlpha_ * 255);
            color = RGB(80, 200, 120); // green when active
        } else if (state_ == AppState::Hover) {
            alpha = (BYTE)(hoverAlpha_ * 255);
        }

        // We use a non-black color so it doesn't get keyed out
        if (color == RGB(0, 0, 0)) color = RGB(1, 1, 1);

        HBRUSH brush = CreateSolidBrush(color);
        RECT zr = z.rect;
        // Convert screen coords to client coords (offset by virtual screen origin)
        OffsetRect(&zr, -vx, -vy);
        FillRect(hdc, &zr, brush);
        DeleteObject(brush);
    }

    EndPaint(hwnd_, &ps);
}

LRESULT CALLBACK WinOverlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT:
            if (g_overlay) g_overlay->Paint();
            return 0;
        case WM_ERASEBKGND:
            return 1; // we handle erase in Paint
        case WM_NCHITTEST:
            if (g_overlay && !g_overlay->editMode_)
                return HTTRANSPARENT;
            return HTCLIENT;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace sn
