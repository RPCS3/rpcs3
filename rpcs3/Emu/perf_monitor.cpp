#include "stdafx.h"
#include "perf_monitor.hpp"
#include "util/cpu_stats.hpp"
#include "Utilities/Thread.h"

LOG_CHANNEL(sys_log, "SYS");

void perf_monitor::operator()()
{
	constexpr u64 update_interval_us = 1000000; // Update every second
	constexpr u64 log_interval_us = 10000000;   // Log every 10 seconds
	u64 elapsed_us = 0;

	utils::cpu_stats stats;
	stats.init_cpu_query();

	while (thread_ctrl::state() != thread_state::aborting)
	{
		thread_ctrl::wait_for(update_interval_us);
		elapsed_us += update_interval_us;

		double total_usage = 0.0;
		std::vector<double> per_core_usage;

		stats.get_per_core_usage(per_core_usage, total_usage);

		if (elapsed_us >= log_interval_us)
		{
			elapsed_us = 0;

			std::string msg = fmt::format("CPU Usage: Total: %.1f%%", total_usage);

			if (!per_core_usage.empty())
			{
				fmt::append(msg, ", Cores:");
			}

			for (size_t i = 0; i < per_core_usage.size(); i++)
			{
				fmt::append(msg, "%s %.1f%%", i > 0 ? "," : "", per_core_usage[i]);
			}

			sys_log.notice("%s", msg);
		}
	}
}

perf_monitor::~perf_monitor()
{
}
