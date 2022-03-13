#pragma once

#include <util/asm.hpp>
#include <util/sysinfo.hpp>

extern u64 get_system_time();

namespace rsx
{
	static inline u64 uclock()
	{
		if (const u64 freq = (utils::get_tsc_freq() / 1000000))
		{
			return utils::get_tsc() / freq;
		}
		else
		{
			return get_system_time();
		}
	}
}
