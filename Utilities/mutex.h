#pragma once

#include <mutex>
#include "util/types.hpp"
#include "util/atomic.hpp"

// Shared mutex with small size (u32).
class shared_mutex final
{
	enum : u32
	{
		c_one = 1u << 14, // Fixed-point 1.0 value (one writer, max_readers = c_one - 1)
		c_sig = 1u << 30,
		c_err = 1u << 31,
	};

	atomic_t<u32> m_value{};

	void imp_lock_shared(u32 val);
	void imp_unlock_shared(u32 old);
	void imp_wait();
	void imp_signal();
	void imp_lock(u32 val);
	void imp_unlock(u32 old);
	void imp_lock_upgrade();
	void imp_lock_unlock();

public:
	constexpr shared_mutex() = default;

	bool try_lock_shared()
	{
		const u32 value = m_value.load();

		// Conditional increment
		return value < c_one - 1 && m_value.compare_and_swap_test(value, value + 1);
	}

	void lock_shared()
	{
		const u32 value = m_value.load();

		if (value >= c_one - 1 || !m_value.compare_and_swap_test(value, value + 1)) [[unlikely]]
		{
			imp_lock_shared(value);
		}
	}

	void lock_shared_hle()
	{
		const u32 value = m_value.load();

		if (value < c_one - 1) [[likely]]
		{
			u32 old = value;
			if (atomic_storage<u32>::compare_exchange_hle_acq(m_value.raw(), old, value + 1)) [[likely]]
			{
				return;
			}
		}

		imp_lock_shared(value);
	}

	void unlock_shared()
	{
		// Unconditional decrement (can result in broken state)
		const u32 value = m_value.fetch_sub(1);

		if (value >= c_one) [[unlikely]]
		{
			imp_unlock_shared(value);
		}
	}

	void unlock_shared_hle()
	{
		const u32 value = atomic_storage<u32>::fetch_add_hle_rel(m_value.raw(), -1);

		if (value >= c_one) [[unlikely]]
		{
			imp_unlock_shared(value);
		}
	}

	bool try_lock()
	{
		return m_value.compare_and_swap_test(0, c_one);
	}

	void lock()
	{
		const u32 value = m_value.compare_and_swap(0, c_one);

		if (value) [[unlikely]]
		{
			imp_lock(value);
		}
	}

	void lock_hle()
	{
		u32 value = 0;

		if (!atomic_storage<u32>::compare_exchange_hle_acq(m_value.raw(), value, c_one)) [[unlikely]]
		{
			imp_lock(value);
		}
	}

	void unlock()
	{
		// Unconditional decrement (can result in broken state)
		const u32 value = m_value.fetch_sub(c_one);

		if (value != c_one) [[unlikely]]
		{
			imp_unlock(value);
		}
	}

	void unlock_hle()
	{
		const u32 value = atomic_storage<u32>::fetch_add_hle_rel(m_value.raw(), ~c_one + 1);

		if (value != c_one) [[unlikely]]
		{
			imp_unlock(value);
		}
	}

	bool try_lock_upgrade()
	{
		const u32 value = m_value.load();

		// Conditional increment, try to convert a single reader into a writer, ignoring other writers
		return (value + c_one - 1) % c_one == 0 && m_value.compare_and_swap_test(value, value + c_one - 1);
	}

	void lock_upgrade()
	{
		if (!try_lock_upgrade()) [[unlikely]]
		{
			imp_lock_upgrade();
		}
	}

	void lock_downgrade()
	{
		// Convert to reader lock (can result in broken state)
		m_value -= c_one - 1;
	}

	// Optimized wait for lockability without locking, relaxed
	void lock_unlock()
	{
		if (m_value != 0) [[unlikely]]
		{
			imp_lock_unlock();
		}
	}

	// Check whether can immediately obtain an exclusive (writer) lock
	bool is_free() const
	{
		return m_value.load() == 0;
	}

	// Check whether can immediately obtain a shared (reader) lock
	bool is_lockable() const
	{
		return m_value.load() < c_one - 1;
	}

	bool has_waiters() const
	{
		return m_value.load() > c_one;
	}
};

// Simplified shared (reader) lock implementation.
class reader_lock final
{
	shared_mutex& m_mutex;
	bool m_upgraded = false;

public:
	reader_lock(const reader_lock&) = delete;

	reader_lock& operator=(const reader_lock&) = delete;

	explicit reader_lock(shared_mutex& mutex)
		: m_mutex(mutex)
	{
		m_mutex.lock_shared();
	}

	// One-way lock upgrade; note that the observed state could have been changed
	void upgrade()
	{
		if (!m_upgraded)
		{
			m_mutex.lock_upgrade();
			m_upgraded = true;
		}
	}

	// Try to upgrade; if it succeeds, the observed state has NOT been changed
	bool try_upgrade()
	{
		return m_upgraded || (m_upgraded = m_mutex.try_lock_upgrade());
	}

	~reader_lock()
	{
		m_upgraded ? m_mutex.unlock() : m_mutex.unlock_shared();
	}
};
