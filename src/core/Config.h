#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace sn {

// ───────────── Zone Config ─────────────
struct ZoneConfig {
    std::string id       = "right";
    std::string type     = "edge";      // "edge" or "floating"
    std::string edge     = "right";     // "left","right","top","bottom"
    std::string mode     = "motion";    // "motion","click_step","hold","wheel_boost"
    int         width    = 10;
    int         height   = 0;           // 0 = use height_pct
    int         height_pct = 100;
    bool        locked   = false;
    // click-step params
    int         click_step_lines  = 3;
    int         click_repeat_ms   = 60;
    int         click_smoothing_ms= 90;
    // floating zone position (only if type=="floating")
    int         x = 0, y = 0, w = 60, h = 200;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ZoneConfig,
    id, type, edge, mode, width, height, height_pct, locked,
    click_step_lines, click_repeat_ms, click_smoothing_ms,
    x, y, w, h)

// ───────────── Engine Config ─────────────
struct EngineConfig {
    double sensitivity       = 1.2;
    int    dead_zone_px      = 2;
    double smoothing         = 0.85;
    double accel_power       = 1.35;
    int    tick_ms           = 10;
    int    max_events_per_sec= 120;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EngineConfig,
    sensitivity, dead_zone_px, smoothing, accel_power, tick_ms, max_events_per_sec)

// ───────────── Hotkey Config ─────────────
struct HotkeyConfig {
    std::string toggle_enabled = "Ctrl+Alt+S";
    std::string toggle_edit    = "Ctrl+Alt+E";
    std::string toggle_zone    = "Ctrl+Alt+Z";
    std::string toggle_wheel   = "Ctrl+Alt+W";
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HotkeyConfig,
    toggle_enabled, toggle_edit, toggle_zone, toggle_wheel)

// ───────────── UI Config ─────────────
struct UIConfig {
    bool   visible      = false;
    double hover_alpha  = 0.15;
    double active_alpha = 0.25;
    std::string theme   = "default";
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UIConfig,
    visible, hover_alpha, active_alpha, theme)

// ───────────── Exclusion Config ─────────────
struct ExclusionConfig {
    bool auto_suspend_fullscreen = true;
    std::vector<std::string> process_names;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ExclusionConfig,
    auto_suspend_fullscreen, process_names)

// ───────────── App Config (root) ─────────────
struct AppConfig {
    int               version          = 1;
    bool              enabled          = true;
    std::string       wheel_block_mode = "off"; // off, global, outside_zone_only, inside_zone_only
    std::string       bypass_modifier  = "Alt";
    HotkeyConfig      hotkeys;
    EngineConfig      engine;
    std::vector<ZoneConfig> zones;
    ExclusionConfig   exclusions;
    UIConfig          ui;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AppConfig,
    version, enabled, wheel_block_mode, bypass_modifier,
    hotkeys, engine, zones, exclusions, ui)

// ───────────── Config Store ─────────────
class ConfigStore {
public:
    // Load from file. Returns false on error (uses defaults).
    bool Load(const std::string& path);
    // Save to file. Returns false on error.
    bool Save(const std::string& path) const;
    // Generate default config with 1 right-edge zone.
    static AppConfig GetDefault();

    AppConfig& Get() { return config_; }
    const AppConfig& Get() const { return config_; }

private:
    AppConfig config_;
};

} // namespace sn
