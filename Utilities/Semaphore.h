#pragma once

#include "types.h"
#include "Atomic.h"

class semaphore_t
{
	// semaphore mutex
	std::mutex m_mutex;

	// semaphore condition variable
	std::condition_variable m_cv;

	struct alignas(8) sync_var_t
	{
		u32 value; // current semaphore value
		u32 waiters; // current amount of waiters
	};

	// current semaphore value
	atomic_t<sync_var_t> m_var;

public:
	// max semaphore value
	const u32 max_value;

	semaphore_t(u32 max_value = 1, u32 value = 0)
		: m_var(sync_var_t{ value, 0 })
		, max_value(max_value)
	{
	}

	bool try_wait();

	bool try_post();

	void wait();

	bool post_and_wait();
};
