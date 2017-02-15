#include "mutex.h"
#include "sync.h"

void shared_mutex::imp_lock_shared(s64 _old)
{
	verify("shared_mutex overflow" HERE), _old <= c_max;

	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		const s64 value = m_value.load();

		if (value >= c_min && m_value.compare_and_swap_test(value, value - c_min))
		{
			return;
		}
	}

	// Acquire writer lock
	imp_wait(m_value.load());

	// Convert value
	s64 value = m_value.fetch_add(c_one - c_min);

	if (value != 0)
	{
		imp_unlock(value);
	}
#ifdef _WIN32
	// Wait as a reader if necessary
	if (value + c_one - c_min < 0)
	{
		NtWaitForKeyedEvent(nullptr, (int*)&m_value + 1, false, nullptr);
	}
#else
	// Use resulting value
	value += c_one - c_min;

	while (value < 0)
	{
		futex((int*)&m_value.raw() + IS_LE_MACHINE, FUTEX_WAIT_PRIVATE, int(value >> 32), nullptr, nullptr, 0);

		value = m_value.load();
	}
#endif
}

void shared_mutex::imp_unlock_shared(s64 _old)
{
	verify("shared_mutex overflow" HERE), _old + c_min <= c_max;

	// Check reader count, notify the writer if necessary
	if ((_old + c_min) % c_one == 0)
	{
#ifdef _WIN32
		NtReleaseKeyedEvent(nullptr, &m_value, false, nullptr);
#else
		futex((int*)&m_value.raw() + IS_BE_MACHINE, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
#endif
	}
}

void shared_mutex::imp_wait(s64 _old)
{
#ifdef _WIN32
	if (m_value.sub_fetch(c_one))
	{
		NtWaitForKeyedEvent(nullptr, &m_value, false, nullptr);
	}
#else
	_old = m_value.fetch_sub(c_one);

	// Return immediately if locked
	while (_old != c_one)
	{
		// Load new value
		const s64 value = m_value.load();

		// Detect addition (unlock op)
		if (value / c_one > _old / c_one)
		{
			return;
		}

		futex((int*)&m_value.raw() + IS_BE_MACHINE, FUTEX_WAIT_PRIVATE, value, nullptr, nullptr, 0);

		// Update old value
		_old = value;
	}
#endif
}

void shared_mutex::imp_lock(s64 _old)
{
	verify("shared_mutex overflow" HERE), _old <= c_max;

	for (int i = 0; i < 10; i++)
	{
		busy_wait();

		const s64 value = m_value.load();

		if (value == c_one && m_value.compare_and_swap_test(c_one, 0))
		{
			return;
		}
	}

	imp_wait(m_value.load());
}

void shared_mutex::imp_unlock(s64 _old)
{
	verify("shared_mutex overflow" HERE), _old + c_one <= c_max;

	// 1) Notify the next writer if necessary
	// 2) Notify all readers otherwise if necessary
#ifdef _WIN32
	if (_old + c_one <= 0)
	{
		NtReleaseKeyedEvent(nullptr, &m_value, false, nullptr);
	}
	else if (s64 count = -_old / c_min)
	{
		while (count--)
		{
			NtReleaseKeyedEvent(nullptr, (int*)&m_value + 1, false, nullptr);
		}
	}
#else
	if (_old + c_one <= 0)
	{
		futex((int*)&m_value.raw() + IS_BE_MACHINE, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
	}
	else if (s64 count = -_old / c_min)
	{
		futex((int*)&m_value.raw() + IS_LE_MACHINE, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr, nullptr, 0);
	}
#endif
}

void shared_mutex::imp_lock_upgrade()
{
	unlock_shared();
	lock();
}

void shared_mutex::imp_lock_degrade()
{
	unlock();
	lock_shared();
}

bool shared_mutex::try_lock_shared()
{
	// Conditional decrement
	return m_value.fetch_op([](s64& value) { if (value >= c_min) value -= c_min; }) >= c_min;
}

bool shared_mutex::try_lock()
{
	// Conditional decrement (TODO: obtain c_sig)
	return m_value.compare_and_swap_test(c_one, 0);
}

bool shared_mutex::try_lock_upgrade()
{
	// TODO
	return m_value.compare_and_swap_test(c_one - c_min, 0);
}

bool shared_mutex::try_lock_degrade()
{
	// TODO
	return m_value.compare_and_swap_test(0, c_one - c_min);
}
