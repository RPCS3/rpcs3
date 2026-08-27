// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace lsfg {

constexpr size_t LSFG_MAX_MULTIPLIER = 4;

struct LsfgPacerConfig {
    uint32_t multiplier{2};
    uint32_t target_rate{};
    float refresh_rate{};
};

struct LsfgPlan {
    size_t generations{};
    bool warm{};
};

struct LsfgPacerStats {
    float source_rate{};
    float loop_rate{};
    float refresh_rate{};
    float target_rate{};
    float slots{};
    size_t limit{};
    bool rates_settled{};
    uint64_t last_drawn{};
    float last_elapsed{};
    uint64_t source_frames{};
};

class LsfgPacer {
public:
    void SetConfig(const LsfgPacerConfig& config_) {
        config = config_;
    }

    [[nodiscard]] const LsfgPacerConfig& Config() const {
        return config;
    }

    [[nodiscard]] size_t MaxGenerations() const;

    [[nodiscard]] LsfgPlan Plan(size_t capacity, uint64_t source_frames);

    [[nodiscard]] LsfgPacerStats Stats() const;

    void Reset();

private:
    using Clock = std::chrono::steady_clock;

    void TrackSourceRate(Clock::time_point now, uint64_t source_frames);
    void TrackLoopRate(float interval_seconds);
    [[nodiscard]] bool RatesSettled() const;
    [[nodiscard]] size_t HeadroomLimit() const;

    LsfgPacerConfig config;

    std::optional<Clock::time_point> last_frame;
    std::optional<Clock::time_point> last_source_sample;
    uint64_t last_source_frames{};
    float source_interval{};
    float source_frame_accum{};
    float source_time_accum{};
    float loop_interval{};
    uint32_t source_samples{};
    uint32_t loop_samples{};
    uint64_t last_drawn{};
    float last_elapsed{};
    float output_credit{};
    size_t limit{};
};

}
