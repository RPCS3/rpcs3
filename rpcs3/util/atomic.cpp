#include "atomic.hpp"

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
template <bool CheckCb = true>
static NEVER_INLINE bool
#ifdef _WIN32
__vectorcall
#endif
ptr_cmp(const void* data, u32 size, __m128i old128, __m128i mask128)
{
	if constexpr (CheckCb)
	{
		if (!s_tls_wait_cb(data))
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
		const auto v0 = std::bit_cast<__m128i>(atomic_storage<u128>::load(*reinterpret_cast<const u128*>(data)));
		const auto v1 = _mm_xor_si128(v0, old128);
		const auto v2 = _mm_and_si128(v1, mask128);
		const auto v3 = _mm_packs_epi16(v2, v2);

		if (_mm_cvtsi128_si64(v3) == 0)
		{
			return true;
		}

		break;
	}
	default:
	{
		fprintf(stderr, "ptr_cmp(): bad size (size=%u)" HERE "\n", size);
		std::abort();
	}
	}

	return false;
}

// Returns true if mask overlaps, or the argument is invalid
static bool
#ifdef _WIN32
__vectorcall
#endif
cmp_mask(u32 size1, __m128i mask1, __m128i val1, u32 size2, __m128i mask2, __m128i val2)
{
	// In force wake up, one of the size arguments is zero
	const u32 size = std::min(size1, size2);

	if (!size) [[unlikely]]
	{
		return true;
	}

	// Compare only masks, new value is not available in this mode
	if ((size1 | size2) == umax)
	{
		// Simple mask overlap
		const auto v0 = _mm_and_si128(mask1, mask2);
		const auto v1 = _mm_packs_epi16(v0, v0);
		return _mm_cvtsi128_si64(v1) != 0;
	}

	// Generate masked value inequality bits
	const auto v0 = _mm_and_si128(_mm_and_si128(mask1, mask2), _mm_xor_si128(val1, val2));

	if (size <= 8)
	{
		// Generate sized mask
		const u64 mask = UINT64_MAX >> ((64 - size * 8) & 63);

		if (!(_mm_cvtsi128_si64(v0) & mask))
		{
			return false;
		}
	}
	else if (size == 16)
	{
		if (!_mm_cvtsi128_si64(_mm_packs_epi16(v0, v0)))
		{
			return false;
		}
	}
	else
	{
		fprintf(stderr, "cmp_mask(): bad size (size1=%u, size2=%u)" HERE "\n", size1, size2);
		std::abort();
	}

	return true;
}

namespace atomic_wait
{
	// Essentially a fat semaphore
	struct alignas(64) cond_handle
	{
#ifdef _WIN32
		u64 tid = GetCurrentThreadId();
#else
		u64 tid = reinterpret_cast<u64>(pthread_self());
#endif
		atomic_t<u32> sync{};
		u32 size{};
		u64 tsc0{};
		const void* ptr{};
		__m128i mask{};
		__m128i oldv{};

#ifdef USE_STD
		// Standard CV/mutex pair (often contains pthread_cond_t/pthread_mutex_t)
		std::condition_variable cond;
		std::mutex mtx;
#endif

		bool forced_wakeup()
		{
			const auto [_old, ok] = sync.fetch_op([](u32& val)
			{
				if (val == 1 || val == 2)
				{
					val = 3;
					return true;
				}

				return false;
			});

			// Prevent collision between normal wake-up and forced one
			return ok && _old == 1;
		}

		void alert_native()
		{
#ifdef USE_FUTEX
			// Use "wake all" arg for robustness, only 1 thread is expected
			futex(&sync, FUTEX_WAKE_PRIVATE, 0x7fff'ffff);
#elif defined(USE_STD)
			// Not super efficient: locking is required to avoid lost notifications
			mtx.lock();
			mtx.unlock();
			cond.notify_all();
#elif defined(_WIN32)
			if (NtWaitForAlertByThreadId)
			{
				// Sets some sticky alert bit, at least I believe so
				NtAlertThreadByThreadId(tid);
			}
			else
			{
				// Can wait in rare cases, which is its annoying weakness
				NtReleaseKeyedEvent(nullptr, &sync, 1, nullptr);
			}
#endif
		}

		bool try_alert_native()
		{
#if defined(USE_FUTEX)
			return false;
#elif defined(USE_STD)
			// Optimistic non-blocking path
			if (mtx.try_lock())
			{
				mtx.unlock();
				cond.notify_all();
				return true;
			}

			return false;
#elif defined(_WIN32)
			if (NtAlertThreadByThreadId)
			{
				// Don't notify prematurely with this API
				return false;
			}

			static LARGE_INTEGER instant{};

			if (NtReleaseKeyedEvent(nullptr, &sync, 1, &instant) != NTSTATUS_SUCCESS)
			{
				// Failed to notify immediately
				return false;
			}

			return true;
#endif
		}
	};

#ifndef USE_STD
	static_assert(sizeof(cond_handle) == 64);
#endif
}

// Max allowed thread number is chosen to fit in 16 bits
static std::aligned_storage_t<sizeof(atomic_wait::cond_handle), alignof(atomic_wait::cond_handle)> s_cond_list[UINT16_MAX + 1]{};

// Used to allow concurrent notifying
static atomic_t<u32> s_cond_refs[UINT16_MAX + 1]{};

// Allocation bits
static atomic_t<u64, 64> s_cond_bits[(UINT16_MAX + 1) / 64]{};

// Allocation semaphore
static atomic_t<u32, 64> s_cond_sema{0};

static u32 cond_alloc()
{
	// Determine whether there is a free slot or not
	if (!s_cond_sema.try_inc(UINT16_MAX + 1))
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

	for (u32 i = start;; i++)
	{
		const u32 group = i % ::size32(s_cond_bits);

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

		if (ok) [[likely]]
		{
			// Find lowest clear bit
			const u32 id = group * 64 + std::countr_one(bits);

			if (id == 0) [[unlikely]]
			{
				// Special case, set bit and continue
				continue;
			}

			// Construct inplace before it can be used
			new (s_cond_list + id) atomic_wait::cond_handle();

			// Add first reference
			verify(HERE), !s_cond_refs[id]++;

			return id;
		}
	}

	// Unreachable
	std::abort();
	return 0;
}

static atomic_wait::cond_handle* cond_get(u32 cond_id)
{
	if (cond_id - 1 < u32{UINT16_MAX}) [[likely]]
	{
		return std::launder(reinterpret_cast<atomic_wait::cond_handle*>(s_cond_list + cond_id));
	}

	return nullptr;
}

static void cond_free(u32 cond_id)
{
	if (cond_id - 1 >= u32{UINT16_MAX})
	{
		fprintf(stderr, "cond_free(): bad id %u" HERE "\n", cond_id);
		std::abort();
	}

	// Dereference, destroy on last ref
	if (--s_cond_refs[cond_id])
	{
		return;
	}

	// Call the destructor
	cond_get(cond_id)->~cond_handle();

	// Remove the allocation bit
	s_cond_bits[cond_id / 64] &= ~(1ull << (cond_id % 64));

	// Release the semaphore
	s_cond_sema--;
}

static u32 cond_lock(atomic_t<u16>* sema)
{
	while (const u32 cond_id = sema->load())
	{
		const auto [old, ok] = s_cond_refs[cond_id].fetch_op([](u32& ref)
		{
			if (!ref || ref == UINT32_MAX)
			{
				// Don't reference already deallocated semaphore
				return false;
			}

			ref++;
			return true;
		});

		if (ok)
		{
			return cond_id;
		}

		if (old == UINT32_MAX)
		{
			fmt::raw_error("Thread limit " STRINGIZE(UINT32_MAX) " for a single address reached in atomic notifier.");
		}

		if (sema->load() != cond_id)
		{
			// Try again if it changed
			continue;
		}
		else
		{
			break;
		}
	}

	return 0;
}

namespace atomic_wait
{
#define MAX_THREADS (56)

	struct alignas(128) sync_var
	{
		constexpr sync_var() noexcept = default;

		// Reference counter, owning pointer, collision bit and optionally selected slot
		atomic_t<u64> addr_ref{};

	private:
		// Semaphores (allocated in reverse order), empty are zeros
		atomic_t<u16> sema_data[MAX_THREADS]{};

		// Allocated semaphore bits (to make total size 128)
		atomic_t<u64> sema_bits{};

	public:
		atomic_t<u16>* sema_alloc()
		{
			const auto [bits, ok] = sema_bits.fetch_op([](u64& bits)
			{
				if (bits + 1 < (1ull << MAX_THREADS))
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
				return get_sema(std::countr_one(bits));
			}

			// TODO: support extension if reached
			fmt::raw_error("Thread limit " STRINGIZE(MAX_THREADS) " for a single address reached in atomic wait.");
			return nullptr;
		}

		atomic_t<u16>* get_sema(u32 id)
		{
			verify(HERE), id < MAX_THREADS;

			return &sema_data[(MAX_THREADS - 1) - id];
		}

		u64 get_sema_bits() const
		{
			return sema_bits & ((1ull << MAX_THREADS) - 1);
		}

		void reset_sema_bit(atomic_t<u16>* sema)
		{
			verify(HERE), sema >= sema_data && sema < std::end(sema_data);

			sema_bits &= ~(1ull << ((MAX_THREADS - 1) - (sema - sema_data)));
		}

		void sema_free(atomic_t<u16>* sema)
		{
			if (sema < sema_data || sema >= std::end(sema_data))
			{
				fprintf(stderr, "sema_free(): bad sema ptr %p" HERE "\n", sema);
				std::abort();
			}

			// Try to deallocate semaphore (may be delegated to a notifier)
			cond_free(sema->exchange(0));

			// Clear sema bit
			reset_sema_bit(sema);
		}
	};

	static_assert(sizeof(sync_var) == 128);

#undef MAX_THREADS
}

// Main hashtable for atomic wait.
alignas(128) static atomic_wait::sync_var s_hashtable[s_hashtable_size]{};

namespace atomic_wait
{
	struct slot_info
	{
		constexpr slot_info() noexcept = default;

		// Branch extension
		atomic_wait::sync_var branch[48 - s_hashtable_power]{};
	};
}

// Number of search groups (defines max slot branch count as gcount * 64)
#define MAX_SLOTS (4096)

// Array of slot branch objects
alignas(128) static atomic_wait::slot_info s_slot_list[MAX_SLOTS]{};

// Allocation bits
static atomic_t<u64, 64> s_slot_bits[MAX_SLOTS / 64]{};

// Allocation semaphore
static atomic_t<u32, 64> s_slot_sema{0};

static_assert(MAX_SLOTS % 64 == 0);

static u64 slot_alloc()
{
	// Determine whether there is a free slot or not
	if (!s_slot_sema.try_inc(MAX_SLOTS + 1))
	{
		fmt::raw_error("Hashtable extension slot limit " STRINGIZE(MAX_SLOTS) " reached in atomic wait.");
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

	for (u32 i = start;; i++)
	{
		const u32 group = i % ::size32(s_slot_bits);

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

	// Unreachable
	std::abort();
	return 0;
}

#undef MAX_SLOTS

static atomic_wait::sync_var* slot_get(std::uintptr_t iptr, atomic_wait::sync_var* loc, u64 lv = 0)
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

	// Reset semaphore
	s_slot_sema--;
}

static void slot_free(std::uintptr_t iptr, atomic_wait::sync_var* loc, u64 lv = 0)
{
	const u64 value = loc->addr_ref.load();

	if ((value & s_pointer_mask) != (iptr & s_pointer_mask))
	{
		ASSERT(value & s_waiter_mask);
		ASSERT(value & s_collision_bit);

		// Get the number of leading equal bits to determine subslot
		const u64 eq_bits = std::countl_zero<u64>((((iptr ^ value) & (s_pointer_mask >> lv)) | ~s_pointer_mask) << 16);

		// Proceed recursively, to deallocate deepest branch first
		slot_free(iptr, s_slot_list[(value & s_slot_mask) / one_v<s_slot_mask>].branch + eq_bits, eq_bits + 1);
	}

	// Actual cleanup in reverse order
	auto [_old, ok] = loc->addr_ref.fetch_op([&](u64& value)
	{
		ASSERT(value & s_waiter_mask);
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
	});

	if (ok > 1 && _old & s_collision_bit)
	{
		// Deallocate slot on last waiter
		slot_free(_old);
	}
}

SAFE_BUFFERS void
#ifdef _WIN32
__vectorcall
#endif
atomic_wait_engine::wait(const void* data, u32 size, __m128i old_value, u64 timeout, __m128i mask)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	// Allocated slot index
	u64 slot_a = -1;

	// Found slot object
	atomic_wait::sync_var* slot = nullptr;

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

	for (atomic_wait::sync_var* ptr = &s_hashtable[iptr % s_hashtable_size];;)
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

	const u32 cond_id = cond_alloc();

	if (cond_id == 0)
	{
		fmt::raw_error("Thread limit " STRINGIZE(UINT16_MAX) " reached in atomic wait.");
	}

	auto sema = slot->sema_alloc();

	while (!sema)
	{
		if (timeout + 1 || ptr_cmp<false>(data, size, old_value, mask))
		{
			cond_free(cond_id);
			slot_free(iptr, &s_hashtable[iptr % s_hashtable_size]);
			return;
		}

		// TODO
		busy_wait(30000);
		sema = slot->sema_alloc();
	}

	// Save for notifiers
	const auto cond = cond_get(cond_id);

	// Store some info for notifiers (some may be unused)
	cond->size = size;
	cond->mask = mask;
	cond->oldv = old_value;
	cond->ptr  = data;
	cond->tsc0 = __rdtsc();

	cond->sync = 1;
	sema->store(static_cast<u16>(cond_id));

#ifdef USE_STD
	// Lock mutex
	std::unique_lock lock(cond->mtx);
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

		if (cond->sync.load() > 1) [[unlikely]]
		{
			// Signaled prematurely
			if (cond->sync.load() == 3 || !cond->sync.compare_and_swap_test(2, 1))
			{
				break;
			}
		}
		else
		{
			futex(&cond->sync, FUTEX_WAIT_PRIVATE, 1, timeout + 1 ? &ts : nullptr);
		}
#elif defined(USE_STD)
		if (cond->sync.load() > 1) [[unlikely]]
		{
			if (cond->sync.load() == 3 || !cond->sync.compare_and_swap_test(2, 1))
			{
				break;
			}
		}

		if (timeout + 1)
		{
			cond->cond.wait_for(lock, std::chrono::nanoseconds(timeout));
		}
		else
		{
			cond->cond.wait(lock);
		}
#elif defined(_WIN32)
		LARGE_INTEGER qw;
		qw.QuadPart = -static_cast<s64>(timeout / 100);

		if (timeout % 100)
		{
			// Round up to closest 100ns unit
			qw.QuadPart -= 1;
		}

		if (fallback) [[unlikely]]
		{
			if (cond->sync.load() == 3 || !cond->sync.compare_and_swap_test(2, 1))
			{
				fallback = false;
				break;
			}

			fallback = false;
		}

		if (NtWaitForAlertByThreadId)
		{
			switch (DWORD status = NtWaitForAlertByThreadId(cond, timeout + 1 ? &qw : nullptr))
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
			if (NtWaitForKeyedEvent(nullptr, &cond->sync, false, timeout + 1 ? &qw : nullptr) == NTSTATUS_SUCCESS)
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

		if (cond->sync.compare_and_swap_test(1, 2))
		{
			// Succeeded in self-notifying
			break;
		}

		if (NtWaitForAlertByThreadId)
		{
			if (NtWaitForAlertByThreadId(cond, &instant) == NTSTATUS_ALERTED)
			{
				break;
			}

			continue;
		}

		if (!NtWaitForKeyedEvent(nullptr, &cond->sync, false, &instant))
		{
			// Succeeded in obtaining an event without waiting
			break;
		}

		continue;
#endif
	}

#ifdef USE_STD
	if (lock)
	{
		lock.unlock();
	}
#endif

	slot->sema_free(sema);

	slot_free(iptr, &s_hashtable[iptr % s_hashtable_size]);

	s_tls_wait_cb(nullptr);
}

// Platform specific wake-up function
static NEVER_INLINE bool
#ifdef _WIN32
__vectorcall
#endif
alert_sema(atomic_t<u16>* sema, const void* data, u64 info, u32 size, __m128i mask, __m128i new_value)
{
	const u32 cond_id = cond_lock(sema);

	if (!cond_id)
	{
		return false;
	}

	const auto cond = cond_get(cond_id);

	verify(HERE), cond;

	bool ok = false;

	if (cond->sync && (!size ? (!info || cond->tid == info) : cond->ptr == data && cmp_mask(size, mask, new_value, cond->size, cond->mask, cond->oldv)))
	{
		if ((!size && cond->forced_wakeup()) || (size && cond->sync.load() == 1 && cond->sync.compare_and_swap_test(1, 2)))
		{
			ok = true;
			cond->alert_native();
		}
	}

	// Remove lock, possibly deallocate cond
	cond_free(cond_id);
	return ok;
}

void atomic_wait_engine::set_wait_callback(bool(*cb)(const void* data))
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

void atomic_wait_engine::set_notify_callback(void(*cb)(const void*, u64))
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

bool atomic_wait_engine::raw_notify(const void* data, u64 thread_id)
{
	// Special operation mode. Note that this is not atomic.
	if (!data)
	{
		// Special path: search thread_id without pointer information
		for (u32 i = 1; i < UINT16_MAX; i++)
		{
			const auto [_, ok] = s_cond_refs[i].fetch_op([&](u32& ref)
			{
				if (!ref)
				{
					// Skip dead semaphores
					return false;
				}

				if (thread_id)
				{
					u64 tid = 0;
					std::memcpy(&tid, &cond_get(i)->tid, sizeof(tid));

					if (tid != thread_id)
					{
						// Check thread first without locking (memory may be uninitialized)
						return false;
					}
				}

				if (ref < UINT32_MAX)
				{
					// Need to busy loop otherwise (TODO)
					ref++;
				}

				return true;
			});

			if (ok) [[unlikely]]
			{
				const auto cond = cond_get(i);

				if (!thread_id || cond->tid == thread_id)
				{
					if (cond->forced_wakeup())
					{
						cond->alert_native();

						if (thread_id)
						{
							// Only if thread_id is speficied, stop only it and return true.
							cond_free(i);
							return true;
						}
					}
				}

				cond_free(i);
			}
		}

		return false;
	}

	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return false;
	}

	s_tls_notify_cb(data, 0);

	u64 progress = 0;

	for (u64 bits = slot->get_sema_bits(); bits; bits &= bits - 1)
	{
		const auto sema = slot->get_sema(std::countr_zero(bits));

		// Forced notification
		if (alert_sema(sema, data, thread_id, 0, _mm_setzero_si128(), _mm_setzero_si128()))
		{
			s_tls_notify_cb(data, ++progress);

			if (thread_id == 0)
			{
				// Works like notify_all in this case
				continue;
			}

			break;
		}
	}

	s_tls_notify_cb(data, -1);
	return progress != 0;
}

void
#ifdef _WIN32
__vectorcall
#endif
atomic_wait_engine::notify_one(const void* data, u32 size, __m128i mask, __m128i new_value)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return;
	}

	s_tls_notify_cb(data, 0);

	u64 progress = 0;

	for (u64 bits = slot->get_sema_bits(); bits; bits &= bits - 1)
	{
		const auto sema = slot->get_sema(std::countr_zero(bits));

		if (alert_sema(sema, data, progress, size, mask, new_value))
		{
			s_tls_notify_cb(data, ++progress);
			break;
		}
	}

	s_tls_notify_cb(data, -1);
}

SAFE_BUFFERS void
#ifdef _WIN32
__vectorcall
#endif
atomic_wait_engine::notify_all(const void* data, u32 size, __m128i mask, __m128i new_value)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return;
	}

	s_tls_notify_cb(data, 0);

	u64 progress = 0;
	{
		// Make a copy to filter out waiters that fail some checks
		u64 copy = slot->get_sema_bits();
		u64 lock = 0;
		u32 lock_ids[64]{};

		for (u64 bits = copy; bits; bits &= bits - 1)
		{
			const u32 id = std::countr_zero(bits);

			const auto sema = slot->get_sema(id);

			if (const u32 cond_id = cond_lock(sema))
			{
				// Add lock bit for cleanup
				lock |= 1ull << id;
				lock_ids[id] = cond_id;

				const auto cond = cond_get(cond_id);

				verify(HERE), cond;

				if (cond->sync && cond->ptr == data && cmp_mask(size, mask, new_value, cond->size, cond->mask, cond->oldv))
				{
					if (cond->sync.load() == 1 && cond->sync.compare_and_swap_test(1, 2))
					{
						// Ok.
						continue;
					}
				}
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

				if (cond_get(lock_ids[id])->try_alert_native())
				{
					s_tls_notify_cb(data, ++progress);

					// Remove the bit from next stage
					copy &= ~(1ull << id);
				}
			}
		}

		// Proceed with remaining bits using "normal" blocking waiting
		for (u64 bits = copy; bits; bits &= bits - 1)
		{
			cond_get(lock_ids[std::countr_zero(bits)])->alert_native();

			s_tls_notify_cb(data, ++progress);
		}

		// Cleanup locked notifiers
		for (u64 bits = lock; bits; bits &= bits - 1)
		{
			cond_free(lock_ids[std::countr_zero(bits)]);
		}

		s_tls_notify_cb(data, -1);
		return;
	}

	// Unused, let's keep for reference
	for (u64 bits = slot->get_sema_bits(); bits; bits &= bits - 1)
	{
		const auto sema = slot->get_sema(std::countr_zero(bits));

		if (alert_sema(sema, data, progress, size, mask, new_value))
		{
			s_tls_notify_cb(data, ++progress);
			continue;
		}
	}

	s_tls_notify_cb(data, -1);
}
