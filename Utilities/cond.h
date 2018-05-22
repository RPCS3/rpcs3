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

	explicit_bool_t wait(u64 usec_timeout = -1);

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
