#pragma once
#include "Config.h"
#include <windows.h>
#include <vector>

namespace sn {

// Runtime zone data — computed from config + monitor geometry
struct Zone {
    ZoneConfig cfg;
    RECT       rect = {};     // screen coords
    bool       enabled = true;
};

class ZoneManager {
public:
    void LoadFromConfig(const std::vector<ZoneConfig>& zones);
    void RecalcRects();              // recalc based on current monitors

    // Returns pointer to zone under pt, or nullptr
    const Zone* HitTest(POINT pt) const;
    Zone*       HitTest(POINT pt);

    // Toggle individual zone
    void ToggleNextZone();

    std::vector<Zone>& Zones() { return zones_; }
    const std::vector<Zone>& Zones() const { return zones_; }

private:
    RECT CalcEdgeRect(const ZoneConfig& cfg) const;
    RECT CalcFloatingRect(const ZoneConfig& cfg) const;

    std::vector<Zone> zones_;
    int active_toggle_idx_ = 0;
};

} // namespace sn
