#pragma once
#include <windows.h>

namespace sn {

class ScrollEngine {
public:
    // Single click scroll — emit one batch of wheel events
    void ClickScroll(int direction, int amount_px);

    // Continuous scroll — call per tick while button held
    void ContinuousScrollTick(int direction, int base_speed, int accel, double hold_seconds);

    // Stop continuous scroll
    void Reset() { hold_time_ = 0.0; }

    double HoldTime() const { return hold_time_; }

private:
    void SendWheelEvent(int delta_px);
    double hold_time_ = 0.0;
    double accum_ = 0.0;
};

} // namespace sn
