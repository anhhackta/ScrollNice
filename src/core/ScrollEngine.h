#pragma once
#include <windows.h>

namespace sn {

// ─────────────────────────────────────────────────────────
// ScrollEngine — converts click/hover actions into wheel events
//
// Key design choices:
//  • ClickScroll()  → immediate, one-shot wheel event
//  • ContinuousScrollTick() → called every ~16ms (60fps timer) while held
//  • SendWheelEvent() → routes to correct target window:
//      - If targetHwnd_ is set (found via WindowFromPoint), PostMessage
//        directly to that window regardless of focus
//      - Otherwise fallback to SendInput (goes to focused window)
//
// Using PostMessage + WM_MOUSEWHEEL instead of SendInput avoids
// the "scroll goes nowhere" problem when the zone window itself
// receives focus during click events.
// ─────────────────────────────────────────────────────────

class ScrollEngine {
public:
    // Single click scroll — emit one batch of wheel events
    void ClickScroll(int direction, int amount_px);

    // Continuous scroll — call per tick while button held or hovering
    void ContinuousScrollTick(int direction, int base_speed, int accel, double hold_seconds);

    // Reset accumulator (call when stopping scroll)
    void Reset() { hold_time_ = 0.0; accum_ = 0.0; }

    double HoldTime() const { return hold_time_; }

    // Target window to receive scroll events.
    // Set this to the scrollable window found under the cursor.
    // If nullptr, falls back to SendInput (focused window).
    void SetTargetHwnd(HWND hwnd) { targetHwnd_ = hwnd; }
    HWND GetTargetHwnd() const    { return targetHwnd_; }

private:
    // Send wheel event. Routes to targetHwnd_ if valid, otherwise SendInput.
    void SendWheelEvent(int delta_px);

    double hold_time_ = 0.0;
    double accum_     = 0.0;
    HWND   targetHwnd_ = nullptr;  // scrollable window under cursor
};

} // namespace sn
