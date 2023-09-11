#include "sema.h"

#include "util/asm.hpp"

void semaphore_base::imp_wait()
{
	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		const u32 value = m_value.load();

		if (value & c_value_mask && m_value.compare_and_swap_test(value, value - c_value))
		{
			return;
		}
	}

	bool waits = false;

	while (true)
	{
		// Try hard way
		const u32 value = m_value.fetch_op([&](u32& value)
		{
			ensure(value != c_waiter_mask); // "semaphore_base: overflow"

			if (value & c_value_mask)
			{
				// Obtain signal
				value -= c_value;

				if (waits)
				{
					// Remove waiter
					value -= c_waiter;
				}
			}
			else if (!waits)
			{
				// Add waiter
				value += c_waiter;
			}
		});

		if (value & c_value_mask)
		{
			break;
		}

		m_value.wait(value + (waits ? 0 : c_waiter));
		waits = true;
	}
}

void semaphore_base::imp_post(u32 _old)
{
	ensure(~_old & c_value_mask); // "semaphore_base: overflow"

	if ((_old & c_waiter_mask) / c_waiter > (_old & c_value_mask) / c_value)
	{
		m_value.notify_one();
	}
}

bool semaphore_base::try_post(u32 _max)
{
	// Conditional increment
	const auto [value, ok] = m_value.fetch_op([&](u32& value)
	{
		if ((value & c_value_mask) <= _max)
		{
			value += c_value;
			return true;
		}

		return false;
	});

	if (!ok)
	{
		return false;
	}

	if (value & c_waiter_mask)
	{
		imp_post(value);
	}

	return true;
}
