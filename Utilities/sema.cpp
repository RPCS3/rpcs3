#include "sema.h"

void semaphore_base::imp_wait()
{
	for (u8 i = 0; i < 10; i++)
	{
		busy_wait();

		const s32 value = m_value.load();

		if (value > 0 && m_value.compare_and_swap_test(value, value - 1))
		{
			return;
		}
	}

	while (true)
	{
		// Try hard way
		const s32 value = m_value.atomic_op([](s32& value)
		{
			// Use sign bit to acknowledge waiter presence
			if (value && value > INT32_MIN)
			{
				value--;

				if (value < 0)
				{
					// Remove sign bit
					value -= INT32_MIN;
				}
			}
			else
			{
				// Set sign bit
				value = INT32_MIN;
			}

			return value;
		});

		if (value >= 0)
		{
			// Signal other waiter to wake up or to restore sign bit
			m_value.notify_one();
			return;
		}

		m_value.wait(value);
	}
}

void semaphore_base::imp_post(s32 _old)
{
	verify("semaphore_base: overflow" HERE), _old < 0;

	m_value.notify_one();
}

bool semaphore_base::try_post(s32 _max)
{
	// Conditional increment
	const s32 value = m_value.fetch_op([&](s32& value)
	{
		if (value < _max)
		{
			value += 1;
		}
	});

	if (value < 0)
	{
		imp_post(value);
	}

	return value < _max;
}
