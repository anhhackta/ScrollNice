#include "ScrollEngine.h"
#include <cmath>
#include <algorithm>

namespace sn {

// ─────────────────────────────────────────────────────────
// SendWheelEvent: Core routing logic
//
// Strategy for reaching the correct scrollable window:
//   1. If targetHwnd_ is set → PostMessage directly (bypasses focus)
//   2. Fallback → SendInput (goes to focused window — less reliable)
//
// PostMessage with WM_MOUSEWHEEL:
//   wParam high word = wheel delta (WHEEL_DELTA=120 = ~3 lines)
//   wParam low word  = virtual keys (0 = no modifier)
//   lParam           = cursor screen coordinates (MAKELPARAM(x, y))
//
// We put the current cursor position in lParam so the target window
// can do its own hit-testing if needed (some apps use it).
// ─────────────────────────────────────────────────────────
void ScrollEngine::SendWheelEvent(int delta_px) {
    if (delta_px == 0) return;

    // Convert pixel amount → WHEEL_DELTA units
    // WHEEL_DELTA = 120 ≈ 3 lines of scroll
    int wheel_delta = delta_px * WHEEL_DELTA / 100;
    if (wheel_delta == 0)
        wheel_delta = (delta_px > 0) ? WHEEL_DELTA : -WHEEL_DELTA;

    POINT cursor;
    GetCursorPos(&cursor);

    if (targetHwnd_ && IsWindow(targetHwnd_)) {
        // Route directly to the target window regardless of focus.
        // MAKEWPARAM: low=virtual keys (0=none), high=wheel delta
        WPARAM wp = MAKEWPARAM(0, (SHORT)wheel_delta);
        LPARAM lp = MAKELPARAM(cursor.x, cursor.y);
        PostMessage(targetHwnd_, WM_MOUSEWHEEL, wp, lp);
    } else {
        // Fallback: SendInput (delivers to focused window)
        INPUT input    = {};
        input.type     = INPUT_MOUSE;
        input.mi.dwFlags   = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = (DWORD)wheel_delta;
        SendInput(1, &input, sizeof(INPUT));
    }
}

void ScrollEngine::ClickScroll(int direction, int amount_px) {
    // direction: +1 = scroll UP, -1 = scroll DOWN
    SendWheelEvent(direction * amount_px);
}

void ScrollEngine::ContinuousScrollTick(int direction, int base_speed, int accel, double hold_seconds) {
    hold_time_ = hold_seconds;

    // Speed increases the longer the user holds — capped at 200px/tick
    double speed = base_speed + accel * hold_seconds;
    speed = std::min(speed, 200.0);

    // Accumulate fractional events (avoids missing slow speeds)
    accum_ += direction * speed * 0.016; // 0.016s ≈ 60fps tick

    // Emit wheel events when accumulator crosses threshold
    const double threshold = 30.0;
    while (std::abs(accum_) >= threshold) {
        int sign = (accum_ > 0) ? 1 : -1;
        SendWheelEvent(sign * (int)threshold);
        accum_ -= sign * threshold;
    }
}

} // namespace sn
