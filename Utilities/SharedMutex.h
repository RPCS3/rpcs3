#pragma once

//! An attempt to create effective implementation of "shared mutex", lock-free in optimistic case.
//! All locking and unlocking may be done by single LOCK XADD or LOCK CMPXCHG instructions.
//! MSVC implementation of std::shared_timed_mutex seems suboptimal.
//! std::shared_mutex is not available until C++17.
class shared_mutex final
{
	enum : u32
	{
		SM_WRITER_LOCK  = 1u << 31, // Exclusive lock flag, must be MSB
		SM_WRITER_QUEUE = 1u << 30, // Flag set if m_wq_size != 0
		SM_READER_QUEUE = 1u << 29, // Flag set if m_rq_size != 0

		SM_READER_COUNT = SM_READER_QUEUE - 1, // Valid reader count bit mask
		SM_READER_MAX   = 1u << 24, // Max reader count
	};

	std::atomic<u32> m_ctrl{}; // Control atomic variable: reader count | SM_* flags
	std::thread::id m_owner{}; // Current exclusive owner (TODO: implement only for debug mode?)

	std::mutex m_mutex;

	u32 m_rq_size{}; // Reader queue size (threads waiting on m_rcv)
	u32 m_wq_size{}; // Writer queue size (threads waiting on m_wcv+m_ocv)
	
	std::condition_variable m_rcv; // Reader queue
	std::condition_variable m_wcv; // Writer queue
	std::condition_variable m_ocv; // For current exclusive owner

	static bool op_lock_shared(u32& ctrl)
	{
		// Check writer flags and reader limit
		return (ctrl & ~SM_READER_QUEUE) < SM_READER_MAX ? ctrl++, true : false;
	}

	static bool op_lock_excl(u32& ctrl)
	{
		// Test and set writer lock
		return (ctrl & SM_WRITER_LOCK) == 0 ? ctrl |= SM_WRITER_LOCK, true : false;
	}

	void impl_lock_shared(u32 old_ctrl);
	void impl_unlock_shared(u32 new_ctrl);
	void impl_lock_excl(u32 ctrl);
	void impl_unlock_excl(u32 ctrl);

public:
	shared_mutex() = default;

	// Lock in shared mode
	void lock_shared()
	{
		const u32 old_ctrl = m_ctrl++;

		// Check flags and reader limit
		if (old_ctrl >= SM_READER_MAX)
		{
			impl_lock_shared(old_ctrl);
		}
	}

	// Try to lock in shared mode
	bool try_lock_shared()
	{
		return atomic_op(m_ctrl, [](u32& ctrl)
		{
			// Check flags and reader limit
			return ctrl < SM_READER_MAX ? ctrl++, true : false;
		});
	}

	// Unlock in shared mode
	void unlock_shared()
	{
		const u32 new_ctrl = --m_ctrl;

		// Check if notification required
		if (new_ctrl >= SM_READER_MAX)
		{
			impl_unlock_shared(new_ctrl);
		}
	}

	// Lock exclusively
	void lock()
	{
		u32 value = 0;

		if (!m_ctrl.compare_exchange_strong(value, SM_WRITER_LOCK))
		{
			impl_lock_excl(value);
		}
	}

	// Try to lock exclusively
	bool try_lock()
	{
		u32 value = 0;

		return m_ctrl.compare_exchange_strong(value, SM_WRITER_LOCK);
	}

	// Unlock exclusively
	void unlock()
	{
		const u32 value = m_ctrl.fetch_add(SM_WRITER_LOCK);

		// Check if notification required
		if (value != SM_WRITER_LOCK)
		{
			impl_unlock_excl(value);
		}
	}
};

//! Simplified shared (reader) lock implementation, similar to std::lock_guard.
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
