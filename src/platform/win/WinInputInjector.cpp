#include "WinInputInjector.h"

namespace sn {

void WinInputInjector::SendWheel(int units) {
    if (units == 0) return;

    // Rate limiting
    DWORD now = GetTickCount();
    if (now - last_send_ms_ >= 1000) {
        last_send_ms_ = now;
        events_this_sec_ = 0;
    }
    if (events_this_sec_ >= max_per_sec_) return;

    int abs_units = (units > 0) ? units : -units;
    int sign = (units > 0) ? 1 : -1;

    for (int i = 0; i < abs_units && events_this_sec_ < max_per_sec_; ++i) {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = (DWORD)(sign * WHEEL_DELTA);
        SendInput(1, &input, sizeof(INPUT));
        events_this_sec_++;
    }
}

} // namespace sn
