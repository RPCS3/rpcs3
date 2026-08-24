// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lsfg_pacer.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace lsfg {

namespace {

using Clock = std::chrono::steady_clock;

constexpr float INTERVAL_SMOOTHING = 0.25f;
constexpr float MINIMUM_BASE_RATE = 10.0f;
constexpr float FIXED_DISCONTINUITY_SECONDS = 0.25f;
constexpr float BURST_CADENCE_RATIO = 3.0f;
constexpr float BURST_TARGET_RATIO = 2.0f;
constexpr float PROBE_THROUGHPUT_TOLERANCE = 0.95f;
constexpr float PROBE_BASE_COLLAPSE_RATIO = 0.70f;
constexpr float PROBE_MARGINAL_GAIN = 1.15f;
constexpr float TARGET_SATISFIED_RATIO = 0.95f;
constexpr float UNLOADED_BASE_RETENTION = 0.75f;
constexpr float CREDIT_EPSILON = 1.0e-4f;
constexpr uint32_t MAX_PROBE_FAILURES = 4;

constexpr auto STABILIZATION_DURATION = std::chrono::seconds(1);
constexpr auto PROBE_DURATION = std::chrono::seconds(1);
constexpr auto DEFICIT_DURATION = std::chrono::seconds(1);
constexpr auto PROBE_STEP_DELAY = std::chrono::milliseconds(250);

[[nodiscard]] Clock::duration ProbeBackoff(uint32_t failures) {
    switch (failures) {
    case 1:
        return std::chrono::seconds(5);
    case 2:
        return std::chrono::seconds(15);
    case 3:
        return std::chrono::seconds(30);
    default:
        return std::chrono::seconds(60);
    }
}

}

size_t LsfgPacer::MaxGenerations() const {
    if (config.multiplier < 2) return 0;
    if (config.target_rate != 0) return LSFG_MAX_MULTIPLIER - 1;
    return std::min<size_t>(config.multiplier, LSFG_MAX_MULTIPLIER) - 1;
}

LsfgPlan LsfgPacer::Plan(size_t capacity) {
    const size_t ceiling = std::min(capacity, MaxGenerations());
    if (ceiling == 0) {
        Reset();
        return {};
    }

    const Clock::time_point now = Clock::now();
    const size_t previous_generations = std::exchange(issued_generations, 0);
    if (!last_frame) {
        last_frame = now;
        return {};
    }

    const Clock::duration interval = now - *last_frame;
    const float interval_seconds = std::chrono::duration<float>(interval).count();
    last_frame = now;

    if (interval_seconds <= 0.0f) {
        Stabilize(now);
        return {};
    }

    const float target_rate = static_cast<float>(config.target_rate);

    if (target_rate == 0.0f) {
        output_credit = 0.0f;
        if (interval_seconds > FIXED_DISCONTINUITY_SECONDS) {
            issued_generations = 0;
            return {};
        }
        limit = ceiling;
        issued_generations = limit;
        return LsfgPlan{limit, limit > 0};
    }

    if (smoothed_interval > 0.0f) {
        float burst_threshold = BURST_CADENCE_RATIO / smoothed_interval;
        if (target_rate > 0.0f) {
            burst_threshold = std::max(burst_threshold, target_rate * BURST_TARGET_RATIO);
        }
        if (1.0f / interval_seconds > burst_threshold) {
            DeferEvaluations(interval);
            output_credit = 0.0f;
            return {};
        }
    }

    if (interval_seconds > 1.0f / MINIMUM_BASE_RATE) {
        Stabilize(now);
        return {};
    }

    smoothed_interval = smoothed_interval > 0.0f
                            ? smoothed_interval +
                                  (interval_seconds - smoothed_interval) * INTERVAL_SMOOTHING
                            : interval_seconds;

    if (previous_generations == 0) {
        const float measured = 1.0f / smoothed_interval;
        unloaded_base_rate =
            unloaded_base_rate > 0.0f
                ? unloaded_base_rate + (measured - unloaded_base_rate) * INTERVAL_SMOOTHING
                : measured;
    }

    if (stable_until) {
        if (now < *stable_until) {
            return {};
        }
        stable_until.reset();
    }

    UpdateLimit(now, 1.0f / smoothed_interval, target_rate, ceiling);

    const size_t allowed = std::min(limit, ceiling);
    const float desired_outputs = smoothed_interval * target_rate;
    if (allowed == 0 || desired_outputs <= 1.0f) {
        output_credit = 0.0f;
        return {};
    }

    output_credit += desired_outputs;
    const size_t outputs =
        std::max<size_t>(1, static_cast<size_t>(std::floor(output_credit + CREDIT_EPSILON)));
    const size_t generations = std::min(outputs - 1, allowed);

    output_credit -= static_cast<float>(generations + 1);
    if (output_credit < 0.0f) {
        output_credit = 0.0f;
    } else if (generations == allowed && output_credit >= 1.0f) {
        output_credit = std::fmod(output_credit, 1.0f);
    }

    issued_generations = generations;
    return LsfgPlan{generations, true};
}

void LsfgPacer::UpdateLimit(Clock::time_point now, float base_rate, float target_rate,
                            size_t ceiling) {
    limit = std::min(limit, ceiling);

    if (probe_until) {
        if (now < *probe_until) {
            return;
        }
        probe_until.reset();
        output_credit = 0.0f;

        const float previous_output =
            std::min(target_rate, probe_base_rate * static_cast<float>(probe_previous_limit + 1));
        const float current_output =
            std::min(target_rate, base_rate * static_cast<float>(limit + 1));

        const bool throughput_regressed =
            current_output < previous_output * PROBE_THROUGHPUT_TOLERANCE;
        const bool collapsed_for_marginal_gain =
            base_rate < probe_base_rate * PROBE_BASE_COLLAPSE_RATIO &&
            current_output < previous_output * PROBE_MARGINAL_GAIN;
        const bool emulation_slowed = unloaded_base_rate > 0.0f &&
                                      base_rate < unloaded_base_rate * UNLOADED_BASE_RETENTION;

        if (throughput_regressed || collapsed_for_marginal_gain || emulation_slowed) {
            limit = probe_previous_limit;
            probe_failures = std::min(probe_failures + 1, MAX_PROBE_FAILURES);
            next_probe = now + ProbeBackoff(probe_failures);
            deficit_since.reset();
            return;
        }

        probe_failures = 0;
        next_probe = now + PROBE_STEP_DELAY;
    }

    if (base_rate * static_cast<float>(limit + 1) >= target_rate * TARGET_SATISFIED_RATIO ||
        limit >= ceiling) {
        deficit_since.reset();
        return;
    }

    if (!deficit_since) {
        deficit_since = now;
        return;
    }
    if (now - *deficit_since < DEFICIT_DURATION) {
        return;
    }
    if (next_probe && now < *next_probe) {
        return;
    }

    probe_previous_limit = limit;
    probe_base_rate = base_rate;
    ++limit;
    probe_until = now + PROBE_DURATION;
    deficit_since.reset();
    output_credit = 0.0f;
}

void LsfgPacer::DeferEvaluations(Clock::duration amount) {
    const auto defer = [amount](std::optional<Clock::time_point>& deadline) {
        if (deadline) {
            *deadline += amount;
        }
    };
    defer(stable_until);
    defer(probe_until);
    defer(next_probe);
    deficit_since.reset();
}

void LsfgPacer::Stabilize(Clock::time_point now) {
    stable_until = now + STABILIZATION_DURATION;
    probe_until.reset();
    deficit_since.reset();
    smoothed_interval = 0.0f;
    output_credit = 0.0f;
}

void LsfgPacer::Reset() {
    last_frame.reset();
    stable_until.reset();
    probe_until.reset();
    next_probe.reset();
    deficit_since.reset();
    smoothed_interval = 0.0f;
    output_credit = 0.0f;
    probe_base_rate = 0.0f;
    unloaded_base_rate = 0.0f;
    issued_generations = 0;
    probe_previous_limit = 0;
    limit = 0;
    probe_failures = 0;
}

}
