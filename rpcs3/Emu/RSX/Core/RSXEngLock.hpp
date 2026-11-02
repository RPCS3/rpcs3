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
				pthr = nullptr;

			lock();
		}

		~eng_lock()
		{
			unlock();
		}

		void lock()
		{
			if (pthr) pthr->pause();
		}

		void unlock()
		{
			if (pthr) pthr->unpause();
		}
	};
}
