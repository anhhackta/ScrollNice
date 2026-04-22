#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include "../../core/Config.h"

namespace sn {

// Events the zone window sends to the app
enum class ZoneEvent {
    LeftClickDown,
    LeftClickUp,
    RightClickDown,
    RightClickUp,
    DragMove,
    ResizeEnd,
    HoverMove,    // mouse moved inside zone (for Mode 3)
    HoverLeave,   // mouse left zone (for Mode 3)
    Closed
};

struct ZoneEventData {
    ZoneEvent event;
    POINT clickPos;       // client coords of click
    int zoneWidth;
    int zoneHeight;
    int newX, newY;       // for drag/resize
};

using ZoneEventCallback = std::function<void(const ZoneEventData&)>;

class WinOverlay {
public:
    bool Create(HINSTANCE hInst, const ZoneConfig& cfg, ZoneEventCallback cb);
    void Destroy();

    void SetPosition(int x, int y);
    void SetSize(int w, int h);
    void SetLocked(bool locked);
    void SetEditMode(bool edit);
    void SetScrollMode(ScrollMode mode);
    void SetOpacity(double alpha);
    void SetEnabled(bool enabled);
    void SetCoverImage(const std::string& path);

    void Redraw();
    void Show();
    void Hide();

    HWND Handle() const { return hwnd_; }
    const ZoneConfig& Config() const { return cfg_; }

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    void Paint(HDC hdc, int w, int h);
    void DrawModeVisuals(HDC hdc, int w, int h);
    void DrawResizeGrip(HDC hdc, int w, int h);

    // GDI cache — created once in InitGDI(), destroyed in DestroyGDI()
    void InitGDI();
    void DestroyGDI();

    HWND hwnd_ = nullptr;
    ZoneConfig cfg_;
    ScrollMode mode_ = ScrollMode::ClickHold;
    bool editMode_ = false;
    bool enabled_ = true;
    bool isDragging_ = false;
    POINT dragStart_ = {};
    RECT  dragStartRect_ = {};
    bool isResizing_ = false;
    POINT resizeStart_ = {};
    bool mouseTracking_ = false;

    ZoneEventCallback callback_;
    HBITMAP coverBmp_ = nullptr;

    // ── Cached GDI objects (no per-frame alloc) ──
    HFONT   fontBold_   = nullptr;  // 14pt Segoe UI Bold
    HFONT   fontSmall_  = nullptr;  // 10pt Segoe UI
    HPEN    borderPen_  = nullptr;  // 2px white
    HPEN    editPen_    = nullptr;  // 3px orange
    HPEN    dashPen_    = nullptr;  // 1px dashed white
    HBRUSH  topBrush_   = nullptr;  // Mode 3 top tint (blue)
    HBRUSH  botBrush_   = nullptr;  // Mode 3 bottom tint (red)
    HBRUSH  editBrush_  = nullptr;  // edit mode background tint
};

} // namespace sn
