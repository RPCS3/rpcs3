#pragma once

#include <util/types.hpp>

namespace rsx
{
	struct profiling_timer
	{
		bool enabled = false;
		steady_clock::time_point last;

		profiling_timer() = default;

		void start()
		{
			if (enabled) [[unlikely]]
			{
				last = steady_clock::now();
			}
		}

		s64 duration()
		{
			if (!enabled) [[likely]]
			{
				return 0ll;
			}

			auto old = last;
			last = steady_clock::now();
			return std::chrono::duration_cast<std::chrono::microseconds>(last - old).count();
		}
	};
}
