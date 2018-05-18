#pragma once

#include "types.h"
#include "Atomic.h"

// Lightweight condition variable
class cond_variable
{
	// Internal waiter counter
	atomic_t<u32> m_value{0};

	friend class notifier;

protected:
	// Internal waiting function
	bool imp_wait(u32 _old, u64 _timeout) noexcept;

	// Try to notify up to _count threads
	void imp_wake(u32 _count) noexcept;

public:
	constexpr cond_variable() = default;

	// Intrusive wait algorithm for lockable objects
	template <typename T>
	explicit_bool_t wait(T& object, u64 usec_timeout = -1)
	{
		const u32 _old = m_value.fetch_add(1); // Increment waiter counter
		object.unlock();
		const bool res = imp_wait(_old, usec_timeout);
		object.lock();
		return res;
	}

	// Wake one thread
	void notify_one() noexcept
	{
		if (m_value)
		{
			imp_wake(1);
		}
	}

	// Wake all threads
	void notify_all() noexcept
	{
		if (m_value)
		{
			imp_wake(-1);
		}
	}

	static constexpr u64 max_timeout = u64{UINT32_MAX} / 1000 * 1000000;
};

// Pair of a fake shared mutex (only limited shared locking) and a condition variable
class notifier
{
	atomic_t<u32> m_counter{0};
	cond_variable m_cond;

public:
	constexpr notifier() = default;

	void lock_shared()
	{
		m_counter++;
	}

	void unlock_shared()
	{
		const u32 counter = --m_counter;

		if (counter & 0x7f)
		{
			return;
		}

		if (counter >= 0x80)
		{
			const u32 _old = m_counter.atomic_op([](u32& value) -> u32
			{
				if (value & 0x7f)
				{
					return 0;
				}

				return std::exchange(value, 0) >> 7;
			});

			if (_old && m_cond.m_value)
			{
				m_cond.imp_wake(_old);
			}
		}
	}

	explicit_bool_t wait(u64 usec_timeout = -1)
	{
		const u32 _old = m_cond.m_value.fetch_add(1);

		if (0x80 <= m_counter.fetch_op([](u32& value)
		{
			value--;

			if (value >= 0x80)
			{
				value -= 0x80;
			}
		}))
		{
			// Return without waiting
			m_cond.imp_wait(_old, 0);
			m_counter++;
			return true;
		}

		const bool res = m_cond.imp_wait(_old, usec_timeout);
		m_counter++;
		return res;
	}

	void notify_all()
	{
		if (m_counter)
		{
			m_counter.atomic_op([](u32& value)
			{
				if (const u32 add = value & 0x7f)
				{
					// Mutex is locked in shared mode
					value += add << 7;
				}
				else
				{
					// Mutex is unlocked
					value = 0;
				}
			});
		}

		// Notify after imaginary "exclusive" lock+unlock
		m_cond.notify_all();
	}
};
