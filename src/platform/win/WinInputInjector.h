#pragma once
#include <windows.h>

namespace sn {

class WinInputInjector {
public:
    // Send wheel scroll. delta > 0 = scroll up, delta < 0 = scroll down.
    // Each unit = WHEEL_DELTA (120).
    void SendWheel(int units);

    // Rate limiting
    void SetMaxEventsPerSec(int max) { max_per_sec_ = max; }

private:
    int    max_per_sec_  = 120;
    DWORD  last_send_ms_ = 0;
    int    events_this_sec_ = 0;
};

} // namespace sn
