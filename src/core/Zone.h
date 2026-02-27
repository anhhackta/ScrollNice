#pragma once
#include "Config.h"
#include <windows.h>

namespace sn {

// Which half of the zone was clicked
enum class ZoneHalf {
    None,
    Top, Bottom,
    Left, Right
};

class ZoneManager {
public:
    void LoadFromConfig(const ZoneConfig& cfg);
    void UpdatePosition(int x, int y);
    void UpdateSize(int w, int h);

    RECT GetRect() const;

    // Is pt inside zone?
    bool HitTest(POINT pt) const;

    // Determine which half of zone the point is in
    ZoneHalf GetHalf(POINT pt, ScrollMode mode) const;

    const ZoneConfig& Config() const { return cfg_; }
    ZoneConfig& Config() { return cfg_; }

private:
    ZoneConfig cfg_;
};

} // namespace sn
