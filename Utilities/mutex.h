#pragma once

#include "types.h"
#include "Atomic.h"

// Shared mutex.
class shared_mutex final
{
	enum : s64
	{
		c_one = 1ull << 31, // Fixed-point 1.0 value (one writer)
		c_min = 0x00000002, // Fixed-point 1.0/max_readers value
		c_sig = 0x00000001,
		c_max = c_one
	};

	atomic_t<s64> m_value{c_one}; // Semaphore-alike counter

	void imp_lock_shared(s64 _old);
	void imp_unlock_shared(s64 _old);

	void imp_lock(s64 _old);
	void imp_unlock(s64 _old);

	void imp_lock_upgrade();
	void imp_lock_degrade();

public:
	constexpr shared_mutex() = default;

	bool try_lock_shared();

	void lock_shared()
	{
		const s64 value = m_value.load();

		// Fast path: decrement if positive
		if (UNLIKELY(value < c_min || value > c_one || !m_value.compare_and_swap_test(value, value - c_min)))
		{
			imp_lock_shared(value);
		}
	}

	void unlock_shared()
	{
		// Unconditional increment
		const s64 value = m_value.fetch_add(c_min);

		if (value < 0 || value > c_one - c_min)
		{
			imp_unlock_shared(value);
		}
	}

	bool try_lock();

	void lock()
	{
		// Unconditional decrement
		const s64 value = m_value.fetch_sub(c_one);

		if (value != c_one)
		{
			imp_lock(value);
		}
	}

	void unlock()
	{
		// Unconditional increment
		const s64 value = m_value.fetch_add(c_one);

		if (value != 0)
		{
			imp_unlock(value);
		}
	}

	bool try_lock_upgrade();

	void lock_upgrade()
	{
		// TODO
		if (!m_value.compare_and_swap_test(c_one - c_min, 0))
		{
			imp_lock_upgrade();
		}
	}

	bool try_lock_degrade();

	void lock_degrade()
	{
		// TODO
		if (!m_value.compare_and_swap_test(0, c_one - c_min))
		{
			imp_lock_degrade();
		}
	}
};

// Simplified shared (reader) lock implementation.
class reader_lock final
{
	shared_mutex& m_mutex;

	void lock()
	{
		m_mutex.lock_shared();
	}

	void unlock()
	{
		m_mutex.unlock_shared();
	}

	friend class cond_variable;

public:
	reader_lock(const reader_lock&) = delete;

	explicit reader_lock(shared_mutex& mutex)
		: m_mutex(mutex)
	{
		lock();
	}

	~reader_lock()
	{
		unlock();
	}
};

// Simplified exclusive (writer) lock implementation.
class writer_lock final
{
	shared_mutex& m_mutex;

public:
	writer_lock(const writer_lock&) = delete;

	explicit writer_lock(shared_mutex& mutex)
		: m_mutex(mutex)
	{
		m_mutex.lock();
	}

	~writer_lock()
	{
		m_mutex.unlock();
	}
};

// Exclusive (writer) lock in the scope of shared (reader) lock (experimental).
class upgraded_lock final
{
	shared_mutex& m_mutex;

public:
	upgraded_lock(const writer_lock&) = delete;

	explicit upgraded_lock(shared_mutex& mutex)
		: m_mutex(mutex)
	{
		m_mutex.lock_upgrade();
	}

	~upgraded_lock()
	{
		m_mutex.lock_degrade();
	}
};

// Normal mutex with owner registration.
class owned_mutex
{
	atomic_t<u32> m_value{0};
	atomic_t<u32> m_owner{0};

protected:
	// Thread id
	static thread_local const u32 g_tid;

public:
	constexpr owned_mutex() = default;

	// Returns false if current thread already owns the mutex.
	bool lock() noexcept;

	// Returns false if locked by any thread.
	bool try_lock() noexcept;

	// Returns false if current thread doesn't own the mutex.
	bool unlock() noexcept;

	// Check state.
	bool is_locked() const { return m_value != 0; }

	// Check owner.
	bool is_owned() const { return m_owner == g_tid; }
};

// Recursive lock for owned_mutex (experimental).
class recursive_lock final
{
	owned_mutex& m_mutex;
	const bool m_first;

public:
	recursive_lock(const recursive_lock&) = delete;

	explicit recursive_lock(owned_mutex& mutex)
		: m_mutex(mutex)
		, m_first(mutex.lock())
	{
	}

	// Check whether the lock "owns" the mutex
	explicit operator bool() const
	{
		return m_first;
	}

	~recursive_lock()
	{
		if (m_first && !m_mutex.unlock())
		{
		}
	}
};
