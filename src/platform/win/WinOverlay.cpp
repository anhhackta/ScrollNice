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

    // Set opacity
    SetOpacity(cfg_.opacity);

    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    return true;
}

void WinOverlay::Destroy() {
    if (coverBmp_) { DeleteObject(coverBmp_); coverBmp_ = nullptr; }
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    g_zoneWnd = nullptr;
}

// ─────── Public setters ───────
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
        if (a < 20) a = 20; // min visibility
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

void WinOverlay::Redraw() {
    if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE);
}

void WinOverlay::Show() { if (hwnd_) ShowWindow(hwnd_, SW_SHOWNOACTIVATE); }
void WinOverlay::Hide() { if (hwnd_) ShowWindow(hwnd_, SW_HIDE); }

// ─────── Painting ───────
void WinOverlay::Paint(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int w = rc.right, h = rc.bottom;

    // Background
    COLORREF bgColor = HexToColorRef(cfg_.color);
    if (editMode_) bgColor = RGB(255, 180, 0); // orange in edit mode

    // Draw cover image if available
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

    // Border
    COLORREF borderColor = editMode_ ? RGB(255, 120, 0) : RGB(255, 255, 255);
    HPEN pen = CreatePen(PS_SOLID, editMode_ ? 3 : 2, borderColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, nullBrush);

    // Rounded rectangle border
    RoundRect(hdc, 0, 0, w, h, 16, 16);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);

    // Draw split line
    DrawSplitLine(hdc);

    // Draw mode label
    DrawModeLabel(hdc);

    // Draw lock indicator
    if (cfg_.locked) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        RECT lockRect = {w - 22, 2, w - 2, 18};
        DrawTextW(hdc, L"\U0001F512", -1, &lockRect, DT_CENTER | DT_VCENTER);
    }

    // Resize handle in edit mode
    if (editMode_) {
        RECT grip = {w - 14, h - 14, w, h};
        HBRUSH gripBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &grip, gripBrush);
        DeleteObject(gripBrush);
    }
}

void WinOverlay::DrawSplitLine(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int w = rc.right, h = rc.bottom;

    HPEN dashPen = CreatePen(PS_DASH, 1, RGB(255, 255, 255));
    HPEN oldPen = (HPEN)SelectObject(hdc, dashPen);

    if (mode_ == ScrollMode::SplitTB) {
        MoveToEx(hdc, 4, h / 2, nullptr);
        LineTo(hdc, w - 4, h / 2);

        // Draw arrows
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        HFONT font = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        RECT topR = {0, 4, w, h / 2 - 4};
        DrawTextW(hdc, L"\u25B2 UP", -1, &topR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT botR = {0, h / 2 + 4, w, h - 4};
        DrawTextW(hdc, L"\u25BC DOWN", -1, &botR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    } else if (mode_ == ScrollMode::SplitLR) {
        MoveToEx(hdc, w / 2, 4, nullptr);
        LineTo(hdc, w / 2, h - 4);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        HFONT font = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        RECT leftR = {0, 0, w / 2 - 2, h};
        DrawTextW(hdc, L"\u25B2\nUP", -1, &leftR, DT_CENTER | DT_VCENTER | DT_WORDBREAK);

        RECT rightR = {w / 2 + 2, 0, w, h};
        DrawTextW(hdc, L"\u25BC\nDOWN", -1, &rightR, DT_CENTER | DT_VCENTER | DT_WORDBREAK);

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(dashPen);
}

void WinOverlay::DrawModeLabel(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);

    // Skip for split modes (they draw their own labels)
    if (mode_ == ScrollMode::SplitTB || mode_ == ScrollMode::SplitLR) return;

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    HFONT font = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);

    const wchar_t* label = L"";
    if (mode_ == ScrollMode::ClickLR)    label = L"L\u2191 R\u2193";
    else if (mode_ == ScrollMode::ClickRL) label = L"L\u2193 R\u2191";
    else if (mode_ == ScrollMode::Continuous) label = L"Hold L\u2191 R\u2193";

    RECT textR = {4, rc.bottom - 20, rc.right - 4, rc.bottom - 2};
    DrawTextW(hdc, label, -1, &textR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(font);
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

        // Check resize handle in edit mode
        if (self->editMode_) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            if (pt.x > rc.right - 14 && pt.y > rc.bottom - 14) {
                self->isResizing_ = true;
                self->resizeStart_ = pt;
                SetCapture(hwnd);
                return 0;
            }
        }

        // Dragging (when not locked and in edit mode, or always if unlocked)
        if (!self->cfg_.locked) {
            self->isDragging_ = true;
            self->dragStart_ = pt;
            GetWindowRect(hwnd, &self->dragStartRect_);
            SetCapture(hwnd);
        }

        // Fire click event
        if (self->callback_ && self->enabled_ && !self->editMode_) {
            ZoneEventData d = {};
            d.event = ZoneEvent::LeftClickDown;
            d.clickPos = pt;
            RECT r; GetClientRect(hwnd, &r);
            d.zoneWidth = r.right;
            d.zoneHeight = r.bottom;
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
            d.zoneWidth = r.right;
            d.zoneHeight = r.bottom;
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
            d.zoneWidth = r.right;
            d.zoneHeight = r.bottom;
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
            d.zoneWidth = r.right;
            d.zoneHeight = r.bottom;
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
            int nw = (wr.right - wr.left) + dw;
            int nh = (wr.bottom - wr.top) + dh;
            if (nw < 60) nw = 60;
            if (nh < 60) nh = 60;
            self->resizeStart_ = pt;
            self->cfg_.width = nw;
            self->cfg_.height = nh;
            SetWindowPos(hwnd, nullptr, 0, 0, nw, nh, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        else if (self->isDragging_) {
            POINT screenPt = pt;
            ClientToScreen(hwnd, &screenPt);
            int nx = screenPt.x - self->dragStart_.x;
            int ny = screenPt.y - self->dragStart_.y;
            self->cfg_.x = nx;
            self->cfg_.y = ny;
            SetWindowPos(hwnd, nullptr, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        // Change cursor for resize area in edit mode
        if (self->editMode_) {
            RECT rc; GetClientRect(hwnd, &rc);
            if (pt.x > rc.right - 14 && pt.y > rc.bottom - 14)
                SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
            else
                SetCursor(LoadCursor(nullptr, self->cfg_.locked ? IDC_ARROW : IDC_SIZEALL));
        }
        return 0;
    }

    case WM_CONTEXTMENU:
        // Prevent default context menu
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace sn
