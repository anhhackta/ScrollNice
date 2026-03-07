#include "WinOverlay.h"
#include <windowsx.h>

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

// ─────── Create / Destroy ───────
bool WinOverlay::Create(HINSTANCE hInst, const ZoneConfig& cfg, ZoneEventCallback cb) {
    g_zoneWnd = this;
    cfg_ = cfg;
    callback_ = cb;

    WNDCLASSEXW wc = {};
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

    SetOpacity(cfg_.opacity);
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    return true;
}

void WinOverlay::Destroy() {
    if (coverBmp_) { DeleteObject(coverBmp_); coverBmp_ = nullptr; }
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    g_zoneWnd = nullptr;
}

// ─────── Setters ───────
void WinOverlay::SetPosition(int x, int y) {
    cfg_.x = x; cfg_.y = y;
    if (hwnd_) SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void WinOverlay::SetSize(int w, int h) {
    cfg_.width = w; cfg_.height = h;
    if (hwnd_) SetWindowPos(hwnd_, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
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
void WinOverlay::Show() { if (hwnd_) ShowWindow(hwnd_, SW_SHOWNOACTIVATE); }
void WinOverlay::Hide() { if (hwnd_) ShowWindow(hwnd_, SW_HIDE); }

// ─────── Painting ───────
void WinOverlay::Paint(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int w = rc.right, h = rc.bottom;

    // Background color
    COLORREF bgColor = HexToColorRef(cfg_.color);
    if (editMode_) bgColor = RGB(255, 140, 0);

    if (coverBmp_) {
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, coverBmp_);
        StretchBlt(hdc, 0, 0, w, h, memDC, 0, 0, w, h, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteDC(memDC);
    } else {
        HBRUSH brush = CreateSolidBrush(bgColor);
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
    }

    // Mode-specific visuals
    DrawModeVisuals(hdc);

    // Border (rounded)
    COLORREF borderColor = editMode_ ? RGB(255, 100, 0) : RGB(255, 255, 255);
    int borderW = editMode_ ? 3 : 2;
    HPEN pen = CreatePen(PS_SOLID, borderW, borderColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, nullBrush);
    RoundRect(hdc, 0, 0, w, h, 16, 16);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);

    // Lock indicator
    if (cfg_.locked && !editMode_) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        RECT lockR = {w - 22, 2, w - 2, 20};
        // Draw a simple lock icon via text
        HFONT fnt = CreateFontW(13, 0, 0, 0, FW_BOLD, 0, 0, 0,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, fnt);
        DrawTextW(hdc, L"\uD83D\uDD12", -1, &lockR, DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);
        DeleteObject(fnt);
    }

    // Edit mode: resize grip (bottom-right corner)
    if (editMode_) {
        RECT grip = {w - 14, h - 14, w, h};
        HBRUSH gripBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &grip, gripBrush);
        DeleteObject(gripBrush);
    }
}

void WinOverlay::DrawModeVisuals(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int w = rc.right, h = rc.bottom;

    SetBkMode(hdc, TRANSPARENT);

    // ── Mode 1 (ClickHold): show L↑ R↓ label ──
    if (mode_ == ScrollMode::ClickHold) {
        HFONT font = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, font);
        SetTextColor(hdc, RGB(255, 255, 255));

        // Up arrow in top half
        RECT topR = {0, h / 4, w, h / 2};
        DrawTextW(hdc, L"\u25B2", -1, &topR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Down arrow in bottom half
        RECT botR = {0, h / 2, w, 3 * h / 4};
        DrawTextW(hdc, L"\u25BC", -1, &botR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Mode label at bottom
        HFONT fntSm = CreateFontW(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        SelectObject(hdc, fntSm);
        SetTextColor(hdc, COLORREF(0x99FFFFFF & 0x00FFFFFF | (0x88 << 24))); // semi-white
        SetTextColor(hdc, RGB(200, 200, 200));
        RECT labelR = {2, h - 18, w - 2, h - 2};
        DrawTextW(hdc, L"L\u2191  R\u2193", -1, &labelR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, old);
        DeleteObject(font);
        DeleteObject(fntSm);
        return;
    }

    // ── Mode 2 (SplitHold): split line + top/bottom labels ──
    if (mode_ == ScrollMode::SplitHold) {
        // Dashed split line at midpoint
        HPEN dashPen = CreatePen(PS_DASH, 1, RGB(255, 255, 255));
        HPEN oldPen = (HPEN)SelectObject(hdc, dashPen);
        MoveToEx(hdc, 8, h / 2, nullptr);
        LineTo(hdc, w - 8, h / 2);
        SelectObject(hdc, oldPen);
        DeleteObject(dashPen);

        HFONT font = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, font);
        SetTextColor(hdc, RGB(255, 255, 255));

        RECT topR = {0, 4, w, h / 2 - 2};
        DrawTextW(hdc, L"\u25B2 L\u00EAN", -1, &topR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT botR = {0, h / 2 + 2, w, h - 4};
        DrawTextW(hdc, L"\u25BC XU\u1ED0NG", -1, &botR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, old);
        DeleteObject(font);
        return;
    }

    // ── Mode 3 (HoverAuto): hover indicator ──
    if (mode_ == ScrollMode::HoverAuto) {
        // Gradient-like hint: top zone = blue-ish, bottom = red-ish tint
        HBRUSH topBrush = CreateSolidBrush(RGB(60, 100, 180));
        RECT topArea = {4, 4, w - 4, h / 2 - 2};
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, topBrush);
        SelectObject(hdc, oldBrush);
        FillRect(hdc, &topArea, topBrush);
        DeleteObject(topBrush);

        HBRUSH botBrush = CreateSolidBrush(RGB(180, 60, 60));
        RECT botArea = {4, h / 2 + 2, w - 4, h - 4};
        FillRect(hdc, &botArea, botBrush);
        DeleteObject(botBrush);

        HFONT font = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, font);
        SetTextColor(hdc, RGB(255, 255, 255));

        RECT topR = {0, 8, w, h / 2 - 4};
        DrawTextW(hdc, L"\u25B2 DI CHU\u1ED8T", -1, &topR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT botR = {0, h / 2 + 4, w, h - 8};
        DrawTextW(hdc, L"\u25BC DI CHU\u1ED8T", -1, &botR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Dashed divider
        HPEN dashPen = CreatePen(PS_DASH, 1, RGB(255, 255, 255));
        HPEN oldPen = (HPEN)SelectObject(hdc, dashPen);
        MoveToEx(hdc, 8, h / 2, nullptr);
        LineTo(hdc, w - 8, h / 2);
        SelectObject(hdc, oldPen);
        DeleteObject(dashPen);

        SelectObject(hdc, old);
        DeleteObject(font);
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
        self->Paint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;

    case WM_LBUTTONDOWN: {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

        // Resize handle check in edit mode
        if (self->editMode_) {
            RECT rc; GetClientRect(hwnd, &rc);
            if (pt.x > rc.right - 14 && pt.y > rc.bottom - 14) {
                self->isResizing_ = true;
                self->resizeStart_ = pt;
                SetCapture(hwnd);
                return 0;
            }
        }

        // Edit mode: drag
        if (!self->cfg_.locked || self->editMode_) {
            self->isDragging_ = true;
            self->dragStart_ = pt;
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
            self->isDragging_ = false;
            self->isResizing_ = false;
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

        // Resize
        if (self->isResizing_) {
            int dw = pt.x - self->resizeStart_.x;
            int dh = pt.y - self->resizeStart_.y;
            RECT wr; GetWindowRect(hwnd, &wr);
            int nw = std::max(60, (wr.right - wr.left) + dw);
            int nh = std::max(60, (wr.bottom - wr.top) + dh);
            self->resizeStart_ = pt;
            self->cfg_.width = nw; self->cfg_.height = nh;
            SetWindowPos(hwnd, nullptr, 0, 0, nw, nh, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            self->Redraw();
        }
        // Drag
        else if (self->isDragging_) {
            POINT screenPt = pt;
            ClientToScreen(hwnd, &screenPt);
            int nx = screenPt.x - self->dragStart_.x;
            int ny = screenPt.y - self->dragStart_.y;
            self->cfg_.x = nx; self->cfg_.y = ny;
            SetWindowPos(hwnd, nullptr, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        // Cursor styling
        if (self->editMode_) {
            RECT rc; GetClientRect(hwnd, &rc);
            if (pt.x > rc.right - 14 && pt.y > rc.bottom - 14)
                SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
            else
                SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
        }

        // Hover event for Mode 3
        if (self->callback_ && self->enabled_ && !self->editMode_) {
            // Track mouse leave
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
        if (self->callback_ && self->enabled_ && !self->editMode_) {
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
