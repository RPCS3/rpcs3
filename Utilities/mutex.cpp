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
#ifdef _WIN32
		NtReleaseKeyedEvent(nullptr, &m_value, false, nullptr);
#else
		m_value += c_sig;

		futex(reinterpret_cast<int*>(&m_value.raw()), FUTEX_WAKE_BITSET_PRIVATE, 1, nullptr, nullptr, c_sig);
#endif
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

		futex(reinterpret_cast<int*>(&m_value.raw()), FUTEX_WAIT_BITSET_PRIVATE, value, nullptr, nullptr, c_sig);
	}
#endif
}

void shared_mutex::imp_lock(u32 val)
{
	verify("shared_mutex underflow" HERE), val < c_err;

	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		if (try_lock())
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
#ifdef _WIN32
	if (old - c_one)
	{
		NtReleaseKeyedEvent(nullptr, &m_value, false, nullptr);
	}
#else
	if (old - c_one)
	{
		m_value += c_sig;

		futex(reinterpret_cast<int*>(&m_value.raw()), FUTEX_WAKE_BITSET_PRIVATE, 1, nullptr, nullptr, c_sig);
	}
#endif
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
