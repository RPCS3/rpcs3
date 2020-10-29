﻿#include "atomic.hpp"

#if defined(__linux__)
#define USE_FUTEX
#elif !defined(_WIN32)
#define USE_STD
#endif

#include "Utilities/sync.h"

#include <utility>
#include <mutex>
#include <condition_variable>
#include <chrono>
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

// Callback for notification functions for optimizations
static thread_local void(*s_tls_notify_cb)(const void* data, u64 progress) = [](const void*, u64){};

// Compare data in memory with old value, and return true if they are equal
template <bool CheckCb = true, bool CheckData = true>
static inline bool
#ifdef _WIN32
__vectorcall
#endif
ptr_cmp(const void* data, std::size_t size, __m128i old128, __m128i mask128)
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

	const u64 old_value = _mm_cvtsi128_si64(old128);
	const u64 mask = _mm_cvtsi128_si64(mask128);

	switch (size)
	{
	case 1: return (reinterpret_cast<const atomic_t<u8>*>(data)->load() & mask) == (old_value & mask);
	case 2: return (reinterpret_cast<const atomic_t<u16>*>(data)->load() & mask) == (old_value & mask);
	case 4: return (reinterpret_cast<const atomic_t<u32>*>(data)->load() & mask) == (old_value & mask);
	case 8: return (reinterpret_cast<const atomic_t<u64>*>(data)->load() & mask) == (old_value & mask);
	case 16:
	{
		const auto v0 = _mm_load_si128(reinterpret_cast<const __m128i*>(data));
		const auto v1 = _mm_xor_si128(v0, old128);
		const auto v2 = _mm_and_si128(v1, mask128);
		const auto v3 = _mm_packs_epi16(v2, v2);

		if (_mm_cvtsi128_si64(v3) == 0)
		{
			return true;
		}
	}
	}

	return false;
}

#ifdef USE_STD
namespace
{
	// Standard CV/mutex pair
	struct cond_handle
	{
		std::condition_variable cond;
		std::mutex mtx;

		cond_handle() noexcept
		{
			mtx.lock();
		}
	};
}

// Arbitrary max allowed thread number
static constexpr u32 s_max_conds = 512 * 64;

static std::aligned_storage_t<sizeof(cond_handle), alignof(cond_handle)> s_cond_list[s_max_conds]{};

atomic_t<u64, 64> s_cond_bits[s_max_conds / 64];

atomic_t<u32, 64> s_cond_sema{0};

static u32 cond_alloc()
{
	// Determine whether there is a free slot or not
	if (!s_cond_sema.try_inc(s_max_conds + 1))
	{
		return 0;
	}

	// Diversify search start points to reduce contention and increase immediate success chance
#ifdef _WIN32
	const u32 start = GetCurrentProcessorNumber();
#elif __linux__
	const u32 start = sched_getcpu();
#else
	const u32 start = __rdtsc();
#endif

	for (u32 i = start * 8;; i++)
	{
		const u32 group = i % (s_max_conds / 64);

		const auto [bits, ok] = s_cond_bits[group].fetch_op([](u64& bits)
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
			const u32 id = group * 64 + std::countr_one(bits);

			// Construct inplace before it can be used
			new (s_cond_list + id) cond_handle();

			return id + 1;
		}
	}

	// TODO: unreachable
	std::abort();
	return 0;
}

static cond_handle* cond_get(u32 cond_id)
{
	if (cond_id - 1 < s_max_conds) [[likely]]
	{
		return std::launder(reinterpret_cast<cond_handle*>(s_cond_list + (cond_id - 1)));
	}

	return nullptr;
}

static void cond_free(u32 cond_id)
{
	if (cond_id - 1 >= s_max_conds)
	{
		// Ignore bad id because it may contain notifier lock
		return;
	}

	// Call the destructor
	cond_get(cond_id)->~cond_handle();

	// Remove the allocation bit
	s_cond_bits[(cond_id - 1) / 64] &= ~(1ull << ((cond_id - 1) % 64));

	// Release the semaphore
	s_cond_sema--;
}
#endif

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
#ifdef USE_STD
			const u32 cond_id = cond_alloc();

			if (cond_id == 0)
			{
				// Too many threads
				return nullptr;
			}
#endif

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

#if defined(USE_STD)
				sema->release(cond_id);
#elif defined(USE_FUTEX)
				sema->release(1);
#elif defined(_WIN32)
				if (NtWaitForAlertByThreadId)
				{
					sema->release(GetCurrentThreadId());
				}
				else
				{
					sema->release(1);
				}
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
#ifdef USE_STD
			cond_free(sema->exchange(0));
#else
			sema->release(0);
#endif
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
static atomic_t<u64, 64> s_slot_bits[s_slot_gcount]{};

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

SAFE_BUFFERS void
#ifdef _WIN32
__vectorcall
#endif
atomic_storage_futex::wait(const void* data, std::size_t size, __m128i old_value, u64 timeout, __m128i mask)
{
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

#ifdef _WIN32
	// May be used by NtWaitForAlertByThreadId
	u32 thread_id[16]{GetCurrentThreadId()};
#endif

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

#ifdef USE_STD
	// Create mutex for condition variable (already locked)
	std::unique_lock lock(cond_get(sema->load() & 0x7fffffff)->mtx, std::adopt_lock);
#endif

	// Can skip unqueue process if true
#if defined(USE_FUTEX) || defined(USE_STD)
	constexpr bool fallback = true;
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
#elif defined(USE_STD)
		const u32 val = sema->load();

		if (val >> 31)
		{
			// Locked by notifier
			if (!ptr_cmp(data, size, old_value, mask))
			{
				break;
			}
		}
		else if (timeout + 1)
		{
			cond_get(val)->cond.wait_for(lock, std::chrono::nanoseconds(timeout));
		}
		else
		{
			cond_get(val)->cond.wait(lock);
		}
#elif defined(_WIN32)
		LARGE_INTEGER qw;
		qw.QuadPart = -static_cast<s64>(timeout / 100);

		if (timeout % 100)
		{
			// Round up to closest 100ns unit
			qw.QuadPart -= 1;
		}

		if (NtWaitForAlertByThreadId)
		{
			if (fallback) [[unlikely]]
			{
				// Restart waiting
				if (sema->load() == umax)
				{
					sema->release(thread_id[0]);
				}

				fallback = false;
			}

			// Let's assume it can return spuriously
			switch (DWORD status = NtWaitForAlertByThreadId(thread_id, timeout + 1 ? &qw : nullptr))
			{
			case NTSTATUS_ALERTED: fallback = true; break;
			case NTSTATUS_TIMEOUT: break;
			default:
			{
				SetLastError(status);
				fmt::raw_verify_error("Unexpected NtWaitForAlertByThreadId result.", nullptr, 0);
			}
			}
		}
		else
		{
			if (fallback)
			{
				// Restart waiting
				verify(HERE), sema->load() == 2;
				sema->release(1);
				fallback = false;
			}

			if (!NtWaitForKeyedEvent(nullptr, sema, false, timeout + 1 ? &qw : nullptr))
			{
				// Error code assumed to be timeout
				fallback = true;
			}
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

		if (NtWaitForAlertByThreadId)
		{
			if (sema->compare_and_swap_test(thread_id[0], -1))
			{
				break;
			}

			if (NtWaitForAlertByThreadId(thread_id, &instant) == NTSTATUS_ALERTED)
			{
				break;
			}

			continue;
		}

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

#ifdef _WIN32
	verify(HERE), thread_id[0] == GetCurrentThreadId();
#endif

#ifdef USE_STD
	lock.unlock();
#endif

	slot->sema_free(sema);

	slot_free(iptr, &s_hashtable[iptr % s_hashtable_size]);

	s_tls_wait_cb(nullptr);
}

// Platform specific wake-up function
static inline bool alert_sema(atomic_t<u32>* sema, const void* data, u64 progress)
{
#ifdef USE_FUTEX
	if (sema->load() == 1 && sema->compare_and_swap_test(1, 2))
	{
		if (!progress)
		{
			// Imminent notification
			s_tls_notify_cb(data, 0);
		}

		// Use "wake all" arg for robustness, only 1 thread is expected
		futex(sema, FUTEX_WAKE_PRIVATE, 0x7fff'ffff);
		return true;
	}
#elif defined(USE_STD)
	// Check if not zero and not locked
	u32 old_val = sema->load();

	if (((old_val - 1) >> 31) == 0)
	{
		const auto [cond_id, ok] = sema->fetch_op([](u32& id)
		{
			if ((id - 1) >> 31)
			{
				return false;
			}

			// Set notify lock
			id |= 1u << 31;
			return true;
		});

		if (ok)
		{
			if (auto cond = cond_get(cond_id))
			{
				if (!progress)
				{
					// Imminent notification
					s_tls_notify_cb(data, 0);
				}

				// Not super efficient: locking is required to avoid lost notifications
				cond->mtx.lock();
				cond->mtx.unlock();
				cond->cond.notify_all();

				// Try to remove notifier lock gracefully
				if (!sema->compare_and_swap_test(cond_id | (1u << 31), cond_id)) [[unlikely]]
				{
					// Cleanup helping
					cond_free(cond_id);
					return false;
				}

				return true;
			}
		}
	}
#elif defined(_WIN32)
	if (NtWaitForAlertByThreadId)
	{
		u32 tid = sema->load();

		// Check if tid is neither 0 nor -1
		if (tid + 1 > 1 && sema->compare_and_swap_test(tid, -1))
		{
			if (!progress)
			{
				// Imminent notification
				s_tls_notify_cb(data, 0);
			}

			if (NtAlertThreadByThreadId(tid) == NTSTATUS_SUCCESS)
			{
				// Could be some dead thread otherwise
				return true;
			}
		}

		return false;
	}

	if (sema->load() == 1 && sema->compare_and_swap_test(1, 2))
	{
		if (!progress)
		{
			// Imminent notification
			s_tls_notify_cb(data, 0);
		}

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
	else
	{
		s_tls_wait_cb = [](const void*){ return true; };
	}
}

void atomic_storage_futex::set_notify_callback(void(*cb)(const void*, u64))
{
	if (cb)
	{
		s_tls_notify_cb = cb;
	}
	else
	{
		s_tls_notify_cb = [](const void*, u64){};
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

	u64 progress = 0;

	for (u64 bits = slot->sema_bits; bits; bits &= bits - 1)
	{
		const auto sema = &slot->sema_data[std::countr_zero(bits)];

		if (alert_sema(sema, data, progress))
		{
			s_tls_notify_cb(data, ++progress);
			break;
		}
	}

	s_tls_notify_cb(data, -1);
}

void atomic_storage_futex::notify_all(const void* data)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return;
	}

	u64 progress = 0;

#if defined(_WIN32) && !defined(USE_FUTEX)
	if (!NtAlertThreadByThreadId)
	{
		// Make a copy to filter out waiters that fail some checks
		u64 copy = slot->sema_bits.load();

		// Used for making non-blocking syscall
		static LARGE_INTEGER instant{};

		for (u64 bits = copy; bits; bits &= bits - 1)
		{
			const u32 id = std::countr_zero(bits);

			const auto sema = &slot->sema_data[id];

			if (sema->load() == 1 && sema->compare_and_swap_test(1, 2))
			{
				// Waiters locked for notification
				if (bits == copy)
				{
					// Notify imminent notification
					s_tls_notify_cb(data, 0);
				}

				continue;
			}

			// Remove the bit from next stage
			copy &= ~(1ull << id);
		}

		// If only one waiter exists, there is no point in trying to optimize
		if (copy & (copy - 1))
		{
			for (u64 bits = copy; bits; bits &= bits - 1)
			{
				const u32 id = std::countr_zero(bits);

				const auto sema = &slot->sema_data[id];

				if (NtReleaseKeyedEvent(nullptr, sema, 1, &instant))
				{
					// Failed to notify immediately
					continue;
				}

				s_tls_notify_cb(data, ++progress);

				// Remove the bit from next stage
				copy &= ~(1ull << id);
			}
		}

		// Proceed with remaining bits using "normal" blocking waiting
		for (u64 bits = copy; bits; bits &= bits - 1)
		{
			NtReleaseKeyedEvent(nullptr, &slot->sema_data[std::countr_zero(bits)], 1, nullptr);
			s_tls_notify_cb(data, ++progress);
		}

		s_tls_notify_cb(data, -1);
		return;
	}
#endif

	for (u64 bits = slot->sema_bits.load(); bits; bits &= bits - 1)
	{
		const auto sema = &slot->sema_data[std::countr_zero(bits)];

		if (alert_sema(sema, data, progress))
		{
			s_tls_notify_cb(data, ++progress);
			continue;
		}
	}

	s_tls_notify_cb(data, -1);
}
