#pragma once

#include "atomic.hpp"
#include <mutex>

// Pessimistic mutex for slow operation, does not spin wait, occupies only one byte
class slow_mutex
{
	atomic_t<u8> m_value{0};

public:
	constexpr slow_mutex() noexcept = default;

	void lock() noexcept
	{
		// Two-stage locking: increment, then wait
		while (true)
		{
			const u8 prev = m_value.fetch_op([](u8& val)
			{
				if (val == umax) [[unlikely]]
					return;

				val++;
			});

			if (prev == umax) [[unlikely]]
			{
				// Keep trying until counter can be incremented
				m_value.wait(0xff, 0x01);
			}
			else if (prev == 0)
			{
				// Locked
				return;
			}
			else
			{
				// Normal waiting
				break;
			}
		}

		// Wait for 7 bits to become 0, which could only mean one thing
		m_value.wait<atomic_wait::op_ne>(0, 0xfe);
	}

	bool try_lock() noexcept
	{
		return m_value.compare_and_swap_test(0, 1);
	}

	void unlock() noexcept
	{
		const u8 prev = m_value.fetch_op([](u8& val)
		{
			if (val) [[likely]]
				val--;
		});

		if (prev == 0) [[unlikely]]
		{
			fmt::raw_error("I tried to unlock unlocked mutex." HERE);
		}

		// Normal notify with forced value (ignoring real waiter count)
		m_value.notify_one(0xfe, 0);

		if (prev == umax) [[unlikely]]
		{
			// Overflow notify: value can be incremented
			m_value.notify_one(0x01, 0);
		}
	}

	bool is_free() noexcept
	{
		return !m_value;
	}

	void lock_unlock() noexcept
	{
		if (m_value)
		{
			lock();
			unlock();
		}
	}
};
