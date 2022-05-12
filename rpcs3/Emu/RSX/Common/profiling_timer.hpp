#pragma once

#include <util/types.hpp>
#include "time.hpp"

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
				last = rsx::uclock();
			}
		}

		s64 duration()
		{
			if (!enabled) [[likely]]
			{
				return 0ll;
			}

			auto old = last;
			last = rsx::uclock();
			return static_cast<s64>(last - old);
		}
	};
}
