#include "stdafx.h"
#include "RSXPowerMgmt.h"
#include "RSXDisplay.h"

namespace rsx::power_mgmt
{
	static u64 abs_diff(u64 a, u64 b)
	{
		return a >= b ? a - b : b - a;
	}

	pacing_targets get_pacing_targets(u64 vblank_rate_10)
	{
		return
		{{
			{ 300, false, 96 },
			{ 600, true, 94 },
			{ vblank_rate_10, true, 94 },
			{ vblank_rate_10 / 2, false, 96 },
		}};
	}

	const pacing_target* match_pacing_target(const pacing_targets& targets, u64 fps_10, u64 low_rate_tolerance, u64 high_rate_tolerance)
	{
		for (const auto& target : targets)
		{
			if (abs_diff(fps_10, target.fps_10) < (target.high_rate ? high_rate_tolerance : low_rate_tolerance))
			{
				return &target;
			}
		}

		return nullptr;
	}

	frame_pacing_stats collect_frame_pacing_stats(const std::deque<frame_time_t>& frame_times, u64 current_time)
	{
		frame_pacing_stats stats{};
		const usz count = frame_times.size();

		stats.avg_frame_time = (frame_times.back().timestamp - frame_times.front().timestamp) / (count - 1);
		stats.fps_10 = 10'000'000 / stats.avg_frame_time;

		for (usz i = count - preempt_settle_window + 1; i < count; i++)
		{
			const u64 prev = frame_times[i - 1].preempt_count;
			const u64 cur = frame_times[i].preempt_count;
			stats.preempt_drops += prev > cur;
			stats.preempt_raises += prev < cur;
		}

		stats.settled = !stats.preempt_drops && !stats.preempt_raises;

		for (usz i = 1; i <= count; i++)
		{
			const bool is_last = i == count;
			const u64 timestamp = is_last ? current_time : frame_times[i].timestamp;
			const u64 deviation = abs_diff(timestamp - frame_times[i - 1].timestamp, stats.avg_frame_time);

			if (deviation < stats.avg_frame_time / 7)
			{
				continue;
			}

			stats.fails++;

			if (deviation >= stats.avg_frame_time / 3)
			{
				stats.hard_fails++;
				stats.last_frame_hard_fail |= is_last;
			}
		}

		return stats;
	}
}
