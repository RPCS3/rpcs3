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

protected:
	// Internal waiting function
	bool imp_wait(u32 _old, u64 _timeout) noexcept;

	// Try to notify up to _count threads
	void imp_wake(u32 _count) noexcept;

public:
	constexpr cond_variable() = default;

	// Intrusive wait algorithm for lockable objects
	template <typename T>
	bool wait(T& object, u64 usec_timeout = -1)
	{
		const u32 _old = m_value.fetch_add(1); // Increment waiter counter
		object.unlock();
		const bool res = imp_wait(_old, usec_timeout);
		object.lock();
		return res;
	}

	// Unlock all specified objects but don't lock them again
	template <typename... Locks>
	bool wait_unlock(u64 usec_timeout, Locks&&... locks)
	{
		const u32 _old = m_value.fetch_add(1); // Increment waiter counter
		(..., std::forward<Locks>(locks).unlock());
		return imp_wait(_old, usec_timeout);
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
			imp_wake(65535);
		}
	}

	static constexpr u64 max_timeout = u64{UINT32_MAX} / 1000 * 1000000;
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

	u32 count() const noexcept
	{
		const u64 cvx32 = m_cvx32;
		return utils::popcnt32(static_cast<u32>(cvx32 | (cvx32 >> 32)));
	}

	bool wait(shared_lock const& lock, u64 usec_timeout = -1) noexcept
	{
		AUDIT(lock.m_this == this);
		return imp_wait(lock.m_slot, usec_timeout);
	}

	void wait_all() noexcept;

	bool wait_all(shared_lock& lock) noexcept;

	void notify_all() noexcept
	{
		if (LIKELY(!m_cvx32))
			return;

		imp_notify();
	}

	bool notify_all(shared_lock& lock) noexcept;
};
