#pragma once

#include "vm.h"

class cpu_thread;
class shared_mutex;

namespace vm
{
	extern shared_mutex g_mutex;

	extern thread_local atomic_t<cpu_thread*>* g_tls_locked;

	enum range_lock_flags : u64
	{
		/* flags (3 bits) */

		range_readable = 1ull << 32,
		range_writable = 2ull << 32,
		range_executable = 4ull << 32,
		range_all_mask = 7ull << 32,

		/* flag combinations with special meaning */

		range_normal = 3ull << 32, // R+W
		range_updated = 2ull << 32, // R+W as well but do not
		range_allocated = 4ull << 32, // No safe access
		range_deallocated = 0, // No safe access
	};

	extern atomic_t<u64> g_range_lock;

	extern atomic_t<u8> g_shareable[];

	// Register reader
	void passive_lock(cpu_thread& cpu);

	// Register range lock for further use
	atomic_t<u64, 64>* alloc_range_lock();

	void range_lock_internal(atomic_t<u64>* res, atomic_t<u64, 64>* range_lock, u32 begin, u32 size);

	// Lock memory range
	FORCE_INLINE void range_lock(atomic_t<u64>* res, atomic_t<u64, 64>* range_lock, u32 begin, u32 size)
	{
		const u64 lock_val = g_range_lock.load();
		const u64 lock_addr = static_cast<u32>(lock_val); // -> u64
		const u32 lock_size = static_cast<u32>(lock_val >> 35);
		const u64 res_val = res ? res->load() & 127 : 0;

		u64 addr = begin;

		if (g_shareable[begin >> 16])
		{
			addr = addr & 0xffff;
		}

		if ((addr + size <= lock_addr || addr >= lock_addr + lock_size) && !res_val) [[likely]]
		{
			// Optimistic locking
			range_lock->store(begin | (u64{size} << 32));

			const u64 new_lock_val = g_range_lock.load();
			const u64 new_res_val = res ? res->load() & 127 : 0;

			if (!new_lock_val && !new_res_val) [[likely]]
			{
				return;
			}

			if (new_lock_val == lock_val && !new_res_val) [[likely]]
			{
				return;
			}

			range_lock->release(0);
		}

		// Fallback to slow path
		range_lock_internal(res, range_lock, begin, size);
	}

	// Wait for all range locks to release in specified range
	void clear_range_locks(u32 addr, u32 size);

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
