#pragma once

#include <util/types.hpp>
#include "Emu/Cell/timers.hpp"

namespace rsx
{
	struct profiling_timer
	{
		bool enabled = false;
		u64 last;

		profiling_timer() = default;

		void start()
		{
			if (enabled) [[unlikely]]
			{
				last = get_system_time();
			}
		}

		s64 duration()
		{
			if (!enabled) [[likely]]
			{
				return 0ll;
			}

			auto old = last;
			last = get_system_time();
			return static_cast<s64>(last - old);
		}
	};
}
