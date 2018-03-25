#include "mutex.h"
#include "sync.h"

#include <climits>
#include <vector>
#include <algorithm>

// TLS variable for tracking owned mutexes
thread_local std::vector<shared_mutex*> g_tls_locks;

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

#ifdef _WIN32
	// Acquire writer lock
	imp_wait(m_value.load());

	// Convert to reader lock
	s64 value = m_value.fetch_add(c_one - c_min);

	// Proceed exclusively
	return;

	if (value != 0)
	{
		imp_unlock(value);
	}

	// Wait as a reader if necessary
	if (value + c_one - c_min < 0)
	{
		NtWaitForKeyedEvent(nullptr, (int*)&m_value + 1, false, nullptr);
	}
#else
	// Acquire writer lock
	imp_wait(0);

	// Convert to reader lock
	m_value += c_one - c_min;

	// Disabled code
	while (false)
	{
		const s64 value0 = m_value.fetch_op([](s64& value)
		{
			if (value >= c_min)
			{
				value -= c_min;
			}
		});

		if (value0 >= c_min)
		{
			return;
		}

		// Acquire writer lock
		imp_wait(value0);

		// Convert to reader lock
		s64 value1 = m_value.fetch_add(c_one - c_min);

		if (value1 != 0)
		{
			imp_unlock(value1);
		}

		value1 += c_one - c_min;

		if (value1 > 0)
		{
			return;
		}

		// Wait as a reader if necessary
		while (futex((int*)&m_value.raw() + IS_LE_MACHINE, FUTEX_WAIT_BITSET_PRIVATE, int(value1 >> 32), nullptr, nullptr, INT_MIN))
		{
			value1 = m_value.load();

			if (value1 >= 0)
			{
				return;
			}
		}

		// If blocked by writers, release the reader lock and try again
		const s64 value2 = m_value.fetch_op([](s64& value)
		{
			if (value < 0)
			{
				value += c_min;
			}
		});

		if (value2 >= 0)
		{
			return;
		}

		imp_unlock_shared(value2);
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
		m_value -= c_sig;

		futex((int*)&m_value.raw() + IS_LE_MACHINE, FUTEX_WAKE_BITSET_PRIVATE, 1, nullptr, nullptr, u32(c_sig >> 32));
#endif
	}
}

void shared_mutex::imp_wait(s64)
{
#ifdef _WIN32
	if (m_value.sub_fetch(c_one))
	{
		NtWaitForKeyedEvent(nullptr, &m_value, false, nullptr);
	}
#else
	if (!m_value.sub_fetch(c_one))
	{
		// Return immediately if locked
		return;
	}

	while (true)
	{
		// Load new value, try to acquire c_sig
		const s64 value = m_value.fetch_op([](s64& value)
		{
			if (value <= c_one - c_sig)
			{
				value += c_sig;
			}
		});

		if (value <= c_one - c_sig)
		{
			return;
		}

		futex((int*)&m_value.raw() + IS_LE_MACHINE, FUTEX_WAIT_BITSET_PRIVATE, int(value >> 32), nullptr, nullptr, u32(c_sig >> 32));
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
	else if (s64 count = -_old / c_min * 0)
	{
		// Disabled code
		while (count--)
		{
			NtReleaseKeyedEvent(nullptr, (int*)&m_value + 1, false, nullptr);
		}
	}
#else
	if (_old + c_one <= 0)
	{
		m_value -= c_sig;

		futex((int*)&m_value.raw() + IS_LE_MACHINE, FUTEX_WAKE_BITSET_PRIVATE, 1, nullptr, nullptr, u32(c_sig >> 32));
	}
	else if (false)
	{
		// Disabled code
		futex((int*)&m_value.raw() + IS_LE_MACHINE, FUTEX_WAKE_BITSET_PRIVATE, INT_MAX, nullptr, nullptr, INT_MIN);
	}
#endif
}

void shared_mutex::imp_lock_upgrade()
{
	// TODO
	unlock_shared();
	lock();
}

void shared_mutex::imp_lock_degrade()
{
	// TODO
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

safe_reader_lock::safe_reader_lock(shared_mutex& mutex)
	: m_mutex(mutex)
	, m_is_owned(false)
{
	if (std::count(g_tls_locks.cbegin(), g_tls_locks.cend(), &m_mutex) == 0)
	{
		m_is_owned = true;

		if (m_is_owned)
		{
			m_mutex.lock_shared();
			g_tls_locks.emplace_back(&m_mutex);
			return;
		}

		// TODO: order locks
	}
}

safe_reader_lock::~safe_reader_lock()
{
	if (m_is_owned)
	{
		m_mutex.unlock_shared();
		g_tls_locks.erase(std::remove(g_tls_locks.begin(), g_tls_locks.end(), &m_mutex), g_tls_locks.cend());
		return;
	}

	// TODO: order locks
}

safe_writer_lock::safe_writer_lock(shared_mutex& mutex)
	: m_mutex(mutex)
	, m_is_owned(false)
	, m_is_upgraded(false)
{
	if (std::count(g_tls_locks.cbegin(), g_tls_locks.cend(), &m_mutex) == 0)
	{
		m_is_owned = true;

		if (m_is_owned)
		{
			m_mutex.lock();
			g_tls_locks.emplace_back(&m_mutex);
			return;
		}

		// TODO: order locks
	}

	if (m_mutex.is_reading())
	{
		m_is_upgraded = true;
		m_mutex.lock_upgrade();
	}
}

safe_writer_lock::~safe_writer_lock()
{
	if (m_is_upgraded)
	{
		m_mutex.lock_degrade();
		return;
	}

	if (m_is_owned)
	{
		m_mutex.unlock();
		g_tls_locks.erase(std::remove(g_tls_locks.begin(), g_tls_locks.end(), &m_mutex), g_tls_locks.cend());
		return;
	}

	// TODO: order locks
}
