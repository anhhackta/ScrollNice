#include "ScrollEngine.h"
#include <cmath>
#include <algorithm>

namespace sn {

void ScrollEngine::SendWheelEvent(int delta_px) {
    if (delta_px == 0) return;

    // Convert pixel amount to wheel delta units (WHEEL_DELTA = 120 = ~3 lines)
    // Positive delta = scroll up, negative = scroll down
    int wheel_delta = delta_px * WHEEL_DELTA / 100;
    if (wheel_delta == 0) wheel_delta = (delta_px > 0) ? WHEEL_DELTA : -WHEEL_DELTA;

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = (DWORD)wheel_delta;
    SendInput(1, &input, sizeof(INPUT));
}

void ScrollEngine::ClickScroll(int direction, int amount_px) {
    // direction: 1 = up, -1 = down
    SendWheelEvent(direction * amount_px);
}

void ScrollEngine::ContinuousScrollTick(int direction, int base_speed, int accel, double hold_seconds) {
    hold_time_ = hold_seconds;

    // Speed increases as user holds longer
    double speed = base_speed + accel * hold_seconds;
    speed = std::min(speed, 200.0); // cap

    // Accumulate fractional wheel events
    accum_ += direction * speed * 0.016; // ~60fps tick

    // Emit when accumulated enough
    const double threshold = 30.0;
    while (std::abs(accum_) >= threshold) {
        int sign = (accum_ > 0) ? 1 : -1;
        SendWheelEvent(sign * (int)threshold);
        accum_ -= sign * threshold;
    }
}

} // namespace sn
