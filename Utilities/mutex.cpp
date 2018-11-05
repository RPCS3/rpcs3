#include "mutex.h"
#include "sync.h"

#include <climits>

void shared_mutex::imp_lock_shared(u32 val)
{
	verify("shared_mutex underflow" HERE), val < c_err;

	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		if (try_lock_shared())
		{
			return;
		}
	}

	// Acquire writer lock and downgrade
	const u32 old = m_value.fetch_add(c_one);

	if (old == 0)
	{
		lock_downgrade();
		return;
	}

	verify("shared_mutex overflow" HERE), (old % c_sig) + c_one < c_sig;
	imp_wait();
	lock_downgrade();
}

void shared_mutex::imp_unlock_shared(u32 old)
{
	verify("shared_mutex underflow" HERE), old - 1 < c_err;

	// Check reader count, notify the writer if necessary
	if ((old - 1) % c_one == 0)
	{
		imp_signal();
	}
}

void shared_mutex::imp_wait()
{
#ifdef _WIN32
	NtWaitForKeyedEvent(nullptr, &m_value, false, nullptr);
#else
	while (true)
	{
		// Load new value, try to acquire c_sig
		auto [value, ok] = m_value.fetch_op([](u32& value)
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
			return;
		}

		futex(&m_value, FUTEX_WAIT_BITSET_PRIVATE, value, nullptr, c_sig);
	}
#endif
}

void shared_mutex::imp_signal()
{
#ifdef _WIN32
	NtReleaseKeyedEvent(nullptr, &m_value, false, nullptr);
#else
	m_value += c_sig;
	futex(&m_value, FUTEX_WAKE_BITSET_PRIVATE, 1, nullptr, c_sig);
	//futex(&m_value, FUTEX_WAKE_BITSET_PRIVATE, c_one, nullptr, c_sig - 1);
#endif
}

void shared_mutex::imp_lock(u32 val)
{
	verify("shared_mutex underflow" HERE), val < c_err;

	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		if (!m_value && try_lock())
		{
			return;
		}
	}

	const u32 old = m_value.fetch_add(c_one);

	if (old == 0)
	{
		return;
	}

	verify("shared_mutex overflow" HERE), (old % c_sig) + c_one < c_sig;
	imp_wait();
}

void shared_mutex::imp_unlock(u32 old)
{
	verify("shared_mutex underflow" HERE), old - c_one < c_err;

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

	verify("shared_mutex overflow" HERE), (old % c_sig) + c_one - 1 < c_sig;

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

#ifndef _WIN32
	while (false)
	{
		const u32 val = m_value;

		if (val % c_one == 0 && (val / c_one < _max || val >= c_sig))
		{
			return;
		}

		if (val <= c_one)
		{
			// Can't expect a signal
			break;
		}

		_max = val / c_one;

		// Monitor all bits except c_sig
		futex(&m_value, FUTEX_WAIT_BITSET_PRIVATE, val, nullptr, c_sig - 1);
	}
#endif

	// Lock and unlock
	if (!m_value.fetch_add(c_one))
	{
		unlock();
		return;
	}

	imp_wait();
	unlock();
}
