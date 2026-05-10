#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

// Lightweight condition variable
class cond_variable
{
	// Internal waiter counter
	atomic_t<u32> m_value{0};

	enum : u32
	{
		c_waiter_mask = 0x1fff,
		c_signal_mask = 0xffffffff & ~c_waiter_mask,
	};

protected:
	// Increment waiter count
	u32 add_waiter() noexcept
	{
		return m_value.atomic_op([](u32& value) -> u32
		{
			if ((value & c_signal_mask) == c_signal_mask || (value & c_waiter_mask) == c_waiter_mask)
			{
				// Signal or waiter overflow, return immediately
				return 0;
			}

			// Add waiter (c_waiter_mask)
			value += 1;
			return value;
		});
	}

	// Internal waiting function
	void imp_wait(u32 _old, u64 _timeout) noexcept;

	// Try to notify up to _count threads
	void imp_wake(u32 _count) noexcept;

public:
	constexpr cond_variable() = default;

	// Intrusive wait algorithm for lockable objects
	template <typename T>
	void wait(T& object, u64 usec_timeout = -1) noexcept
	{
		const u32 _old = add_waiter();

		if (!_old)
		{
			return;
		}

		object.unlock();
		imp_wait(_old, usec_timeout);
		object.lock();
	}

	// Unlock all specified objects but don't lock them again
	template <typename... Locks>
	void wait_unlock(u64 usec_timeout, Locks&&... locks)
	{
		const u32 _old = add_waiter();
		(..., std::forward<Locks>(locks).unlock());

		if (!_old)
		{
			return;
		}

		imp_wait(_old, usec_timeout);
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

	static constexpr u64 max_timeout = u64{umax} / 1000;
};
