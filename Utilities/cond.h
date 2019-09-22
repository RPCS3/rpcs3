#pragma once

#include "types.h"
#include "util/atomic.hpp"
#include <shared_mutex>
#include "asm.h"

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

	static constexpr u64 max_timeout = UINT64_MAX / 1000;
};

// Condition variable fused with a pseudo-mutex supporting only reader locks (up to 32 readers).
class shared_cond
{
	// For information, shouldn't modify
	enum : u64
	{
		// Wait bit is aligned for compatibility with 32-bit futex.
		c_wait = 1,
		c_sig  = 1ull << 32,
		c_lock = 1ull << 32 | 1,
	};

	// Split in 32-bit parts for convenient bit combining
	atomic_t<u64> m_cvx32{0};

	class shared_lock
	{
		shared_cond* m_this;
		u32 m_slot;

		friend class shared_cond;

	public:
		shared_lock(shared_cond* _this) noexcept
			: m_this(_this)
		{
			// Lock and remember obtained slot index
			m_slot = m_this->m_cvx32.atomic_op([](u64& cvx32)
			{
				// Combine used bits and invert to find least significant bit unused
				const u32 slot = static_cast<u32>(utils::cnttz64(~((cvx32 & 0xffffffff) | (cvx32 >> 32)), true));

				// Set lock bits (does nothing if all slots are used)
				const u64 bit = (1ull << slot) & 0xffffffff;
				cvx32 |= bit | (bit << 32);
				return slot;
			});
		}

		shared_lock(const shared_lock&) = delete;

		shared_lock(shared_lock&& rhs)
			: m_this(rhs.m_this)
			, m_slot(rhs.m_slot)
		{
			rhs.m_slot = 32;
		}

		shared_lock& operator=(const shared_lock&) = delete;

		~shared_lock()
		{
			// Clear the slot (does nothing if all slots are used)
			const u64 bit = (1ull << m_slot) & 0xffffffff;
			m_this->m_cvx32 &= ~(bit | (bit << 32));
		}

		explicit operator bool() const noexcept
		{
			// Check success
			return m_slot < 32;
		}

		bool wait(u64 usec_timeout = -1) const noexcept
		{
			return m_this->wait(*this, usec_timeout);
		}
	};

	bool imp_wait(u32 slot, u64 _timeout) noexcept;
	void imp_notify() noexcept;

public:
	constexpr shared_cond() = default;

	shared_lock try_shared_lock() noexcept
	{
		return shared_lock(this);
	}

	bool wait(shared_lock const& lock, u64 usec_timeout = -1) noexcept
	{
		AUDIT(lock.m_this == this);
		return imp_wait(lock.m_slot, usec_timeout);
	}

	void notify_all() noexcept
	{
		if (LIKELY(!m_cvx32))
			return;

		imp_notify();
	}
};
