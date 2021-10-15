#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Utilities/StrFmt.h"

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
				if ((val & 0x7f) == 0x7f) [[unlikely]]
					return;

				val++;
			});

			if ((prev & 0x7f) == 0x7f) [[unlikely]]
			{
				// Keep trying until counter can be incremented
				m_value.wait(0x7f, 0x7f);
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

		// Wait for signal bit
		m_value.wait(0, 0x80);
		m_value &= ~0x80;
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
			fmt::throw_exception("I tried to unlock unlocked mutex.");
		}

		// Set signal and notify
		if (prev & 0x7f)
		{
			m_value |= 0x80;
			m_value.notify_one(0x80);
		}

		if ((prev & 0x7f) == 0x7f) [[unlikely]]
		{
			// Overflow notify: value can be incremented
			m_value.notify_one(0x7f);
		}
	}

	bool is_free() const noexcept
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
