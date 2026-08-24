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
};

struct LsfgPlan {
    size_t generations{};
    bool warm{};
};

class LsfgPacer {
public:
    void SetConfig(const LsfgPacerConfig& config_) {
        config = config_;
    }

    [[nodiscard]] size_t MaxGenerations() const;

    [[nodiscard]] LsfgPlan Plan(size_t capacity);

    void Reset();

private:
    using Clock = std::chrono::steady_clock;

    void Stabilize(Clock::time_point now);
    void DeferEvaluations(Clock::duration amount);
    void UpdateLimit(Clock::time_point now, float base_rate, float target_rate, size_t ceiling);

    LsfgPacerConfig config;

    std::optional<Clock::time_point> last_frame;
    std::optional<Clock::time_point> stable_until;
    std::optional<Clock::time_point> probe_until;
    std::optional<Clock::time_point> next_probe;
    std::optional<Clock::time_point> deficit_since;
    float smoothed_interval{};
    float output_credit{};
    float probe_base_rate{};
    float unloaded_base_rate{};
    size_t issued_generations{};
    size_t probe_previous_limit{};
    size_t limit{};
    uint32_t probe_failures{};
};

}
