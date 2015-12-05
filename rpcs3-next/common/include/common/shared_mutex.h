#pragma once
#include "basic_types.h"
#include "atomic.h"
#include <shared_mutex>

namespace common
{
	// An attempt to create lock-free (in optimistic case) implementation similar to std::shared_mutex;
	// MSVC implementation of std::shared_timed_mutex is not lock-free and thus may be slow, and std::shared_mutex is not available.
	class shared_mutex
	{
		struct ownership_info
		{
			u32 readers : 31;
			u32 writers : 1;
			u16 waiting_readers;
			u16 waiting_writers;
		};

		atomic<ownership_info> m_info{};

		std::mutex m_mutex;

		std::condition_variable m_rcv;
		std::condition_variable m_wcv;
		std::condition_variable m_wrcv;
		std::condition_variable m_wwcv;

	public:
		shared_mutex() = default;

		// Lock in shared mode
		void lock_shared();

		// Try to lock in shared mode
		bool try_lock_shared();

		// Unlock in shared mode
		void unlock_shared();

		// Lock exclusively
		void lock();

		// Try to lock exclusively
		bool try_lock();

		// Unlock exclusively
		void unlock();
	};
}