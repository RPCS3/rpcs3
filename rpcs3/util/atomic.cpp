#include "atomic.hpp"

// TODO: something for other platforms
#if defined(__linux__) || !defined(_WIN32)
#define USE_FUTEX
#endif

#include "Utilities/sync.h"

#include <utility>
#include <mutex>
#include <condition_variable>
#include <iterator>
#include <memory>
#include <cstdlib>

// Hashtable size factor (can be set to 0 to stress-test collisions)
static constexpr uint s_hashtable_power = 16;

// Total number of entries, should be a power of 2.
static constexpr std::uintptr_t s_hashtable_size = 1u << s_hashtable_power;

// Pointer mask without bits used as hash, assuming signed 48-bit pointers.
static constexpr u64 s_pointer_mask = s_hashtable_power > 7 ? 0xffff'ffff'ffff & ~((s_hashtable_size - 1)) : 0xffff'ffff'ffff;

// Max number of waiters is 32767.
static constexpr u64 s_waiter_mask = s_hashtable_power > 7 ? 0x7fff'0000'0000'0000 : 0x7f00'0000'0000'0000;

// Bit indicates that more than one.
static constexpr u64 s_collision_bit = 0x8000'0000'0000'0000;

// Allocated slot with secondary table.
static constexpr u64 s_slot_mask = ~(s_waiter_mask | s_pointer_mask | s_collision_bit);

// Helper to get least significant set bit from 64-bit masks
template <u64 Mask>
static constexpr u64 one_v = Mask & (0 - Mask);

// Callback for wait() function, returns false if wait should return
static thread_local bool(*s_tls_wait_cb)(const void* data) = [](const void*){ return true; };

// Compare data in memory with old value, and return true if they are equal
template <bool CheckCb = true, bool CheckData = true>
static inline bool ptr_cmp(const void* data, std::size_t size, u64 old_value, u64 mask)
{
	if constexpr (CheckCb)
	{
		if (!s_tls_wait_cb(data))
		{
			return false;
		}
	}

	if constexpr (CheckData)
	{
		if (!data)
		{
			return false;
		}
	}

	switch (size)
	{
	case 1: return (reinterpret_cast<const atomic_t<u8>*>(data)->load() & mask) == (old_value & mask);
	case 2: return (reinterpret_cast<const atomic_t<u16>*>(data)->load() & mask) == (old_value & mask);
	case 4: return (reinterpret_cast<const atomic_t<u32>*>(data)->load() & mask) == (old_value & mask);
	case 8: return (reinterpret_cast<const atomic_t<u64>*>(data)->load() & mask) == (old_value & mask);
	}

	return false;
}

namespace
{
	struct sync_var
	{
		constexpr sync_var() noexcept = default;

		// Reference counter, owning pointer, collision bit and optionally selected slot
		atomic_t<u64> addr_ref{};

		// Allocated semaphore bits (max 60)
		atomic_t<u64> sema_bits{};

		// Semaphores (one per thread), data is platform-specific but 0 means empty
		atomic_t<u32> sema_data[60]{};

		atomic_t<u32>* sema_alloc()
		{
			const auto [bits, ok] = sema_bits.fetch_op([](u64& bits)
			{
				if (bits + 1 < (1ull << 60))
				{
					// Set lowest clear bit
					bits |= bits + 1;
					return true;
				}

				return false;
			});

			if (ok) [[likely]]
			{
				// Find lowest clear bit
				const auto sema = &sema_data[std::countr_one(bits)];

#if defined(USE_FUTEX) || defined(_WIN32)
				sema->release(1);
#endif

				return sema;
			}

			return nullptr;
		}

		void sema_free(atomic_t<u32>* sema)
		{
			if (sema < sema_data || sema >= std::end(sema_data))
			{
				std::abort();
			}

			// Clear sema
			sema->release(0);

			// Clear sema bit
			sema_bits &= ~(1ull << (sema - sema_data));
		}
	};
}

// Main hashtable for atomic wait.
alignas(64) static sync_var s_hashtable[s_hashtable_size]{};

namespace
{
	struct slot_info
	{
		constexpr slot_info() noexcept = default;

		// Branch extension
		sync_var branch[48 - s_hashtable_power]{};
	};
}

// Number of search groups (defines max slot branch count as gcount * 64)
static constexpr u32 s_slot_gcount = (s_hashtable_power > 7 ? 4096 : 256) / 64;

// Array of slot branch objects
alignas(64) static slot_info s_slot_list[s_slot_gcount * 64]{};

// Allocation bits
alignas(64) static atomic_t<u64> s_slot_bits[s_slot_gcount]{};

static u64 slot_alloc()
{
	// Diversify search start points to reduce contention and increase immediate success chance
#ifdef _WIN32
	const u32 start = GetCurrentProcessorNumber();
#elif __linux__
	const u32 start = sched_getcpu();
#else
	const u32 start = __rdtsc();
#endif

	for (u32 i = 0;; i++)
	{
		const u32 group = (i + start * 8) % s_slot_gcount;

		const auto [bits, ok] = s_slot_bits[group].fetch_op([](u64& bits)
		{
			if (~bits)
			{
				// Set lowest clear bit
				bits |= bits + 1;
				return true;
			}

			return false;
		});

		if (ok)
		{
			// Find lowest clear bit
			return group * 64 + std::countr_one(bits);
		}
	}

	// TODO: unreachable
	std::abort();
	return 0;
}

static sync_var* slot_get(std::uintptr_t iptr, sync_var* loc, u64 lv = 0)
{
	if (!loc)
	{
		return nullptr;
	}

	const u64 value = loc->addr_ref.load();

	if ((value & s_waiter_mask) == 0)
	{
		return nullptr;
	}

	if ((value & s_pointer_mask) == (iptr & s_pointer_mask))
	{
		return loc;
	}

	if ((value & s_collision_bit) == 0)
	{
		return nullptr;
	}

	// Get the number of leading equal bits to determine subslot
	const u64 eq_bits = std::countl_zero<u64>((((iptr ^ value) & (s_pointer_mask >> lv)) | ~s_pointer_mask) << 16);

	// Proceed recursively, increment level
	return slot_get(iptr, s_slot_list[(value & s_slot_mask) / one_v<s_slot_mask>].branch + eq_bits, eq_bits + 1);
}

static void slot_free(u64 id)
{
	// Reset allocation bit
	id = (id & s_slot_mask) / one_v<s_slot_mask>;
	s_slot_bits[id / 64] &= ~(1ull << (id % 64));
}

static void slot_free(std::uintptr_t iptr, sync_var* loc, u64 lv = 0)
{
	const u64 value = loc->addr_ref.load();

	if ((value & s_pointer_mask) != (iptr & s_pointer_mask))
	{
		if ((value & s_waiter_mask) == 0 || (value & s_collision_bit) == 0)
		{
			std::abort();
		}

		// Get the number of leading equal bits to determine subslot
		const u64 eq_bits = std::countl_zero<u64>((((iptr ^ value) & (s_pointer_mask >> lv)) | ~s_pointer_mask) << 16);

		// Proceed recursively, to deallocate deepest branch first
		slot_free(iptr, s_slot_list[(value & s_slot_mask) / one_v<s_slot_mask>].branch + eq_bits, eq_bits + 1);
	}

	// Actual cleanup in reverse order
	auto [_old, ok] = loc->addr_ref.fetch_op([&](u64& value)
	{
		if (value & s_waiter_mask)
		{
			value -= one_v<s_waiter_mask>;

			if (!(value & s_waiter_mask))
			{
				// Reset on last waiter
				value = 0;
				return 2;
			}

			return 1;
		}

		std::abort();
	});

	if (ok > 1 && _old & s_collision_bit)
	{
		if (loc->sema_bits)
			std::abort();

		// Deallocate slot on last waiter
		slot_free(_old);
	}
}

void atomic_storage_futex::wait(const void* data, std::size_t size, u64 old_value, u64 timeout, u64 mask)
{
	if (!timeout)
	{
		return;
	}

	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	// Allocated slot index
	u64 slot_a = -1;

	// Found slot object
	sync_var* slot = nullptr;

	auto install_op = [&](u64& value) -> u64
	{
		if ((value & s_waiter_mask) == s_waiter_mask)
		{
			// Return immediately on waiter overflow
			return 0;
		}

		if (!value || (value & s_pointer_mask) == (iptr & s_pointer_mask))
		{
			// Store pointer bits
			value |= (iptr & s_pointer_mask);
		}
		else
		{
			if ((value & s_collision_bit) == 0)
			{
				if (slot_a + 1 == 0)
				{
					// Second waiter: allocate slot and install it
					slot_a = slot_alloc() * one_v<s_slot_mask>;
				}

				value |= slot_a;
			}

			// Set collision bit
			value |= s_collision_bit;
		}

		// Add waiter
		value += one_v<s_waiter_mask>;
		return value;
	};

	// Search detail
	u64 lv = 0;

	for (sync_var* ptr = &s_hashtable[iptr % s_hashtable_size];;)
	{
		auto [_old, ok] = ptr->addr_ref.fetch_op(install_op);

		if (slot_a + 1)
		{
			if ((_old & s_collision_bit) == 0 && (ok & s_collision_bit) && (ok & s_slot_mask) == slot_a)
			{
				// Slot set successfully
				slot_a = -1;
			}
		}

		if (!ok)
		{
			// Expected only on top level
			if (timeout + 1 || ptr_cmp<false>(data, size, old_value, mask))
			{
				return;
			}

			// TODO
			busy_wait(30000);
			continue;
		}

		if (!_old || (_old & s_pointer_mask) == (iptr & s_pointer_mask))
		{
			// Success
			if (slot_a + 1)
			{
				// Cleanup slot if unused
				slot_free(slot_a);
				slot_a = -1;
			}

			slot = ptr;
			break;
		}

		// Get the number of leading equal bits (between iptr and slot owner)
		const u64 eq_bits = std::countl_zero<u64>((((iptr ^ ok) & (s_pointer_mask >> lv)) | ~s_pointer_mask) << 16);

		// Collision; need to go deeper
		ptr = s_slot_list[(ok & s_slot_mask) / one_v<s_slot_mask>].branch + eq_bits;

		lv = eq_bits + 1;
	}

	auto sema = slot->sema_alloc();

	while (!sema)
	{
		if (timeout + 1 || ptr_cmp<false>(data, size, old_value, mask))
		{
			slot_free(iptr, &s_hashtable[iptr % s_hashtable_size]);
			return;
		}

		// TODO
		busy_wait(30000);
		sema = slot->sema_alloc();
	}

	// Can skip unqueue process if true
#ifdef USE_FUTEX
	bool fallback = true;
#else
	bool fallback = false;
#endif

	while (ptr_cmp(data, size, old_value, mask))
	{
#ifdef USE_FUTEX
		struct timespec ts;
		ts.tv_sec  = timeout / 1'000'000'000;
		ts.tv_nsec = timeout % 1'000'000'000;

		if (sema->load() > 1) [[unlikely]]
		{
			// Signaled prematurely
			sema->release(1);
		}
		else
		{
			futex(sema, FUTEX_WAIT_PRIVATE, 1, timeout + 1 ? &ts : nullptr);
		}
#elif defined(_WIN32)
		LARGE_INTEGER qw;
		qw.QuadPart = -static_cast<s64>(timeout / 100);

		if (timeout % 100)
		{
			// Round up to closest 100ns unit
			qw.QuadPart -= 1;
		}

		if (fallback)
		{
			// Restart waiting
			sema->release(1);
			fallback = false;
		}

		if (!NtWaitForKeyedEvent(nullptr, sema, false, timeout + 1 ? &qw : nullptr))
		{
			// Error code assumed to be timeout
			fallback = true;
		}
#endif

		if (timeout + 1)
		{
			// TODO: reduce timeout instead
			break;
		}
	}

	while (!fallback)
	{
#if defined(_WIN32)
		static LARGE_INTEGER instant{};

		if (sema->compare_and_swap_test(1, 2))
		{
			// Succeeded in self-notifying
			break;
		}

		if (!NtWaitForKeyedEvent(nullptr, sema, false, &instant))
		{
			// Succeeded in obtaining an event without waiting
			break;
		}
#endif
	}

	slot->sema_free(sema);

	slot_free(iptr, &s_hashtable[iptr % s_hashtable_size]);

	s_tls_wait_cb(nullptr);
}

// Platform specific wake-up function
static inline bool alert_sema(atomic_t<u32>* sema)
{
#ifdef USE_FUTEX
	if (sema->load() == 1 && sema->compare_and_swap_test(1, 2))
	{
		// Use "wake all" arg for robustness, only 1 thread is expected
		futex(sema, FUTEX_WAKE_PRIVATE, 0x7fff'ffff);
		return true;
	}
#elif defined(_WIN32)
	if (sema->load() == 1 && sema->compare_and_swap_test(1, 2))
	{
		// Can wait in rare cases, which is its annoying weakness
		NtReleaseKeyedEvent(nullptr, sema, 1, nullptr);
		return true;
	}
#endif

	return false;
}

void atomic_storage_futex::set_wait_callback(bool(*cb)(const void* data))
{
	if (cb)
	{
		s_tls_wait_cb = cb;
	}
}

void atomic_storage_futex::raw_notify(const void* data)
{
	if (data)
	{
		notify_all(data);
	}
}

void atomic_storage_futex::notify_one(const void* data)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return;
	}

	for (u64 bits = slot->sema_bits; bits; bits &= bits - 1)
	{
		const auto sema = &slot->sema_data[std::countr_zero(bits)];

		if (alert_sema(sema))
		{
			break;
		}
	}
}

void atomic_storage_futex::notify_all(const void* data)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return;
	}

	for (u64 bits = slot->sema_bits; bits; bits &= bits - 1)
	{
		const auto sema = &slot->sema_data[std::countr_zero(bits)];

		if (alert_sema(sema))
		{
			continue;
		}
	}
}
