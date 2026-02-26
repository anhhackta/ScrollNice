#pragma once
#include "Config.h"
#include <cmath>
#include <algorithm>

namespace sn {

class ScrollEngine {
public:
    void Configure(const EngineConfig& cfg) {
        sensitivity_    = cfg.sensitivity;
        dead_zone_px_   = cfg.dead_zone_px;
        smoothing_      = cfg.smoothing;
        accel_power_    = cfg.accel_power;
        tick_ms_        = cfg.tick_ms;
        max_events_sec_ = cfg.max_events_per_sec;
    }

    // Call on each mouse move delta (dy in pixels). Returns wheel events to emit.
    // Sign: positive dy (cursor moved down) → scroll down (negative wheel delta)
    int Update(int dy_px, double dt_sec) {
        // Dead zone
        if (std::abs(dy_px) < dead_zone_px_) {
            dy_px = 0;
        }

        // Raw velocity
        double v_raw = dy_px * sensitivity_;

        // Exponential smoothing
        double alpha = 1.0 - smoothing_;
        v_smooth_ = v_smooth_ * (1.0 - alpha) + v_raw * alpha;

        // Acceleration (nonlinear)
        double v_acc = 0.0;
        if (std::abs(v_smooth_) > 0.001) {
            double sign = (v_smooth_ > 0) ? 1.0 : -1.0;
            v_acc = sign * std::pow(std::abs(v_smooth_), accel_power_);
        }

        // Anti-runaway: clamp
        const double max_v = 500.0;
        v_acc = std::clamp(v_acc, -max_v, max_v);

        // Accumulate
        accum_ += v_acc * dt_sec;

        // Convert to wheel events (WHEEL_DELTA = 120)
        const double step = 1.0; // tune: lower = more sensitive
        int events = 0;
        while (accum_ >= step) {
            accum_ -= step;
            events--;  // scroll down = negative wheel delta
        }
        while (accum_ <= -step) {
            accum_ += step;
            events++;  // scroll up = positive wheel delta
        }

        // Throttle
        if (max_events_sec_ > 0) {
            int max_per_tick = std::max(1, (int)(max_events_sec_ * dt_sec));
            events = std::clamp(events, -max_per_tick, max_per_tick);
        }

        return events; // number of WHEEL_DELTA units to send
    }

    void Reset() {
        v_smooth_ = 0.0;
        accum_    = 0.0;
    }

    int TickMs() const { return tick_ms_; }

private:
    double sensitivity_    = 1.2;
    int    dead_zone_px_   = 2;
    double smoothing_      = 0.85;
    double accel_power_    = 1.35;
    int    tick_ms_        = 10;
    int    max_events_sec_ = 120;

    double v_smooth_ = 0.0;
    double accum_    = 0.0;
};

} // namespace sn
