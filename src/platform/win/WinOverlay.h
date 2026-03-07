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
    void Paint(HDC hdc);
    void DrawModeVisuals(HDC hdc);

    HWND hwnd_ = nullptr;
    ZoneConfig cfg_;
    ScrollMode mode_ = ScrollMode::ClickHold;  // default Mode 1
    bool editMode_ = false;
    bool enabled_ = true;
    bool isDragging_ = false;
    POINT dragStart_ = {};
    RECT  dragStartRect_ = {};
    bool isResizing_ = false;
    POINT resizeStart_ = {};
    bool mouseTracking_ = false;  // for WM_MOUSELEAVE (Mode 3)

    ZoneEventCallback callback_;
    HBITMAP coverBmp_ = nullptr;
};

} // namespace sn
