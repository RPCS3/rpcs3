#pragma once
#include <mutex>
#include "basic_types.h"
#include "atomic.h"

namespace common
{
	class semaphore
	{
		// semaphore mutex
		std::mutex m_mutex;

		// semaphore condition variable
		std::condition_variable m_cv;

		struct sync_var
		{
			u32 value; // current semaphore value
			u32 waiters; // current amount of waiters
		};

		// current semaphore value
		atomic<sync_var> m_var;

	public:
		// max semaphore value
		const u32 max_value;

		semaphore(u32 max_value = 1, u32 value = 0);

		bool try_wait();

		bool try_post();

		void wait();

		bool post_and_wait();
	};
}