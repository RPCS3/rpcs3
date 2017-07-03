#include "cond.h"
#include "sync.h"

#include <limits.h>

#ifndef _WIN32
#include <thread>
#endif

bool cond_variable::imp_wait(u32 _old, u64 _timeout) noexcept
{
	verify(HERE), _old != -1; // Very unlikely: it requires 2^32 distinct threads to wait simultaneously

#ifdef _WIN32
	LARGE_INTEGER timeout;
	timeout.QuadPart = _timeout * -10;

	if (HRESULT rc = NtWaitForKeyedEvent(nullptr, &m_value, false, _timeout == -1 ? nullptr : &timeout))
	{
		verify(HERE), rc == WAIT_TIMEOUT;

		// Retire
		if (!m_value.fetch_op([](u32& value) { if (value) value--; }))
		{
			NtWaitForKeyedEvent(nullptr, &m_value, false, nullptr);
			return true;
		}

		return false;
	}

	return true;
#else
	timespec timeout;
	timeout.tv_sec  = _timeout / 1000000;
	timeout.tv_nsec = (_timeout % 1000000) * 1000;

	for (u32 value = _old + 1;; value = m_value)
	{
		const int err = futex((int*)&m_value.raw(), FUTEX_WAIT_PRIVATE, value, _timeout == -1 ? nullptr : &timeout, nullptr, 0) == 0
			? 0
			: errno;

		// Normal or timeout wakeup
		if (!err || (_timeout != -1 && err == ETIMEDOUT))
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
