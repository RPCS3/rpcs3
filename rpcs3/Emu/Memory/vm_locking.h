#pragma once

#include "vm.h"
#include "Emu/RSX/rsx_utils.h"

class cpu_thread;
class shared_mutex;

namespace vm
{
	extern thread_local atomic_t<cpu_thread*>* g_tls_locked;

	enum range_lock_flags : u64
	{
		/* flags (3 bits, W + R + Reserved) */

		range_writable = 4ull << 61,
		range_readable = 2ull << 61,
		range_reserved = 1ull << 61,
		range_full_mask = 7ull << 61,

		/* flag combinations with special meaning */

		range_locked = 4ull << 61, // R+W as well, but being exclusively accessed (size extends addr)
		range_allocation = 0, // Allocation, no safe access, g_shmem may change at ANY location

		range_pos = 61,
		range_bits = 3,
	};

	extern atomic_t<u64, 64> g_range_lock_bits[2];

	extern atomic_t<u64> g_shmem[];

	// Register reader
	void passive_lock(cpu_thread& cpu);

	// Register range lock for further use
	atomic_t<u64, 64>* alloc_range_lock();

	void range_lock_internal(atomic_t<u64, 64>* range_lock, u32 begin, u32 size);

	// Lock memory range ignoring memory protection (Size!=0 also implies aligned begin)
	template <uint Size = 0>
	FORCE_INLINE void range_lock(atomic_t<u64, 64>* range_lock, u32 begin, u32 _size)
	{
		if constexpr (Size == 0)
		{
			if (begin >> 28 == rsx::constants::local_mem_base >> 28)
			{
				return;
			}
		}

		// Optimistic locking.
		// Note that we store the range we will be accessing, without any clamping.
		range_lock->store(begin | (u64{_size} << 32));

		// Old-style conditional constexpr
		const u32 size = Size ? Size : _size;

		if (Size == 1 || (begin % 4096 + size % 4096) / 4096 == 0 ? !vm::check_addr(begin) : !vm::check_addr(begin, vm::page_readable, size))
		{
			range_lock->release(0);
			range_lock_internal(range_lock, begin, _size);
			return;
		}

		#ifndef _MSC_VER
		__asm__(""); // Tiny barrier
		#endif

		if (!g_range_lock_bits[1]) [[likely]]
		{
			return;
		}

		// Fallback to slow path
		range_lock_internal(range_lock, begin, size);
	}

	// Release it
	void free_range_lock(atomic_t<u64, 64>*) noexcept;

	// Unregister reader
	void passive_unlock(cpu_thread& cpu);

	// Optimization (set cpu_flag::memory)
	bool temporary_unlock(cpu_thread& cpu) noexcept;
	void temporary_unlock() noexcept;

	struct writer_lock final
	{
		atomic_t<u64, 64>* range_lock;

		writer_lock(const writer_lock&) = delete;
		writer_lock& operator=(const writer_lock&) = delete;
		writer_lock() noexcept;
		writer_lock(u32 addr, atomic_t<u64, 64>* range_lock = nullptr, u32 size = 128, u64 flags = range_locked) noexcept;
		~writer_lock() noexcept;
	};
} // namespace vm
