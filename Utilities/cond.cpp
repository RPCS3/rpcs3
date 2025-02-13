#include "cond.h"

// use constants, increase signal space

void cond_variable::imp_wait(u32 _old, u64 _timeout) noexcept
{
	// Not supposed to fail
	ensure(_old);

	// Wait with timeout
	m_value.wait(_old, atomic_wait_timeout{_timeout > max_timeout ? umax : _timeout * 1000});

	// Cleanup
	m_value.atomic_op([](u32& value)
	{
		// Remove waiter (c_waiter_mask)
		value -= 1;

		if ((value & c_waiter_mask) == 0)
		{
			// Last waiter removed, clean signals
			value = 0;
		}
	});
}

void cond_variable::imp_wake(u32 _count) noexcept
{
	const auto [_old, ok] = m_value.fetch_op([](u32& value)
	{
		if (!value || (value & c_signal_mask) == c_signal_mask)
		{
			return false;
		}

		// Add signal
		value += c_signal_mask & (0 - c_signal_mask);
		return true;
	});

	if (!ok || !_count)
	{
		return;
	}

	if (_count > 1 || ((_old + (c_signal_mask & (0 - c_signal_mask))) & c_signal_mask) == c_signal_mask)
	{
		// Resort to notify_all if signal count reached max
		m_value.notify_all();
	}
	else
	{
		m_value.notify_one();
	}
}
