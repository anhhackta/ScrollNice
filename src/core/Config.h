#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace sn {

// ───── Scroll Modes ─────
enum class ScrollMode {
    ClickLR,        // Left-click=up, Right-click=down
    ClickRL,        // Left-click=down, Right-click=up
    SplitLR,        // Zone split left/right halves
    SplitTB,        // Zone split top/bottom halves
    Continuous      // Hold left=up gradually, hold right=down gradually
};

inline std::string ScrollModeToString(ScrollMode m) {
    switch (m) {
        case ScrollMode::ClickLR:    return "click_lr";
        case ScrollMode::ClickRL:    return "click_rl";
        case ScrollMode::SplitLR:    return "split_lr";
        case ScrollMode::SplitTB:    return "split_tb";
        case ScrollMode::Continuous: return "continuous";
    }
    return "click_lr";
}

inline ScrollMode ScrollModeFromString(const std::string& s) {
    if (s == "click_rl")    return ScrollMode::ClickRL;
    if (s == "split_lr")    return ScrollMode::SplitLR;
    if (s == "split_tb")    return ScrollMode::SplitTB;
    if (s == "continuous")  return ScrollMode::Continuous;
    return ScrollMode::ClickLR;
}

// ───── Zone Config ─────
struct ZoneConfig {
    int x = 100, y = 100;
    int width = 120, height = 180;
    double opacity = 0.25;
    std::string color = "#3498db";
    std::string cover_image;  // path to image file (empty = none)
    bool locked = false;
};

inline void to_json(nlohmann::json& j, const ZoneConfig& z) {
    j = {{"x", z.x}, {"y", z.y}, {"width", z.width}, {"height", z.height},
         {"opacity", z.opacity}, {"color", z.color}, {"cover_image", z.cover_image},
         {"locked", z.locked}};
}
inline void from_json(const nlohmann::json& j, ZoneConfig& z) {
    if (j.contains("x")) j.at("x").get_to(z.x);
    if (j.contains("y")) j.at("y").get_to(z.y);
    if (j.contains("width")) j.at("width").get_to(z.width);
    if (j.contains("height")) j.at("height").get_to(z.height);
    if (j.contains("opacity")) j.at("opacity").get_to(z.opacity);
    if (j.contains("color")) j.at("color").get_to(z.color);
    if (j.contains("cover_image")) j.at("cover_image").get_to(z.cover_image);
    if (j.contains("locked")) j.at("locked").get_to(z.locked);
}

// ───── Scroll Config ─────
struct ScrollConfig {
    std::string mode = "click_lr";
    int scroll_amount = 300;      // pixels per click
    int continuous_speed = 10;    // pixels per tick in continuous mode
    int continuous_accel = 2;     // acceleration per second held
};

inline void to_json(nlohmann::json& j, const ScrollConfig& s) {
    j = {{"mode", s.mode}, {"scroll_amount", s.scroll_amount},
         {"continuous_speed", s.continuous_speed}, {"continuous_accel", s.continuous_accel}};
}
inline void from_json(const nlohmann::json& j, ScrollConfig& s) {
    if (j.contains("mode")) j.at("mode").get_to(s.mode);
    if (j.contains("scroll_amount")) j.at("scroll_amount").get_to(s.scroll_amount);
    if (j.contains("continuous_speed")) j.at("continuous_speed").get_to(s.continuous_speed);
    if (j.contains("continuous_accel")) j.at("continuous_accel").get_to(s.continuous_accel);
}

// ───── Sound Config ─────
struct SoundConfig {
    bool enabled = true;
    std::string click_sound;  // path to WAV file (empty = built-in)
};

inline void to_json(nlohmann::json& j, const SoundConfig& s) {
    j = {{"enabled", s.enabled}, {"click_sound", s.click_sound}};
}
inline void from_json(const nlohmann::json& j, SoundConfig& s) {
    if (j.contains("enabled")) j.at("enabled").get_to(s.enabled);
    if (j.contains("click_sound")) j.at("click_sound").get_to(s.click_sound);
}

// ───── Hotkey Config ─────
struct HotkeyConfig {
    std::string toggle_enabled = "Ctrl+Alt+S";
    std::string toggle_edit    = "Ctrl+Alt+E";
    std::string toggle_wheel   = "Ctrl+Alt+W";
};

inline void to_json(nlohmann::json& j, const HotkeyConfig& h) {
    j = {{"toggle_enabled", h.toggle_enabled}, {"toggle_edit", h.toggle_edit},
         {"toggle_wheel", h.toggle_wheel}};
}
inline void from_json(const nlohmann::json& j, HotkeyConfig& h) {
    if (j.contains("toggle_enabled")) j.at("toggle_enabled").get_to(h.toggle_enabled);
    if (j.contains("toggle_edit")) j.at("toggle_edit").get_to(h.toggle_edit);
    if (j.contains("toggle_wheel")) j.at("toggle_wheel").get_to(h.toggle_wheel);
}

// ───── App Config (root) ─────
struct AppConfig {
    int         version = 1;
    bool        enabled = true;
    bool        start_with_windows = false;
    bool        wheel_block = false;
    ZoneConfig  zone;
    ScrollConfig scroll;
    SoundConfig  sound;
    HotkeyConfig hotkeys;
};

inline void to_json(nlohmann::json& j, const AppConfig& c) {
    j = {{"version", c.version}, {"enabled", c.enabled},
         {"start_with_windows", c.start_with_windows}, {"wheel_block", c.wheel_block},
         {"zone", c.zone}, {"scroll", c.scroll}, {"sound", c.sound}, {"hotkeys", c.hotkeys}};
}
inline void from_json(const nlohmann::json& j, AppConfig& c) {
    if (j.contains("version")) j.at("version").get_to(c.version);
    if (j.contains("enabled")) j.at("enabled").get_to(c.enabled);
    if (j.contains("start_with_windows")) j.at("start_with_windows").get_to(c.start_with_windows);
    if (j.contains("wheel_block")) j.at("wheel_block").get_to(c.wheel_block);
    if (j.contains("zone")) j.at("zone").get_to(c.zone);
    if (j.contains("scroll")) j.at("scroll").get_to(c.scroll);
    if (j.contains("sound")) j.at("sound").get_to(c.sound);
    if (j.contains("hotkeys")) j.at("hotkeys").get_to(c.hotkeys);
}

// ───── Config Store ─────
class ConfigStore {
public:
    bool Load(const std::string& path);
    bool Save(const std::string& path) const;
    static AppConfig GetDefault();

    AppConfig& Get() { return config_; }
    const AppConfig& Get() const { return config_; }

private:
    AppConfig config_;
};

} // namespace sn
