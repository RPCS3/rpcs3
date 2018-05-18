#include "cond.h"
#include "sync.h"

#include <limits.h>

#ifndef _WIN32
#include <thread>
#endif

bool cond_variable::imp_wait(u32 _old, u64 _timeout) noexcept
{
	verify(HERE), _old != -1; // Very unlikely: it requires 2^32 distinct threads to wait simultaneously
	const bool is_inf = _timeout > max_timeout;

#ifdef _WIN32
	LARGE_INTEGER timeout;
	timeout.QuadPart = _timeout * -10;

	if (HRESULT rc = _timeout ? NtWaitForKeyedEvent(nullptr, &m_value, false, is_inf ? nullptr : &timeout) : WAIT_TIMEOUT)
	{
		verify(HERE), rc == WAIT_TIMEOUT;

		// Retire
		while (!m_value.fetch_op([](u32& value) { if (value) value--; }))
		{
			timeout.QuadPart = 0;

			if (HRESULT rc2 = NtWaitForKeyedEvent(nullptr, &m_value, false, &timeout))
			{
				verify(HERE), rc2 == WAIT_TIMEOUT;
				SwitchToThread();
				continue;
			}

			return true;
		}

		return false;
	}

	return true;
#else
	if (!_timeout)
	{
		verify(HERE), m_value--;
		return false;
	}

	timespec timeout;
	timeout.tv_sec  = _timeout / 1000000;
	timeout.tv_nsec = (_timeout % 1000000) * 1000;

	for (u32 value = _old + 1;; value = m_value)
	{
		const int err = futex((int*)&m_value.raw(), FUTEX_WAIT_PRIVATE, value, is_inf ? nullptr : &timeout, nullptr, 0) == 0
			? 0
			: errno;

		// Normal or timeout wakeup
		if (!err || (!is_inf && err == ETIMEDOUT))
		{
			// Cleanup (remove waiter)
			verify(HERE), m_value--;
			return !err;
		}

		// Not a wakeup
		verify(HERE), err == EAGAIN;
	}
#endif
}

void cond_variable::imp_wake(u32 _count) noexcept
{
#ifdef _WIN32
	// Try to subtract required amount of waiters
	const u32 count = m_value.atomic_op([=](u32& value)
	{
		if (value > _count)
		{
			value -= _count;
			return _count;
		}

		return std::exchange(value, 0);
	});

	for (u32 i = count; i > 0; i--)
	{
		NtReleaseKeyedEvent(nullptr, &m_value, false, nullptr);
	}
#else
	for (u32 i = _count; i > 0; std::this_thread::yield())
	{
		const u32 value = m_value;

		// Constrain remaining amount with imaginary waiter count
		if (i > value)
		{
			i = value;
		}

		if (!value || i == 0)
		{
			// Nothing to do
			return;
		}

		if (const int res = futex((int*)&m_value.raw(), FUTEX_WAKE_PRIVATE, i > INT_MAX ? INT_MAX : i, nullptr, nullptr, 0))
		{
			verify(HERE), res >= 0 && (u32)res <= i;
			i -= res;
		}

		if (!m_value || i == 0)
		{
			// Escape
			return;
		}
	}
#endif
}

bool notifier::imp_try_lock(u32 count)
{
	return m_counter.atomic_op([&](u32& value)
	{
		if ((value % (max_readers + 1)) + count <= max_readers)
		{
			value += count;
			return true;
		}

		return false;
	});
}

void notifier::imp_unlock(u32 count)
{
	const u32 counter = m_counter.sub_fetch(count);

	if (UNLIKELY(counter % (max_readers + 1)))
	{
		return;
	}

	if (counter)
	{
		const u32 _old = m_counter.atomic_op([](u32& value) -> u32
		{
			if (value % (max_readers + 1))
			{
				return 0;
			}

			return std::exchange(value, 0) / (max_readers + 1);
		});

		const u32 wc = m_cond.m_value;

		if (_old && wc)
		{
			m_cond.imp_wake(_old > wc ? wc : _old);
		}
	}
}

u32 notifier::imp_notify(u32 count)
{
	return m_counter.atomic_op([&](u32& value) -> u32
	{
		if (const u32 add = value % (max_readers + 1))
		{
			// Mutex is locked
			const u32 result = add > count ? count : add;
			value += result * (max_readers + 1);
			return result;
		}
		else
		{
			// Mutex is unlocked
			value = 0;
			return count;
		}
	});
}

explicit_bool_t notifier::wait(u64 usec_timeout)
{
	const u32 _old = m_cond.m_value.fetch_add(1);

	if (max_readers < m_counter.fetch_op([](u32& value)
	{
		if (value > max_readers)
		{
			value -= max_readers;
		}

		value -= 1;
	}))
	{
		// Return without waiting
		m_cond.imp_wait(_old, 0);
		return true;
	}

	const bool res = m_cond.imp_wait(_old, usec_timeout);

	while (!try_lock_shared())
	{
		// TODO
		busy_wait();
	}

	return res;
}
