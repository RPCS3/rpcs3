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

namespace
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
	};

#ifndef USE_STD
	static_assert(sizeof(cond_handle) == 64);
#endif
}

// Max allowed thread number is chosen to fit in 15 bits
static std::aligned_storage_t<sizeof(cond_handle), alignof(cond_handle)> s_cond_list[INT16_MAX]{};

// Used to allow concurrent notifying
static atomic_t<u16> s_cond_refs[INT16_MAX]{};

// Allocation bits
static atomic_t<u64, 64> s_cond_bits[::align<u32>(INT16_MAX, 64) / 64]{};

// Allocation semaphore
static atomic_t<u32, 64> s_cond_sema{0};

static u32 cond_alloc()
{
	// Determine whether there is a free slot or not
	if (!s_cond_sema.try_inc(INT16_MAX + 1))
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

		if (ok)
		{
			// Find lowest clear bit
			const u32 id = group * 64 + std::countr_one(bits);

			// Construct inplace before it can be used
			new (s_cond_list + id) cond_handle();

			return id + 1;
		}
	}

	// Unreachable
	std::abort();
	return 0;
}

static cond_handle* cond_get(u32 cond_id)
{
	if (cond_id - 1 < u32{INT16_MAX}) [[likely]]
	{
		return std::launder(reinterpret_cast<cond_handle*>(s_cond_list + (cond_id - 1)));
	}

	return nullptr;
}

static void cond_free(u32 cond_id)
{
	if (cond_id - 1 >= u32{INT16_MAX})
	{
		fprintf(stderr, "cond_free(): bad id %u" HERE "\n", cond_id);
		std::abort();
	}

	// Call the destructor
	cond_get(cond_id)->~cond_handle();

	// Remove the allocation bit
	s_cond_bits[(cond_id - 1) / 64] &= ~(1ull << ((cond_id - 1) % 64));

	// Release the semaphore
	s_cond_sema--;
}

namespace
{
#define MAX_THREADS (56)

	struct alignas(128) sync_var
	{
		constexpr sync_var() noexcept = default;

		// Reference counter, owning pointer, collision bit and optionally selected slot
		atomic_t<u64> addr_ref{};

		// Semaphores (allocated in reverse order), 0 means empty, 0x8000 is notifier lock
		atomic_t<u16> sema_data[MAX_THREADS]{};

		// Allocated semaphore bits (to make total size 128)
		atomic_t<u64> sema_bits{};

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
				return &sema_data[std::countr_one(bits)];
			}

			// TODO: support extension if reached
			fmt::raw_error("Thread limit " STRINGIZE(MAX_THREADS) " for a single address reached in atomic wait.");
			return nullptr;
		}

		void sema_free(atomic_t<u16>* sema)
		{
			if (sema < sema_data || sema >= std::end(sema_data))
			{
				fprintf(stderr, "sema_free(): bad sema ptr %p" HERE "\n", sema);
				std::abort();
			}

			const u32 cond_id = sema->fetch_and(0x8000);

			if (!cond_id || cond_id >> 15)
			{
				// Delegated cleanup
				return;
			}

			// Free
			cond_free(cond_id);

			// Clear sema bit
			sema_bits &= ~(1ull << (sema - sema_data));
		}
	};

	static_assert(sizeof(sync_var) == 128);

#undef MAX_THREADS
}

// Main hashtable for atomic wait.
alignas(128) static sync_var s_hashtable[s_hashtable_size]{};

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
#define MAX_SLOTS (4096)

// Array of slot branch objects
alignas(128) static slot_info s_slot_list[MAX_SLOTS]{};

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

	// Reset semaphore
	s_slot_sema--;
}

static void slot_free(std::uintptr_t iptr, sync_var* loc, u64 lv = 0)
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
atomic_storage_futex::wait(const void* data, u32 size, __m128i old_value, u64 timeout, __m128i mask)
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

	const u32 cond_id = cond_alloc();

	if (cond_id == 0)
	{
		fmt::raw_error("Thread limit " STRINGIZE(INT16_MAX) " reached in atomic wait.");
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
			if (NtWaitForKeyedEvent(nullptr, sema, false, timeout + 1 ? &qw : nullptr) == NTSTATUS_SUCCESS)
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

		if (!NtWaitForKeyedEvent(nullptr, sema, false, &instant))
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
	auto [cond_id, ok] = sema->fetch_op([](u16& id)
	{
		// Check if not zero and not locked
		if (!id || id & 0x8000)
		{
			return false;
		}

		// Set notify lock
		id |= 0x8000;
		return true;
	});

	if (!ok) [[unlikely]]
	{
		return false;
	}

	const auto cond = cond_get(cond_id);

	ok = false;

	if (cond && cond->sync && (!size ? (!info || cond->tid == info) : cond->ptr == data && cmp_mask(size, mask, new_value, cond->size, cond->mask, cond->oldv)))
	{
		if ((!size && cond->forced_wakeup()) || (size && cond->sync.load() == 1 && cond->sync.compare_and_swap_test(1, 2)))
		{
			ok = true;

#ifdef USE_FUTEX
			// Use "wake all" arg for robustness, only 1 thread is expected
			futex(&cond->sync, FUTEX_WAKE_PRIVATE, 0x7fff'ffff);
#elif defined(USE_STD)
			// Not super efficient: locking is required to avoid lost notifications
			cond->mtx.lock();
			cond->mtx.unlock();
			cond->cond.notify_all();
#elif defined(_WIN32)
			if (NtWaitForAlertByThreadId)
			{
				// Sets some sticky alert bit, at least I believe so
				NtAlertThreadByThreadId(cond->tid);
			}
			else
			{
				// Can wait in rare cases, which is its annoying weakness
				NtReleaseKeyedEvent(nullptr, sema, 1, nullptr);
			}
#endif
		}
	}

	// Remove lock, check if cond_id is already removed (leaving only 0x8000)
	if (sema->fetch_and(0x7fff) == 0x8000)
	{
		cond_free(cond_id);

		// Cleanup, a little hacky obtainment of the host variable
		const auto slot = std::launder(reinterpret_cast<sync_var*>(reinterpret_cast<u64>(sema) & -128));

		// Remove slot bit
		slot->sema_bits &= ~(1ull << (sema - slot->sema_data));
	}

	return ok;
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

bool atomic_storage_futex::raw_notify(const void* data, u64 thread_id)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return false;
	}

	s_tls_notify_cb(data, 0);

	u64 progress = 0;

	for (u64 bits = slot->sema_bits.load(); bits; bits &= bits - 1)
	{
		const auto sema = &slot->sema_data[std::countr_zero(bits)];

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
atomic_storage_futex::notify_one(const void* data, u32 size, __m128i mask, __m128i new_value)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return;
	}

	s_tls_notify_cb(data, 0);

	u64 progress = 0;

	for (u64 bits = slot->sema_bits; bits; bits &= bits - 1)
	{
		const auto sema = &slot->sema_data[std::countr_zero(bits)];

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
atomic_storage_futex::notify_all(const void* data, u32 size, __m128i mask, __m128i new_value)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return;
	}

	s_tls_notify_cb(data, 0);

	u64 progress = 0;

#if defined(_WIN32) && !defined(USE_FUTEX)
	// Special path for Windows 7
	if (!NtAlertThreadByThreadId)
#elif defined(USE_STD)
	{
		// Make a copy to filter out waiters that fail some checks
		u64 copy = slot->sema_bits.load();
		u64 lock = 0;
		u32 lock_ids[64]{};

		for (u64 bits = copy; bits; bits &= bits - 1)
		{
			const u32 id = std::countr_zero(bits);

			const auto sema = &slot->sema_data[id];

			auto [cond_id, ok] = sema->fetch_op([](u16& id)
			{
				if (!id || id & 0x8000)
				{
					return false;
				}

				id |= 0x8000;
				return true;
			});

			if (ok)
			{
				// Add lock bit for cleanup
				lock |= 1ull << id;
				lock_ids[id] = cond_id;

				const auto cond = cond_get(cond_id);

				if (cond && cond->sync && cond->ptr == data && cmp_mask(size, mask, new_value, cond->size, cond->mask, cond->oldv))
				{
					if (cond->sync.load() == 1 && cond->sync.compare_and_swap_test(1, 2))
					{
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

				const auto sema = &slot->sema_data[id];

#ifdef USE_STD
				const auto cond = cond_get(lock_ids[id]);

				// Optimistic non-blocking path
				if (cond->mtx.try_lock())
				{
					cond->mtx.unlock();
					cond->cond.notify_all();
				}
				else
				{
					continue;
				}
#elif defined(_WIN32)
				if (NtReleaseKeyedEvent(nullptr, sema, 1, &instant) != NTSTATUS_SUCCESS)
				{
					// Failed to notify immediately
					continue;
				}
#endif

				s_tls_notify_cb(data, ++progress);

				// Remove the bit from next stage
				copy &= ~(1ull << id);
			}
		}

		// Proceed with remaining bits using "normal" blocking waiting
		for (u64 bits = copy; bits; bits &= bits - 1)
		{
#ifdef USE_STD
			const auto cond = cond_get(lock_ids[std::countr_zero(bits)]);
			cond->mtx.lock();
			cond->mtx.unlock();
			cond->cond.notify_all();
#elif defined(_WIN32)
			NtReleaseKeyedEvent(nullptr, sema, 1, nullptr);
#endif
			s_tls_notify_cb(data, ++progress);
		}

		// Cleanup locked notifiers
		for (u64 bits = lock; bits; bits &= bits - 1)
		{
			const u32 id = std::countr_zero(bits);

			const auto sema = &slot->sema_data[id];

			if (sema->fetch_and(0x7fff) == 0x8000)
			{
				cond_free(lock_ids[id]);

				slot->sema_bits &= ~(1ull << id);
			}
		}

		s_tls_notify_cb(data, -1);
		return;
	}
#endif

	for (u64 bits = slot->sema_bits.load(); bits; bits &= bits - 1)
	{
		const auto sema = &slot->sema_data[std::countr_zero(bits)];

		if (alert_sema(sema, data, progress, size, mask, new_value))
		{
			s_tls_notify_cb(data, ++progress);
			continue;
		}
	}

	s_tls_notify_cb(data, -1);
}
