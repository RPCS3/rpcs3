#pragma once

#include "util/types.hpp"

namespace utils
{
	class cpu_stats
	{
		u64 m_last_cpu, m_sys_cpu, m_usr_cpu;

	public:
		cpu_stats();

		double get_usage();

		static u32 get_thread_count();
	};
}
