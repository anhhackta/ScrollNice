#include "Zone.h"

namespace sn {

void ZoneManager::LoadFromConfig(const std::vector<ZoneConfig>& zones) {
    zones_.clear();
    for (auto& zc : zones) {
        Zone z;
        z.cfg = zc;
        z.enabled = true;
        zones_.push_back(z);
    }
    RecalcRects();
}

void ZoneManager::RecalcRects() {
    for (auto& z : zones_) {
        if (z.cfg.type == "edge") {
            z.rect = CalcEdgeRect(z.cfg);
        } else {
            z.rect = CalcFloatingRect(z.cfg);
        }
    }
}

RECT ZoneManager::CalcEdgeRect(const ZoneConfig& cfg) const {
    // Get primary monitor work area
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    HMONITOR hMon = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
    GetMonitorInfoW(hMon, &mi);
    RECT wa = mi.rcMonitor; // use full monitor area

    int monW = wa.right - wa.left;
    int monH = wa.bottom - wa.top;

    RECT r = {};
    if (cfg.edge == "right") {
        int zoneH = (cfg.height > 0) ? cfg.height : (monH * cfg.height_pct / 100);
        int topOffset = (monH - zoneH) / 2;
        r.left   = wa.right - cfg.width;
        r.right  = wa.right;
        r.top    = wa.top + topOffset;
        r.bottom = r.top + zoneH;
    } else if (cfg.edge == "left") {
        int zoneH = (cfg.height > 0) ? cfg.height : (monH * cfg.height_pct / 100);
        int topOffset = (monH - zoneH) / 2;
        r.left   = wa.left;
        r.right  = wa.left + cfg.width;
        r.top    = wa.top + topOffset;
        r.bottom = r.top + zoneH;
    } else if (cfg.edge == "top") {
        int zoneW = monW;
        int h = (cfg.height > 0) ? cfg.height : 14;
        r.left   = wa.left;
        r.right  = wa.left + zoneW;
        r.top    = wa.top;
        r.bottom = wa.top + h;
    } else if (cfg.edge == "bottom") {
        int zoneW = monW;
        int h = (cfg.height > 0) ? cfg.height : 14;
        r.left   = wa.left;
        r.right  = wa.left + zoneW;
        r.top    = wa.bottom - h;
        r.bottom = wa.bottom;
    }
    return r;
}

RECT ZoneManager::CalcFloatingRect(const ZoneConfig& cfg) const {
    RECT r;
    r.left   = cfg.x;
    r.top    = cfg.y;
    r.right  = cfg.x + cfg.w;
    r.bottom = cfg.y + cfg.h;
    return r;
}

const Zone* ZoneManager::HitTest(POINT pt) const {
    for (auto& z : zones_) {
        if (!z.enabled) continue;
        if (pt.x >= z.rect.left && pt.x < z.rect.right &&
            pt.y >= z.rect.top  && pt.y < z.rect.bottom) {
            return &z;
        }
    }
    return nullptr;
}

Zone* ZoneManager::HitTest(POINT pt) {
    for (auto& z : zones_) {
        if (!z.enabled) continue;
        if (pt.x >= z.rect.left && pt.x < z.rect.right &&
            pt.y >= z.rect.top  && pt.y < z.rect.bottom) {
            return &z;
        }
    }
    return nullptr;
}

void ZoneManager::ToggleNextZone() {
    if (zones_.empty()) return;
    active_toggle_idx_ = active_toggle_idx_ % static_cast<int>(zones_.size());
    zones_[active_toggle_idx_].enabled = !zones_[active_toggle_idx_].enabled;
    active_toggle_idx_ = (active_toggle_idx_ + 1) % static_cast<int>(zones_.size());
}

} // namespace sn
