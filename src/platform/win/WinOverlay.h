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
    void DrawSplitLine(HDC hdc);
    void DrawModeLabel(HDC hdc);

    HWND hwnd_ = nullptr;
    ZoneConfig cfg_;
    ScrollMode mode_ = ScrollMode::ClickLR;
    bool editMode_ = false;
    bool enabled_ = true;
    bool isDragging_ = false;
    POINT dragStart_ = {};
    RECT  dragStartRect_ = {};
    bool isResizing_ = false;
    POINT resizeStart_ = {};

    ZoneEventCallback callback_;
    HBITMAP coverBmp_ = nullptr;
};

} // namespace sn
