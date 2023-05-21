#pragma once

#include <util/types.hpp>
#include "../RSXThread.h"

namespace rsx
{
	class eng_lock
	{
		rsx::thread* pthr;

	public:
		eng_lock(rsx::thread* target)
			:pthr(target)
		{
			if (pthr->is_current_thread())
			{
				pthr = nullptr;
			}
			else
			{
				pthr->pause();
			}
		}

		~eng_lock()
		{
			if (pthr) pthr->unpause();
		}
	};
}
