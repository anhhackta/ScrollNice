#include "WinOverlay.h"
#include <windowsx.h>
#include <algorithm>

namespace sn {

static WinOverlay* g_zoneWnd = nullptr;
static const wchar_t* kZoneClass = L"ScrollNice_Zone";

// ─────── Helpers ───────
static COLORREF HexToColorRef(const std::string& hex) {
    if (hex.size() < 7 || hex[0] != '#') return RGB(52, 152, 219);
    int r = std::stoi(hex.substr(1, 2), nullptr, 16);
    int g = std::stoi(hex.substr(3, 2), nullptr, 16);
    int b = std::stoi(hex.substr(5, 2), nullptr, 16);
    return RGB(r, g, b);
}

// ─────── GDI cache ───────
void WinOverlay::InitGDI() {
    // Fonts
    fontBold_  = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    fontSmall_ = CreateFontW(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    // Pens
    borderPen_ = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    editPen_   = CreatePen(PS_SOLID, 3, RGB(255, 140, 0));
    dashPen_   = CreatePen(PS_DASH,  1, RGB(255, 255, 255));

    // Brushes (Mode 3 tint areas)
    topBrush_  = CreateSolidBrush(RGB(40, 80, 160));
    botBrush_  = CreateSolidBrush(RGB(160, 40, 40));
    editBrush_ = CreateSolidBrush(RGB(255, 140, 0));
}

void WinOverlay::DestroyGDI() {
    auto del = [](auto*& h) { if (h) { DeleteObject(h); h = nullptr; } };
    del(fontBold_);
    del(fontSmall_);
    del(borderPen_);
    del(editPen_);
    del(dashPen_);
    del(glowPen_);
    del(topBrush_);
    del(botBrush_);
    del(editBrush_);
    del(hoverBrush_);
}

// ─────── Create / Destroy ───────
bool WinOverlay::Create(HINSTANCE hInst, const ZoneConfig& cfg, ZoneEventCallback cb) {
    g_zoneWnd = this;
    cfg_      = cfg;
    callback_ = cb;

    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = kZoneClass;
    wc.hCursor        = LoadCursor(nullptr, IDC_HAND);
    wc.hbrBackground  = nullptr;
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        kZoneClass, L"ScrollNice Zone",
        WS_POPUP,
        cfg_.x, cfg_.y, cfg_.width, cfg_.height,
        nullptr, nullptr, hInst, nullptr
    );
    if (!hwnd_) return false;

    InitGDI();
    SetOpacity(cfg_.opacity);
    // Don't show here — let the caller (ApplyConfig) control visibility
    // based on whether the app starts enabled or disabled.
    return true;

}

void WinOverlay::Destroy() {
    DestroyGDI();
    if (coverBmp_) { DeleteObject(coverBmp_); coverBmp_ = nullptr; }
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    g_zoneWnd = nullptr;
}

// ─────── Setters ───────
void WinOverlay::SetPosition(int x, int y) {
    cfg_.x = x; cfg_.y = y;
    if (hwnd_) SetWindowPos(hwnd_, nullptr, x, y, 0, 0,
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void WinOverlay::SetSize(int w, int h) {
    cfg_.width = w; cfg_.height = h;
    if (hwnd_) SetWindowPos(hwnd_, nullptr, 0, 0, w, h,
                             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    Redraw();
}

void WinOverlay::SetLocked(bool locked) { cfg_.locked = locked; Redraw(); }
void WinOverlay::SetEditMode(bool edit) { editMode_ = edit; Redraw(); }
void WinOverlay::SetScrollMode(ScrollMode mode) { mode_ = mode; Redraw(); }

void WinOverlay::SetOpacity(double alpha) {
    cfg_.opacity = alpha;
    if (hwnd_) {
        BYTE a = (BYTE)(alpha * 255);
        if (a < 20) a = 20;
        SetLayeredWindowAttributes(hwnd_, 0, a, LWA_ALPHA);
    }
}

void WinOverlay::SetEnabled(bool enabled) {
    enabled_ = enabled;
    if (hwnd_) ShowWindow(hwnd_, enabled ? SW_SHOWNOACTIVATE : SW_HIDE);
}

void WinOverlay::SetCoverImage(const std::string& path) {
    cfg_.cover_image = path;
    if (coverBmp_) { DeleteObject(coverBmp_); coverBmp_ = nullptr; }
    if (!path.empty()) {
        std::wstring wp(path.begin(), path.end());
        coverBmp_ = (HBITMAP)LoadImageW(nullptr, wp.c_str(), IMAGE_BITMAP,
                                          cfg_.width, cfg_.height, LR_LOADFROMFILE);
    }
    Redraw();
}

void WinOverlay::Redraw() { if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE); }
void WinOverlay::Show()   { if (hwnd_) ShowWindow(hwnd_, SW_SHOWNOACTIVATE); }
void WinOverlay::Hide()   { if (hwnd_) ShowWindow(hwnd_, SW_HIDE); }

// ─────── Painting (DOUBLE-BUFFERED) ───────
void WinOverlay::Paint(HDC hdc, int w, int h) {
    // ── off-screen buffer ──
    HDC     memDC  = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    // ── Background with gradient ──
    COLORREF bgColor = editMode_ ? RGB(255, 165, 0) : HexToColorRef(cfg_.color);

    if (coverBmp_ && !editMode_) {
        HDC tmpDC   = CreateCompatibleDC(memDC);
        HBITMAP old = (HBITMAP)SelectObject(tmpDC, coverBmp_);
        StretchBlt(memDC, 0, 0, w, h, tmpDC, 0, 0, w, h, SRCCOPY);
        SelectObject(tmpDC, old);
        DeleteDC(tmpDC);
    } else {
        // Create gradient background
        RECT rc = {0, 0, w, h};
        HBRUSH bgBrush = CreateSolidBrush(bgColor);
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);

        // Add subtle gradient overlay
        if (!editMode_) {
            HBRUSH gradBrush = CreateSolidBrush(RGB(255, 255, 255));
            RECT topGrad = {0, 0, w, h/3};
            FillRect(memDC, &topGrad, gradBrush);
            DeleteObject(gradBrush);
        }
    }

    // ── Mode visuals ──
    DrawModeVisuals(memDC, w, h);

    // ── Enhanced rounded border with glow ──
    HPEN   pen      = editMode_ ? editPen_ : borderPen_;
    HPEN   oldPen   = (HPEN)SelectObject(memDC, pen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBrush  = (HBRUSH)SelectObject(memDC, nullBrush);

    // Draw outer glow
    if (!editMode_ && enabled_) {
        HPEN glowPen = CreatePen(PS_SOLID, 4, RGB(59, 130, 246));
        HPEN oldGlow = (HPEN)SelectObject(memDC, glowPen);
        RoundRect(memDC, 2, 2, w-2, h-2, 18, 18);
        SelectObject(memDC, oldGlow);
        DeleteObject(glowPen);
    }

    RoundRect(memDC, 0, 0, w, h, 18, 18);
    SelectObject(memDC, oldPen);
    SelectObject(memDC, oldBrush);

    // ── Lock indicator ──
    if (cfg_.locked && !editMode_) {
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(255, 255, 255));
        HFONT old = (HFONT)SelectObject(memDC, fontSmall_);
        RECT  lockR = {w - 28, 4, w - 4, 24};
        DrawTextW(memDC, L"🔒", -1, &lockR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(memDC, old);
    }

    // ── Edit mode resize grip (3 diagonal dots, Win-style) ──
    if (editMode_) {
        DrawResizeGrip(memDC, w, h);
    }

    // ── Blit to screen ──
    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

void WinOverlay::DrawResizeGrip(HDC hdc, int w, int h) {
    // Enhanced resize grip with better visual design
    const int DOT = 4, GAP = 5;
    HBRUSH wBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH dBrush = CreateSolidBrush(RGB(100, 100, 100));
    HBRUSH sBrush = CreateSolidBrush(RGB(59, 130, 246));

    for (int i = 0; i < 3; i++) {
        int ox = w - 6 - i * (DOT + GAP);
        int oy = h - 6 - i * (DOT + GAP);

        // Shadow
        RECT rS = {ox + 2, oy + 2, ox + DOT + 2, oy + DOT + 2};
        FillRect(hdc, &rS, dBrush);

        // Main dot
        RECT r1 = {ox, oy, ox + DOT, oy + DOT};
        FillRect(hdc, &r1, wBrush);

        // Accent dot (inner)
        RECT r2 = {ox + 1, oy + 1, ox + DOT - 1, oy + DOT - 1};
        FillRect(hdc, &r2, sBrush);
    }
    DeleteObject(wBrush);
    DeleteObject(dBrush);
    DeleteObject(sBrush);
}

void WinOverlay::DrawModeVisuals(HDC hdc, int w, int h) {
    SetBkMode(hdc, TRANSPARENT);

    // ── Mode 1 (ClickHold): L↑ R↓ label ──
    if (mode_ == ScrollMode::ClickHold) {
        HFONT old = (HFONT)SelectObject(hdc, fontBold_);
        SetTextColor(hdc, RGB(255, 255, 255));

        RECT topR = {0, h / 4, w, h / 2};
        DrawTextW(hdc, L"\u25B2", -1, &topR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT botR = {0, h / 2, w, 3 * h / 4};
        DrawTextW(hdc, L"\u25BC", -1, &botR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Label at bottom with better styling
        SelectObject(hdc, fontSmall_);
        SetTextColor(hdc, RGB(220, 220, 220));
        RECT labelR = {4, h - 22, w - 4, h - 4};
        DrawTextW(hdc, L"L\u2191  R\u2193", -1, &labelR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, old);
        return;
    }

    // ── Mode 2 (SplitHold): split line + top/bottom arrows ──
    if (mode_ == ScrollMode::SplitHold) {
        HPEN oldPen = (HPEN)SelectObject(hdc, dashPen_);
        MoveToEx(hdc, 8, h / 2, nullptr);
        LineTo(hdc, w - 8, h / 2);
        SelectObject(hdc, oldPen);

        HFONT old = (HFONT)SelectObject(hdc, fontBold_);
        SetTextColor(hdc, RGB(255, 255, 255));

        RECT topR = {0, 4, w, h / 2 - 2};
        DrawTextW(hdc, L"\u25B2", -1, &topR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT botR = {0, h / 2 + 4, w, h - 4};
        DrawTextW(hdc, L"\u25BC", -1, &botR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, old);
        return;
    }

    // ── Mode 3 (HoverAuto): color tint top/bottom + arrows ──
    if (mode_ == ScrollMode::HoverAuto) {
        RECT topArea = {4, 4,   w - 4, h / 2 - 2};
        RECT botArea = {4, h / 2 + 2, w - 4, h - 4};
        FillRect(hdc, &topArea, topBrush_);
        FillRect(hdc, &botArea, botBrush_);

        // Divider
        HPEN oldPen = (HPEN)SelectObject(hdc, dashPen_);
        MoveToEx(hdc, 8, h / 2, nullptr);
        LineTo(hdc, w - 8, h / 2);
        SelectObject(hdc, oldPen);

        // Labels with shadow
        HFONT old = (HFONT)SelectObject(hdc, fontBold_);

        SetTextColor(hdc, RGB(0, 0, 0));
        RECT topRS = {3, 10, w + 3, h / 2 - 6 + 3};
        DrawTextW(hdc, L"\u25B2 HOVER", -1, &topRS, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        RECT botRS = {3, h / 2 + 8 + 3, w + 3, h - 10 + 3};
        DrawTextW(hdc, L"\u25BC HOVER", -1, &botRS, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SetTextColor(hdc, RGB(255, 255, 255));
        RECT topR = {0, 8,   w, h / 2 - 6};
        DrawTextW(hdc, L"\u25B2 HOVER", -1, &topR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT botR = {0, h / 2 + 8, w, h - 10};
        DrawTextW(hdc, L"\u25BC HOVER", -1, &botR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, old);
    }
}

// ─────── Window Procedure ───────
LRESULT CALLBACK WinOverlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = g_zoneWnd;
    if (!self || self->hwnd_ != hwnd) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        self->Paint(hdc, rc.right, rc.bottom);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; // handled in Paint via double-buffer

    case WM_LBUTTONDOWN: {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

        // Resize handle check in edit mode (bottom-right 20×20)
        if (self->editMode_) {
            RECT rc; GetClientRect(hwnd, &rc);
            if (pt.x > rc.right - 20 && pt.y > rc.bottom - 20) {
                self->isResizing_ = true;
                self->resizeStart_ = pt;
                SetCapture(hwnd);
                return 0;
            }
        }

        // Drag (when unlocked or in edit mode)
        if (!self->cfg_.locked || self->editMode_) {
            self->isDragging_ = true;
            self->dragStart_  = pt;
            GetWindowRect(hwnd, &self->dragStartRect_);
            SetCapture(hwnd);
        }

        if (self->callback_ && self->enabled_ && !self->editMode_) {
            ZoneEventData d = {};
            d.event = ZoneEvent::LeftClickDown;
            d.clickPos = pt;
            RECT r; GetClientRect(hwnd, &r);
            d.zoneWidth = r.right; d.zoneHeight = r.bottom;
            self->callback_(d);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (self->isDragging_ || self->isResizing_) {
            self->isDragging_  = false;
            self->isResizing_  = false;
            ReleaseCapture();
        }
        if (self->callback_ && self->enabled_ && !self->editMode_) {
            ZoneEventData d = {};
            d.event = ZoneEvent::LeftClickUp;
            d.clickPos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            RECT r; GetClientRect(hwnd, &r);
            d.zoneWidth = r.right; d.zoneHeight = r.bottom;
            self->callback_(d);
        }
        return 0;
    }

    case WM_RBUTTONDOWN: {
        if (self->callback_ && self->enabled_ && !self->editMode_) {
            ZoneEventData d = {};
            d.event = ZoneEvent::RightClickDown;
            d.clickPos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            RECT r; GetClientRect(hwnd, &r);
            d.zoneWidth = r.right; d.zoneHeight = r.bottom;
            self->callback_(d);
        }
        return 0;
    }

    case WM_RBUTTONUP: {
        if (self->callback_ && self->enabled_ && !self->editMode_) {
            ZoneEventData d = {};
            d.event = ZoneEvent::RightClickUp;
            d.clickPos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            RECT r; GetClientRect(hwnd, &r);
            d.zoneWidth = r.right; d.zoneHeight = r.bottom;
            self->callback_(d);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

        if (self->isResizing_) {
            int dw = pt.x - self->resizeStart_.x;
            int dh = pt.y - self->resizeStart_.y;
            RECT wr; GetWindowRect(hwnd, &wr);
            int nw = (std::max)(60, int(wr.right - wr.left) + dw);
            int nh = (std::max)(60, int(wr.bottom - wr.top) + dh);

            self->resizeStart_ = pt;
            self->cfg_.width  = nw;
            self->cfg_.height = nh;
            SetWindowPos(hwnd, nullptr, 0, 0, nw, nh,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            self->Redraw();
        } else if (self->isDragging_) {
            POINT screenPt = pt;
            ClientToScreen(hwnd, &screenPt);
            int nx = screenPt.x - self->dragStart_.x;
            int ny = screenPt.y - self->dragStart_.y;
            self->cfg_.x = nx; self->cfg_.y = ny;
            SetWindowPos(hwnd, nullptr, nx, ny, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        // Cursor styling in edit mode
        if (self->editMode_) {
            RECT rc; GetClientRect(hwnd, &rc);
            bool inGrip = pt.x > rc.right - 20 && pt.y > rc.bottom - 20;
            SetCursor(LoadCursor(nullptr, inGrip ? IDC_SIZENWSE : IDC_SIZEALL));
        }

        // Mode 3 hover events (only when enabled & in HoverAuto)
        if (self->callback_ && self->enabled_ && !self->editMode_ &&
            self->mode_ == ScrollMode::HoverAuto) {
            if (!self->mouseTracking_) {
                TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&tme);
                self->mouseTracking_ = true;
            }
            ZoneEventData d = {};
            d.event = ZoneEvent::HoverMove;
            d.clickPos = pt;
            RECT r; GetClientRect(hwnd, &r);
            d.zoneWidth = r.right; d.zoneHeight = r.bottom;
            self->callback_(d);
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        self->mouseTracking_ = false;
        if (self->callback_ && self->enabled_ && !self->editMode_ &&
            self->mode_ == ScrollMode::HoverAuto) {
            ZoneEventData d = {};
            d.event = ZoneEvent::HoverLeave;
            self->callback_(d);
        }
        return 0;
    }

    case WM_CONTEXTMENU:
        return 0; // suppress right-click context menu

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace sn
