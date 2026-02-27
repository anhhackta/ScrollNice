#include "Config.h"
#include <fstream>

namespace sn {

bool ConfigStore::Load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { config_ = GetDefault(); return false; }
    try {
        nlohmann::json j;
        f >> j;
        config_ = j.get<AppConfig>();
        return true;
    } catch (...) {
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
    } catch (...) { return false; }
}

AppConfig ConfigStore::GetDefault() {
    AppConfig cfg;
    // Sensible defaults are already set in struct definitions
    return cfg;
}

} // namespace sn
