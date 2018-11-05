#pragma once

#include "types.h"
#include "Atomic.h"
#include <shared_mutex>

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

	bool imp_try_lock(u32 count);

	void imp_unlock(u32 count);

	u32 imp_notify(u32 count);

public:
	constexpr notifier() = default;

	bool try_lock()
	{
		return imp_try_lock(max_readers);
	}

	void unlock()
	{
		imp_unlock(max_readers);
	}

	bool try_lock_shared()
	{
		return imp_try_lock(1);
	}

	void unlock_shared()
	{
		imp_unlock(1);
	}

	bool wait(u64 usec_timeout = -1);

	void notify_all()
	{
		if (m_counter)
		{
			imp_notify(-1);
		}

		// Notify after imaginary "exclusive" lock+unlock
		m_cond.notify_all();
	}

	void notify_one()
	{
		// TODO
		if (m_counter)
		{
			if (imp_notify(1))
			{
				return;
			}
		}

		m_cond.notify_one();
	}

	static constexpr u32 max_readers = 0x7f;
};

class cond_one
{
	enum : u32
	{
		c_wait = 1,
		c_lock = 2,
		c_sig  = 3,
	};

	atomic_t<u32> m_value{0};

	bool imp_wait(u32 _old, u64 _timeout) noexcept;
	void imp_notify() noexcept;

public:
	constexpr cond_one() = default;

	void lock() noexcept
	{
		// Shouldn't be locked by more than one thread concurrently
		while (UNLIKELY(!m_value.compare_and_swap_test(0, c_lock)))
			;
	}

	void unlock() noexcept
	{
		m_value = 0;
	}

	bool wait(std::unique_lock<cond_one>& lock, u64 usec_timeout = -1) noexcept
	{
		AUDIT(lock.owns_lock());
		AUDIT(lock.mutex() == this);

		// State transition: c_sig -> c_lock, c_lock -> c_wait
		const u32 _old = m_value.fetch_sub(1);
		if (LIKELY(_old == c_sig))
			return true;

		return imp_wait(_old, usec_timeout);
	}

	void notify() noexcept
	{
		// Early exit if notification is not required
		if (LIKELY(!m_value))
			return;

		imp_notify();
	}
};
