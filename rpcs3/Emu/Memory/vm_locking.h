#pragma once

#include "vm.h"

class cpu_thread;
class shared_mutex;

namespace vm
{
	extern shared_mutex g_mutex;

	extern thread_local atomic_t<cpu_thread*>* g_tls_locked;

	extern atomic_t<u64> g_addr_lock;

	// Register reader
	void passive_lock(cpu_thread& cpu);

	// Register range lock for further use
	atomic_t<u64, 64>* alloc_range_lock();

	void range_lock_internal(atomic_t<u64, 64>* range_lock, u32 begin, u32 size);

	// Lock memory range
	FORCE_INLINE void range_lock(atomic_t<u64, 64>* range_lock, u32 begin, u32 size)
	{
		const u64 lock_val = g_addr_lock.load();
		const u64 lock_addr = static_cast<u32>(lock_val); // -> u64
		const u32 lock_size = static_cast<u32>(lock_val >> 32);

		if (u64{begin} + size <= lock_addr || begin >= lock_addr + lock_size) [[likely]]
		{
			// Optimistic locking
			range_lock->release(begin | (u64{size} << 32));

			const u64 new_lock_val = g_addr_lock.load();

			if (!new_lock_val || new_lock_val == lock_val) [[likely]]
			{
				return;
			}

			range_lock->release(0);
		}

		// Fallback to slow path
		range_lock_internal(range_lock, begin, size);
	}

	// Release it
	void free_range_lock(atomic_t<u64, 64>*) noexcept;

	// Unregister reader
	void passive_unlock(cpu_thread& cpu);

	// Unregister reader (foreign thread)
	void cleanup_unlock(cpu_thread& cpu) noexcept;

	// Optimization (set cpu_flag::memory)
	void temporary_unlock(cpu_thread& cpu) noexcept;
	void temporary_unlock() noexcept;

	class reader_lock final
	{
		bool m_upgraded = false;

	public:
		reader_lock(const reader_lock&) = delete;
		reader_lock& operator=(const reader_lock&) = delete;
		reader_lock();
		~reader_lock();

		void upgrade();
	};

	struct writer_lock final
	{
		writer_lock(const writer_lock&) = delete;
		writer_lock& operator=(const writer_lock&) = delete;
		writer_lock(u32 addr = 0);
		~writer_lock();
	};
} // namespace vm
