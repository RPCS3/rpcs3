#include "mutex.h"

#include "util/asm.hpp"

void shared_mutex::imp_lock_shared(u32 val)
{
	ensure(val < c_err); // "shared_mutex underflow"

	// Try to steal the notification bit
	if (val & c_sig && m_value.compare_exchange(val, val - c_sig + 1))
	{
		return;
	}

	for (int i = 0; i < 10; i++)
	{
		if (try_lock_shared())
		{
			return;
		}

		const u32 old = m_value;

		if (old & c_sig && m_value.compare_and_swap_test(old, old - c_sig + 1))
		{
			return;
		}

		busy_wait();
	}

	// Acquire writer lock and downgrade
	const u32 old = m_value.fetch_add(c_one);

	if (old == 0)
	{
		lock_downgrade();
		return;
	}

	ensure((old % c_sig) + c_one < c_sig); // "shared_mutex overflow"
	imp_wait();
	lock_downgrade();
}

void shared_mutex::imp_unlock_shared(u32 old)
{
	ensure(old - 1 < c_err); // "shared_mutex underflow"

	// Check reader count, notify the writer if necessary
	if ((old - 1) % c_one == 0)
	{
		imp_signal();
	}
}

void shared_mutex::imp_wait()
{
	while (true)
	{
		const auto [old, ok] = m_value.fetch_op([](u32& value)
		{
			if (value >= c_sig)
			{
				value -= c_sig;
				return true;
			}

			return false;
		});

		if (ok)
		{
			break;
		}

		m_value.wait(old);
	}
}

void shared_mutex::imp_signal()
{
	m_value += c_sig;
	m_value.notify_one();
}

void shared_mutex::imp_lock(u32 val)
{
	ensure(val < c_err); // "shared_mutex underflow"

	// Try to steal the notification bit
	if (val & c_sig && m_value.compare_exchange(val, val - c_sig + c_one))
	{
		return;
	}

	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		const u32 old = m_value;

		if (!old && try_lock())
		{
			return;
		}

		if (old & c_sig && m_value.compare_and_swap_test(old, old - c_sig + c_one))
		{
			return;
		}
	}

	const u32 old = m_value.fetch_add(c_one);

	if (old == 0)
	{
		return;
	}

	ensure((old % c_sig) + c_one < c_sig); // "shared_mutex overflow"
	imp_wait();
}

void shared_mutex::imp_unlock(u32 old)
{
	ensure(old - c_one < c_err); // "shared_mutex underflow"

	// 1) Notify the next writer if necessary
	// 2) Notify all readers otherwise if necessary (currently indistinguishable from writers)
	if (old - c_one)
	{
		imp_signal();
	}
}

void shared_mutex::imp_lock_upgrade()
{
	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		if (try_lock_upgrade())
		{
			return;
		}
	}

	// Convert to writer lock
	const u32 old = m_value.fetch_add(c_one - 1);

	ensure((old % c_sig) + c_one - 1 < c_sig); // "shared_mutex overflow"

	if (old % c_one == 1)
	{
		return;
	}

	imp_wait();
}

void shared_mutex::imp_lock_unlock()
{
	u32 _max = 1;

	for (int i = 0; i < 30; i++)
	{
		const u32 val = m_value;

		if (val % c_one == 0 && (val / c_one < _max || val >= c_sig))
		{
			// Return if have cought a state where:
			// 1) Mutex is free
			// 2) Total number of waiters decreased since last check
			// 3) Signal bit is set (if used on the platform)
			return;
		}

		_max = val / c_one;

		busy_wait(1500);
	}

	// Lock and unlock
	if (!m_value.fetch_add(c_one))
	{
		unlock();
		return;
	}

	imp_wait();
	unlock();
}
