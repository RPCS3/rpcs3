#pragma once

#include <cstdint>
#include <exception>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "Atomic.h"

//! An attempt to create effective implementation of "shared mutex", lock-free in optimistic case.
//! All locking and unlocking may be done by single LOCK XADD or LOCK CMPXCHG instructions.
//! MSVC implementation of std::shared_timed_mutex seems suboptimal.
//! std::shared_mutex is not available until C++17.
class shared_mutex final
{
	using ctrl_type = u32;

	enum : ctrl_type
	{
		SM_WRITER_LOCK = 1u << 31, // Exclusive lock flag, must be MSB
		SM_WAITERS_BIT = 1u << 30, // Flag set if m_wq_size or m_rq_size is non-zero
		SM_INVALID_BIT = 1u << 29, // Unreachable reader count bit (may be set by incorrect unlock_shared() call)

		SM_READER_MASK = SM_WAITERS_BIT - 1, // Valid reader count bit mask
		SM_READER_MAX  = 1u << 24, // Max reader count
	};

	atomic_t<ctrl_type> m_ctrl{}; // Control atomic variable: reader count | SM_* flags

	std::mutex m_mutex;

	std::size_t m_rq_size{}; // Reader queue size (threads waiting on m_rcv)
	std::size_t m_wq_size{}; // Writer queue size (threads waiting on m_wcv and m_ocv)
	
	std::condition_variable m_rcv; // Reader queue
	std::condition_variable m_wcv; // Writer queue
	std::condition_variable m_ocv; // For current exclusive owner

	void lock_shared_hard()
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		// Validate
		if ((m_ctrl & SM_INVALID_BIT) != 0) throw std::runtime_error("shared_mutex::lock_shared(): Invalid bit");
		if ((m_ctrl & SM_READER_MASK) == 0) throw std::runtime_error("shared_mutex::lock_shared(): No readers");

		// Notify non-zero reader queue size
		m_ctrl |= SM_WAITERS_BIT, m_rq_size++;

		// Fix excess reader count
		if ((--m_ctrl & SM_READER_MASK) == 0 && m_wq_size)
		{
			// Notify exclusive owner
			m_ocv.notify_one();
		}

		// Obtain the reader lock
		while (true)
		{
			const auto ctrl = m_ctrl.load();

			// Check writers and reader limit
			if (m_wq_size || (ctrl & ~SM_WAITERS_BIT) >= SM_READER_MAX)
			{
				m_rcv.wait(lock);
				continue;
			}

			if (m_ctrl.compare_and_swap_test(ctrl, ctrl + 1))
			{
				break;
			}
		}

		if (!--m_rq_size && !m_wq_size)
		{
			m_ctrl &= ~SM_WAITERS_BIT;
		}
	}

	void unlock_shared_notify()
	{
		// Mutex is locked for reliable notification because m_ctrl has been changed outside
		std::lock_guard<std::mutex> lock(m_mutex);

		if ((m_ctrl & SM_READER_MASK) == 0 && m_wq_size)
		{
			// Notify exclusive owner
			m_ocv.notify_one();
		}
		else if (m_rq_size)
		{
			// Notify other readers
			m_rcv.notify_one();
		}
	}

	void lock_hard()
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		// Validate
		if ((m_ctrl & SM_INVALID_BIT) != 0) throw std::runtime_error("shared_mutex::lock(): Invalid bit");

		// Notify non-zero writer queue size
		m_ctrl |= SM_WAITERS_BIT, m_wq_size++;

		// Obtain the writer lock
		while (true)
		{
			const auto ctrl = m_ctrl.load();

			if (ctrl & SM_WRITER_LOCK)
			{
				m_wcv.wait(lock);
				continue;
			}

			if (m_ctrl.compare_and_swap_test(ctrl, ctrl | SM_WRITER_LOCK))
			{
				break;
			}
		}

		// Wait for remaining readers
		while ((m_ctrl & SM_READER_MASK) != 0)
		{
			m_ocv.wait(lock);
		}

		if (!--m_wq_size && !m_rq_size)
		{
			m_ctrl &= ~SM_WAITERS_BIT;
		}
	}

	void unlock_notify()
	{
		// Mutex is locked for reliable notification because m_ctrl has been changed outside
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_wq_size)
		{
			// Notify next exclusive owner
			m_wcv.notify_one();
		}
		else if (m_rq_size)
		{
			// Notify all readers
			m_rcv.notify_all();
		}
	}

public:
	shared_mutex() = default;

	// Lock in shared mode
	void lock_shared()
	{
		if (m_ctrl++ >= SM_READER_MAX)
		{
			lock_shared_hard();
		}
	}

	// Try to lock in shared mode
	bool try_lock_shared()
	{
		auto ctrl = m_ctrl.load();

		return ctrl < SM_READER_MAX && m_ctrl.compare_and_swap_test(ctrl, ctrl + 1);
	}

	// Unlock in shared mode
	void unlock_shared()
	{
		if (m_ctrl-- >= SM_READER_MAX)
		{
			unlock_shared_notify();
		}
	}

	// Try to lock exclusively
	bool try_lock()
	{
		return m_ctrl.compare_and_swap_test(0, SM_WRITER_LOCK);
	}

	// Lock exclusively
	void lock()
	{
		if (m_ctrl.compare_and_swap_test(0, SM_WRITER_LOCK)) return;

		lock_hard();		
	}

	// Unlock exclusively
	void unlock()
	{
		if (m_ctrl.fetch_sub(SM_WRITER_LOCK) != SM_WRITER_LOCK)
		{
			unlock_notify();
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
