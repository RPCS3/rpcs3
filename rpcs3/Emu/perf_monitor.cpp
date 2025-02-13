#include "stdafx.h"
#include "perf_monitor.hpp"

#include "Emu/System.h"
#include "Emu/Cell/timers.hpp"
#include "util/cpu_stats.hpp"
#include "Utilities/Thread.h"

LOG_CHANNEL(perf_log, "PERF");

void perf_monitor::operator()()
{
	constexpr u64 update_interval_us = 1000000; // Update every second
	constexpr u64 log_interval_us = 10000000;   // Log every 10 seconds
	u64 elapsed_us = 0;

	utils::cpu_stats stats;
	stats.init_cpu_query();

	u32 logged_pause = 0;
	u64 last_pause_time = umax;

	std::vector<double> per_core_usage;
	std::string msg;

	for (u64 sleep_until = get_system_time(); thread_ctrl::state() != thread_state::aborting;)
	{
		thread_ctrl::wait_until(&sleep_until, update_interval_us);
		elapsed_us += update_interval_us;

		if (thread_ctrl::state() == thread_state::aborting)
		{
			break;
		}

		double total_usage = 0.0;

		stats.get_per_core_usage(per_core_usage, total_usage);

		if (elapsed_us >= log_interval_us)
		{
			elapsed_us = 0;

			const bool is_paused = Emu.IsPaused();
			const u64 pause_time = Emu.GetPauseTime();

			if (!is_paused || last_pause_time != pause_time)
			{
				// Resumed or not paused since last check
				logged_pause = 0;
				last_pause_time = pause_time;
			}

			if (is_paused)
			{
				if (logged_pause >= 2)
				{
					// Let's not spam the log when emulation is paused
					// But still emit the message two times so even paused state can be debugged and inspected
					continue;
				}

				logged_pause++;
			}

			msg.clear();
			fmt::append(msg, "CPU Usage: Total: %.1f%%", total_usage);

			if (!per_core_usage.empty())
			{
				fmt::append(msg, ", Cores:");
			}

			for (usz i = 0; i < per_core_usage.size(); i++)
			{
				fmt::append(msg, "%s %.1f%%", i > 0 ? "," : "", per_core_usage[i]);
			}

			perf_log.notice("%s", msg);
		}
	}
}

perf_monitor::~perf_monitor()
{
}
