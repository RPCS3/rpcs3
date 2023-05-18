#pragma once

#include "util/types.hpp"
#include <vector>

#ifdef _WIN32
#include <pdh.h>
#include <pdhmsg.h>
#endif

namespace utils
{
	class cpu_stats
	{
		u64 m_last_cpu = 0;
		u64 m_sys_cpu = 0;
		u64 m_usr_cpu = 0;

#ifdef _WIN32
		PDH_HQUERY m_cpu_query = nullptr;
		PDH_HCOUNTER m_cpu_cores = nullptr;
#elif __linux__
		size_t m_previous_idle_time_total = 0;
		size_t m_previous_total_time_total = 0;
		std::vector<size_t> m_previous_idle_times_per_cpu;
		std::vector<size_t> m_previous_total_times_per_cpu;
#endif

	public:
		cpu_stats();
		~cpu_stats();

		double get_usage();

		void init_cpu_query();
		void get_per_core_usage(std::vector<double>& per_core_usage, double& total_usage);

		static u32 get_current_thread_count();
	};
}
