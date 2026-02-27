#include "Zone.h"

namespace sn {

void ZoneManager::LoadFromConfig(const ZoneConfig& cfg) {
    cfg_ = cfg;
}

void ZoneManager::UpdatePosition(int x, int y) {
    cfg_.x = x;
    cfg_.y = y;
}

void ZoneManager::UpdateSize(int w, int h) {
    cfg_.width = w;
    cfg_.height = h;
}

RECT ZoneManager::GetRect() const {
    RECT r;
    r.left   = cfg_.x;
    r.top    = cfg_.y;
    r.right  = cfg_.x + cfg_.width;
    r.bottom = cfg_.y + cfg_.height;
    return r;
}

bool ZoneManager::HitTest(POINT pt) const {
    RECT r = GetRect();
    return pt.x >= r.left && pt.x < r.right &&
           pt.y >= r.top  && pt.y < r.bottom;
}

ZoneHalf ZoneManager::GetHalf(POINT pt, ScrollMode mode) const {
    if (!HitTest(pt)) return ZoneHalf::None;

    int relX = pt.x - cfg_.x;
    int relY = pt.y - cfg_.y;

    if (mode == ScrollMode::SplitTB) {
        return (relY < cfg_.height / 2) ? ZoneHalf::Top : ZoneHalf::Bottom;
    }
    if (mode == ScrollMode::SplitLR) {
        return (relX < cfg_.width / 2) ? ZoneHalf::Left : ZoneHalf::Right;
    }

    // For non-split modes, return None (caller uses button instead)
    return ZoneHalf::None;
}

} // namespace sn
