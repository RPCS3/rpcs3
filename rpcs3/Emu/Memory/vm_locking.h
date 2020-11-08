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
		/* flags (3 bits, RWX) */

		range_readable = 4ull << 61,
		range_writable = 2ull << 61,
		range_executable = 1ull << 61,
		range_full_mask = 7ull << 61,

		/* flag combinations with special meaning */

		range_locked = 1ull << 61, // R+W as well, but being exclusively accessed (size extends addr)
		range_allocation = 0, // Allocation, no safe access, g_shmem may change at ANY location

		range_pos = 61,
		range_bits = 3,
	};

	extern atomic_t<u64> g_range_lock;

	extern atomic_t<u64> g_shmem[];

	// Register reader
	void passive_lock(cpu_thread& cpu);

	// Register range lock for further use
	atomic_t<u64, 64>* alloc_range_lock();

	void range_lock_internal(atomic_t<u64, 64>* range_lock, u32 begin, u32 size);

	// Lock memory range
	template <uint Size = 0>
	FORCE_INLINE void range_lock(atomic_t<u64, 64>* range_lock, u32 begin, u32 _size)
	{
		const u32 size = Size ? Size : _size;
		const u64 lock_val = g_range_lock.load();
		const u64 is_share = g_shmem[begin >> 16].load();
		#ifndef _MSC_VER
		__asm__(""); // Tiny barrier
		#endif

		u64 lock_addr = static_cast<u32>(lock_val); // -> u64
		u32 lock_size = static_cast<u32>(lock_val << range_bits >> (32 + range_bits));

		u64 addr = begin;

		// Optimization: if range_locked is not used, the addr check will always pass
		// Otherwise, g_shmem is unchanged and its value is reliable to read
		if ((lock_val >> range_pos) == (range_locked >> range_pos)) [[likely]]
		{
			lock_size = 128;

			if (is_share) [[unlikely]]
			{
				addr = static_cast<u16>(begin) | is_share;
				lock_addr = lock_val;
			}
		}

		if (addr + size <= lock_addr || addr >= lock_addr + lock_size) [[likely]]
		{
			// Optimistic locking.
			// Note that we store the range we will be accessing, without any clamping.
			range_lock->store(begin | (u64{size} << 32));

			const u64 new_lock_val = g_range_lock.load();

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
