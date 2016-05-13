#pragma once

#include "types.h"
#include "Atomic.h"
#include "Platform.h"

// Binary semaphore
class benaphore
{
	struct internal;

	// Reserved value (-1) enforces *_hard() calls
	atomic_t<u32> m_value{};

	atomic_t<internal*> m_data{};

	void wait_hard();
	void post_hard();

public:
	constexpr benaphore() = default;

	~benaphore();

	// Initialize internal data
	void initialize_once();

	void wait()
	{
		if (UNLIKELY(!m_value.compare_and_swap_test(1, 0)))
		{
			wait_hard();
		}
	}

	bool try_wait()
	{
		return m_value.compare_and_swap_test(1, 0);
	}

	void post()
	{
		if (UNLIKELY(!m_value.compare_and_swap_test(0, 1)))
		{
			post_hard();
		}
	}
};
