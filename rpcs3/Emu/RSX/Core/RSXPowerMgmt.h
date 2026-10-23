#pragma once

#include <util/types.hpp>

#include <array>
#include <deque>

namespace rsx
{
	struct frame_time_t;

	// Frame pacing analysis used to throttle guest CPU usage (lv2 preemption/yield control)
	namespace power_mgmt
	{
		constexpr usz frame_history_length = 60;
		constexpr usz preempt_settle_window = 30;

		struct pacing_target
		{
			u64 fps_10 = 0;         // Tenths of fps
			bool high_rate = false; // Matched with the wider of the two tolerances
			u64 yield_pct = 0;      // Portion of the frame during which yielding is allowed
		};

		using pacing_targets = std::array<pacing_target, 4>;

		pacing_targets get_pacing_targets(u64 vblank_rate_10);
		const pacing_target* match_pacing_target(const pacing_targets& targets, u64 fps_10, u64 low_rate_tolerance, u64 high_rate_tolerance);

		struct frame_pacing_stats
		{
			u64 avg_frame_time = 0;
			u64 fps_10 = 0;
			u32 fails = 0;
			u32 hard_fails = 0;
			u32 preempt_raises = 0;
			u32 preempt_drops = 0;
			bool settled = false;              // Preempt count unchanged over the settle window
			bool last_frame_hard_fail = false;
		};

		// Expects a full history (frame_history_length entries)
		frame_pacing_stats collect_frame_pacing_stats(const std::deque<frame_time_t>& frame_times, u64 current_time);
	}
}
