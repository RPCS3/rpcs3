#include "sema.h"
#include "sync.h"

void semaphore_base::imp_wait(bool lsb)
{
	// 1. Obtain LSB, reset it
	// 2. Notify other posters if necessary

#ifdef _WIN32
	if (!lsb)
	{
		while ((m_value & 1) == 0 || !atomic_storage<s32>::btr(m_value.raw(), 0))
		{
			// Wait infinitely until signaled
			verify(HERE), NtWaitForKeyedEvent(nullptr, &m_value, false, nullptr) == ERROR_SUCCESS;
		}
	}

	// Notify instantly
	LARGE_INTEGER timeout;
	timeout.QuadPart = 0;
	if (HRESULT rc = NtReleaseKeyedEvent(nullptr, (u8*)&m_value + 2, false, &timeout))
	{
		verify(HERE), rc == WAIT_TIMEOUT;
	}
#elif __linux__
	if (!lsb)
	{
		for (s32 value = m_value; (m_value & 1) == 0 || !atomic_storage<s32>::btr(m_value.raw(), 0); value = m_value)
		{
			if (futex(&m_value.raw(), FUTEX_WAIT_BITSET_PRIVATE, value, nullptr, nullptr, -2) == -1)
			{
				verify(HERE), errno == EAGAIN;
			}
		}
	}

	verify(HERE), futex(&m_value.raw(), FUTEX_WAKE_BITSET_PRIVATE, 1, nullptr, nullptr, 1) >= 0;
#else
	if (lsb)
	{
		return;
	}

	while ((m_value & 1) == 0 || !atomic_storage<s32>::btr(m_value.raw(), 0))
	{
		std::this_thread::sleep_for(std::chrono::microseconds(50));
	}
#endif
}

void semaphore_base::imp_post(s32 _old)
{
	verify("semaphore_base: overflow" HERE), _old < 0;

	// 1. Set LSB, waiting until it can be set if necessary
	// 2. Notify waiter

#ifdef _WIN32
	while ((_old & 1) == 0 && atomic_storage<s32>::bts(m_value.raw(), 0))
	{
		static_assert(ERROR_SUCCESS == 0, "Unexpected ERROR_SUCCESS value");

		LARGE_INTEGER timeout;
		timeout.QuadPart = -500; // ~50us
		if (HRESULT rc = NtWaitForKeyedEvent(nullptr, (u8*)&m_value + 2, false, &timeout))
		{
			verify(HERE), rc == WAIT_TIMEOUT;
		}
	}

	LARGE_INTEGER timeout;
	timeout.QuadPart = 0; // Instant for the first attempt
	while (HRESULT rc = NtReleaseKeyedEvent(nullptr, &m_value, false, &timeout))
	{
		verify(HERE), rc == WAIT_TIMEOUT;

		if (!timeout.QuadPart)
		{
			timeout.QuadPart = -500; // ~50us
			NtDelayExecution(false, &timeout);
		}

		if ((m_value & 1) == 0)
		{
			break;
		}
	}
#elif __linux__
	for (s32 value = m_value; (_old & 1) == 0 && atomic_storage<s32>::bts(m_value.raw(), 0); value = m_value)
	{
		if (futex(&m_value.raw(), FUTEX_WAIT_BITSET_PRIVATE, value, nullptr, nullptr, 1) == -1)
		{
			verify(HERE), errno == EAGAIN;
		}
	}

	if (1 + 0 == verify(HERE, 1 + futex(&m_value.raw(), FUTEX_WAKE_BITSET_PRIVATE, 1, nullptr, nullptr, -2)))
	{
		usleep(50);
	}
#else
	if (_old & 1)
	{
		return;
	}

	while (m_value & 1 || atomic_storage<s32>::bts(m_value.raw(), 0))
	{
		std::this_thread::sleep_for(std::chrono::microseconds(50));
	}
#endif
}

bool semaphore_base::try_wait()
{
	// Conditional decrement
	const s32 value = m_value.fetch_op([](s32& value)
	{
		if (value > 0 || value & 1)
		{
			if (value <= 1)
			{
				value &= ~1;
			}

			value -= 1 << 1;
		}
	});

	if (value & 1 && value <= 1)
	{
		imp_wait(true);
		return true;
	}

	return value > 0 || value & 1;
}

bool semaphore_base::try_post(s32 _max)
{
	// Conditional increment
	const s32 value = m_value.fetch_op([&](s32& value)
	{
		if (value < _max)
		{
			if (value < 0)
			{
				value |= 1;
			}

			value += 1 << 1;
		}
	});

	if (value < 0)
	{
		imp_post(value ^ 1);
	}

	return value < _max;
}
