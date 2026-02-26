#include "Config.h"
#include <fstream>
#include <iostream>

namespace sn {

bool ConfigStore::Load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        config_ = GetDefault();
        return false;
    }
    try {
        nlohmann::json j;
        f >> j;
        config_ = j.get<AppConfig>();
        return true;
    } catch (const std::exception&) {
        config_ = GetDefault();
        return false;
    }
}

bool ConfigStore::Save(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    try {
        nlohmann::json j = config_;
        f << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

AppConfig ConfigStore::GetDefault() {
    AppConfig cfg;
    // Default zone: right edge
    ZoneConfig right;
    right.id = "right";
    right.type = "edge";
    right.edge = "right";
    right.width = 10;
    right.height_pct = 100;
    right.mode = "motion";
    right.locked = false;
    cfg.zones.push_back(right);
    return cfg;
}

} // namespace sn
