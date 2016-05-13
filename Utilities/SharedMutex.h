#pragma once

#include "types.h"
#include "Atomic.h"
#include "Platform.h"

//! An attempt to create effective implementation of "shared mutex", lock-free in optimistic case.
//! All locking and unlocking may be done by a single LOCK XADD or LOCK CMPXCHG instruction.
//! MSVC implementation of std::shared_timed_mutex seems suboptimal.
//! std::shared_mutex is not available until C++17.
class shared_mutex final
{
	enum : u32
	{
		SM_WRITER_LOCK = 1u << 31, // Exclusive lock flag, must be MSB
		SM_WAITERS_BIT = 1u << 30, // Flag set if m_wq_size or m_rq_size is non-zero
		SM_INVALID_BIT = 1u << 29, // Unreachable reader count bit (may be set by incorrect unlock_shared() call)

		SM_READER_MASK = SM_WAITERS_BIT - 1, // Valid reader count bit mask
		SM_READER_MAX  = 1u << 24, // Max reader count
	};

	atomic_t<u32> m_ctrl{}; // Control variable: reader count | SM_* flags

	struct internal;

	atomic_t<internal*> m_data{}; // Internal data

	void lock_shared_hard();
	void unlock_shared_notify();

	void lock_hard();
	void unlock_notify();

	void lock_upgrade_hard();
	void lock_degrade_hard();

public:
	constexpr shared_mutex() = default;

	// Initialize internal data
	void initialize_once();

	~shared_mutex();

	bool try_lock_shared()
	{
		const u32 ctrl = m_ctrl.load();

		return ctrl < SM_READER_MAX && m_ctrl.compare_and_swap_test(ctrl, ctrl + 1);
	}

	void lock_shared()
	{
		// Optimization: unconditional increment, compensated later
		if (UNLIKELY(m_ctrl++ >= SM_READER_MAX))
		{
			lock_shared_hard();
		}
	}

	void unlock_shared()
	{
		if (UNLIKELY(m_ctrl-- >= SM_READER_MAX))
		{
			unlock_shared_notify();
		}
	}

	bool try_lock()
	{
		return !m_ctrl && m_ctrl.compare_and_swap_test(0, SM_WRITER_LOCK);
	}

	void lock()
	{
		if (UNLIKELY(!m_ctrl.compare_and_swap_test(0, SM_WRITER_LOCK)))
		{
			lock_hard();
		}
	}

	void unlock()
	{
		m_ctrl &= ~SM_WRITER_LOCK;

		if (UNLIKELY(m_ctrl))
		{
			unlock_notify();
		}
	}

	bool try_lock_upgrade()
	{
		return m_ctrl == 1 && m_ctrl.compare_and_swap_test(1, SM_WRITER_LOCK);
	}

	bool try_lock_degrade()
	{
		return m_ctrl == SM_WRITER_LOCK && m_ctrl.compare_and_swap_test(SM_WRITER_LOCK, 1);
	}

	void lock_upgrade()
	{
		if (UNLIKELY(!m_ctrl.compare_and_swap_test(1, SM_WRITER_LOCK)))
		{
			lock_upgrade_hard();
		}
	}

	void lock_degrade()
	{
		if (UNLIKELY(!m_ctrl.compare_and_swap_test(SM_WRITER_LOCK, 1)))
		{
			lock_degrade_hard();
		}
	}
};

//! Simplified shared (reader) lock implementation.
//! std::shared_lock may be used instead if necessary.
class reader_lock final
{
	shared_mutex& m_mutex;

public:
	reader_lock(const reader_lock&) = delete;

	reader_lock(shared_mutex& mutex)
		: m_mutex(mutex)
	{
		m_mutex.lock_shared();
	}

	~reader_lock()
	{
		m_mutex.unlock_shared();
	}
};

//! Simplified exclusive (writer) lock implementation.
//! std::lock_guard may or std::unique_lock be used instead if necessary.
class writer_lock final
{
	shared_mutex& m_mutex;

public:
	writer_lock(const writer_lock&) = delete;

	writer_lock(shared_mutex& mutex)
		: m_mutex(mutex)
	{
		m_mutex.lock();
	}

	~writer_lock()
	{
		m_mutex.unlock();
	}
};

// Exclusive (writer) lock in the scope of shared (reader) lock.
class upgraded_lock final
{
	shared_mutex& m_mutex;

public:
	upgraded_lock(const writer_lock&) = delete;

	upgraded_lock(shared_mutex& mutex)
		: m_mutex(mutex)
	{
		m_mutex.lock_upgrade();
	}

	~upgraded_lock()
	{
		m_mutex.lock_degrade();
	}
};
