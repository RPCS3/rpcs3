#pragma once

#include <util/asm.hpp>
#include <util/sysinfo.hpp>

#include "Emu/Cell/timers.hpp"

namespace rsx
{
	static inline u64 uclock()
	{
		static const ullong s_tsc_scaled_freq = (utils::get_tsc_freq() / 1000000);

		if (s_tsc_scaled_freq)
		{
			return utils::get_tsc() / s_tsc_scaled_freq;
		}
		else
		{
			return get_system_time();
		}
	}
}
