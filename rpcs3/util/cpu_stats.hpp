#pragma once

#include "util/types.hpp"

namespace utils
{
	class cpu_stats
	{
		u64 m_last_cpu = 0, m_sys_cpu = 0, m_usr_cpu = 0;

	public:
		cpu_stats();

		double get_usage();

		static u32 get_thread_count();
	};
}
