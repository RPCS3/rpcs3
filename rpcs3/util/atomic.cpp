#include "atomic.hpp"

#ifdef __linux__
#define USE_FUTEX
#endif

#include "Utilities/sync.h"
#include "Utilities/asm.h"

#ifdef USE_POSIX
#include <semaphore.h>
#endif

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

namespace
{
	struct sync_var
	{
		constexpr sync_var() noexcept = default;

		// Reference counter, owning pointer, collision bit and optionally selected slot
		atomic_t<u64> addr_ref{};

		// Counter for waiting threads for the semaphore and allocated semaphore id
		atomic_t<u64> sema_var{};
	};
}

// Main hashtable for atomic wait.
static sync_var s_hashtable[s_hashtable_size]{};

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
static slot_info s_slot_list[s_slot_gcount * 64]{};

// Allocation bits
static atomic_t<u64> s_slot_bits[s_slot_gcount]{};

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
		const u32 group = (i + start) % s_slot_gcount;

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
			return group * 64 + utils::cnttz64(~bits, false);
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
	const u64 eq_bits = utils::cntlz64((((iptr ^ value) & (s_pointer_mask >> lv)) | ~s_pointer_mask) << 16, true);

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
		const u64 eq_bits = utils::cntlz64((((iptr ^ value) & (s_pointer_mask >> lv)) | ~s_pointer_mask) << 16, true);

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
		// Deallocate slot on last waiter
		slot_free(_old);
	}
}

// Number of search groups (defines max semaphore count as gcount * 64)
static constexpr u32 s_sema_gcount = 128;

static constexpr u64 s_sema_mask = (s_sema_gcount * 64 - 1);

#ifdef USE_POSIX
using sema_handle = sem_t;
#elif defined(USE_FUTEX)
namespace
{
	struct alignas(64) sema_handle
	{
		atomic_t<u32> sema;
	};
}
#elif defined(_WIN32)
using sema_handle = std::uint16_t;
#else
namespace
{
	struct dumb_sema
	{
		u64 count = 0;
		std::mutex mutex;
		std::condition_variable cond;
	};
}

using sema_handle = std::unique_ptr<dumb_sema>;
#endif

// Array of native semaphores
static sema_handle s_sema_list[64 * s_sema_gcount]{};

// Array of associated reference counters
static atomic_t<u64> s_sema_refs[64 * s_sema_gcount]{};

// Allocation bits (reserve first bit)
static atomic_t<u64> s_sema_bits[s_sema_gcount]{1};

static u32 sema_alloc()
{
	// Diversify search start points to reduce contention and increase immediate success chance
#ifdef _WIN32
	const u32 start = GetCurrentProcessorNumber();
#elif __linux__
	const u32 start = sched_getcpu();
#else
	const u32 start = __rdtsc();
#endif

	for (u32 i = 0; i < s_sema_gcount * 3; i++)
	{
		const u32 group = (i + start) % s_sema_gcount;

		const auto [bits, ok] = s_sema_bits[group].fetch_op([](u64& bits)
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
			const u32 id = group * 64 + static_cast<u32>(utils::cnttz64(~bits, false));

#ifdef USE_POSIX
			// Initialize semaphore (should be very fast)
			sem_init(&s_sema_list[id], 0, 0);
#elif defined(_WIN32) || defined(USE_FUTEX)
			// Do nothing
#else
			if (!s_sema_list[id])
			{
				s_sema_list[id] = std::make_unique<dumb_sema>();
			}
#endif

			// Initialize ref counter
			if (s_sema_refs[id]++)
			{
				std::abort();
			}

			return id;
		}
	}

	return 0;
}

static void sema_free(u32 id)
{
	if (id && id < 64 * s_sema_gcount)
	{
		// Dereference first
		if (--s_sema_refs[id])
		{
			return;
		}

#ifdef USE_POSIX
		// Destroy semaphore (should be very fast)
		sem_destroy(&s_sema_list[id]);
#else
		// No action required
#endif

		// Reset allocation bit
		s_sema_bits[id / 64] &= ~(1ull << (id % 64));
	}
}

static bool sema_get(u32 id)
{
	if (id && id < 64 * s_sema_gcount)
	{
		// Increment only if the semaphore is allocated
		if (s_sema_refs[id].fetch_op([](u64& refs)
		{
			if (refs)
			{
				// Increase reference from non-zero value
				refs++;
			}
		}))
		{
			return true;
		}
	}

	return false;
}

static inline bool ptr_cmp(const void* data, std::size_t size, u64 old_value, u64 mask)
{
	switch (size)
	{
	case 1: return (reinterpret_cast<const atomic_t<u8>*>(data)->load() & mask) == (old_value & mask);
	case 2: return (reinterpret_cast<const atomic_t<u16>*>(data)->load() & mask) == (old_value & mask);
	case 4: return (reinterpret_cast<const atomic_t<u32>*>(data)->load() & mask) == (old_value & mask);
	case 8: return (reinterpret_cast<const atomic_t<u64>*>(data)->load() & mask) == (old_value & mask);
	}

	return false;
}

// Callback for wait() function, returns false if wait should return
static thread_local bool(*s_tls_wait_cb)(const void* data) = [](const void*)
{
	return true;
};

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
			return;
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
		const u64 eq_bits = utils::cntlz64((((iptr ^ ok) & (s_pointer_mask >> lv)) | ~s_pointer_mask) << 16, true);

		// Collision; need to go deeper
		ptr = s_slot_list[(ok & s_slot_mask) / one_v<s_slot_mask>].branch + eq_bits;

		lv = eq_bits + 1;
	}

	// Now try to reference a semaphore (allocate it if needed)
	u32 sema_id = static_cast<u32>(slot->sema_var & s_sema_mask);

	for (u32 loop_count = 0; loop_count < 7; loop_count++)
	{
		// Try to allocate a semaphore
		if (!sema_id)
		{
			const u32 sema = sema_alloc();

			if (!sema)
			{
				break;
			}

			sema_id = slot->sema_var.atomic_op([&](u64& value) -> u32
			{
				if (value & s_sema_mask)
				{
					return static_cast<u32>(value & s_sema_mask);
				}

				// Insert allocated semaphore
				value += s_sema_mask + 1;
				value |= sema;
				return 0;
			});

			if (sema_id)
			{
				// Drop unnecessary allocation
				sema_free(sema);
			}
			else
			{
				sema_id = sema;
				break;
			}
		}

		if (!sema_get(sema_id))
		{
			sema_id = 0;
			continue;
		}

		// Try to increment sig (check semaphore validity)
		const auto [_old, _new] = slot->sema_var.fetch_op([&](u64& value) -> u64
		{
			if ((value & ~s_sema_mask) == ~s_sema_mask)
			{
				// Signal overflow
				return 0;
			}

			if ((value & s_sema_mask) != sema_id)
			{
				return 0;
			}

			value += s_sema_mask + 1;
			return value;
		});

		if (!_new)
		{
			sema_free(sema_id);

			if ((_old & ~s_sema_mask) == ~s_sema_mask)
			{
				// Break on signal overflow
				sema_id = -1;
				break;
			}

			sema_id = _new & s_sema_mask;
			continue;
		}

		break;
	}

	bool fallback = false;

	if (sema_id && ptr_cmp(data, size, old_value, mask) && s_tls_wait_cb(data))
	{
#ifdef USE_FUTEX
		struct timespec ts;
		ts.tv_sec  = timeout / 1'000'000'000;
		ts.tv_nsec = timeout % 1'000'000'000;

		if (s_sema_list[sema_id].sema.try_dec(0))
		{
			fallback = true;
		}
		else
		{
			futex(&s_sema_list[sema_id].sema, FUTEX_WAIT_PRIVATE, 0, timeout + 1 ? &ts : nullptr);

			if (s_sema_list[sema_id].sema.try_dec(0))
			{
				fallback = true;
			}
		}
#elif defined(_WIN32) && !defined(USE_POSIX)
		LARGE_INTEGER qw;
		qw.QuadPart = -static_cast<s64>(timeout / 100);

		if (timeout % 100)
		{
			// Round up to closest 100ns unit
			qw.QuadPart -= 1;
		}

		if (!NtWaitForKeyedEvent(nullptr, &s_sema_list[sema_id], false, timeout + 1 ? &qw : nullptr))
		{
			fallback = true;
		}
#elif defined(USE_POSIX)
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec  += timeout / 1'000'000'000;
		ts.tv_nsec += timeout % 1'000'000'000;
		ts.tv_sec  += ts.tv_nsec / 1'000'000'000;
		ts.tv_nsec %= 1'000'000'000;

		// It's pretty unreliable because it uses absolute time, which may jump backwards. Sigh.
		if (timeout + 1)
		{
			if (sem_timedwait(&s_sema_list[sema_id], &ts) == 0)
			{
				fallback = true;
			}
		}
		else
		{
			if (sem_wait(&s_sema_list[sema_id]) == 0)
			{
				fallback = true;
			}
		}
#else
		dumb_sema& sema = *s_sema_list[sema_id];

		std::unique_lock lock(sema.mutex);

		if (timeout + 1)
		{
			sema.cond.wait_for(lock, std::chrono::nanoseconds(timeout), [&]
			{
				return sema.count > 0;
			});
		}
		else
		{
			sema.cond.wait(lock, [&]
			{
				return sema.count > 0;
			});
		}

		if (sema.count > 0)
		{
			sema.count--;
			fallback = true;
		}
#endif
	}

	if (!sema_id)
	{
		fallback = true;
	}

	while (true)
	{
		// Try to decrement
		const auto [prev, ok] = slot->sema_var.fetch_op([&](u64& value)
		{
			if (value)
			{
				// If timeout
				if (!fallback)
				{
					if ((value & ~s_sema_mask) == 0 || (value & s_sema_mask) != sema_id)
					{
						// Give up if signaled or semaphore has already changed
						return false;
					}

					value -= s_sema_mask + 1;

					if ((value & ~s_sema_mask) == 0)
					{
						// Remove allocated sema on last waiter
						value = 0;
					}
				}

				return true;
			}

			return false;
		});

		if (ok || fallback)
		{
			break;
		}

#ifdef USE_FUTEX
		if (s_sema_list[sema_id].sema.try_dec(0))
		{
			fallback = true;
		}
#elif defined(_WIN32) && !defined(USE_POSIX)
		static LARGE_INTEGER instant{};

		if (!NtWaitForKeyedEvent(nullptr, &s_sema_list[sema_id], false, &instant))
		{
			fallback = true;
		}
#elif defined(USE_POSIX)
		if (sem_trywait(&s_sema_list[sema_id]) == 0)
		{
			fallback = true;
		}
#else
		dumb_sema& sema = *s_sema_list[sema_id];

		std::unique_lock lock(sema.mutex);

		if (sema.count > 0)
		{
			sema.count--;
			fallback = true;
		}
#endif
	}

	if (sema_id)
	{
		sema_free(sema_id);
	}

	slot_free(iptr, &s_hashtable[iptr % s_hashtable_size]);

	s_tls_wait_cb(nullptr);
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

	const u64 value = slot->sema_var;

	if ((value & ~s_sema_mask) == 0 || !(value & s_sema_mask))
	{
		return;
	}

	const u32 sema_id = static_cast<u32>(value & s_sema_mask);

	if (!sema_get(sema_id))
	{
		return;
	}

	const auto [_, ok] = slot->sema_var.fetch_op([&](u64& value)
	{
		if ((value & ~s_sema_mask) == 0 || (value & s_sema_mask) != sema_id)
		{
			return false;
		}

		value -= s_sema_mask + 1;

		// Reset allocated semaphore on last waiter
		if ((value & ~s_sema_mask) == 0)
		{
			value = 0;
		}

		return true;
	});

	if (ok)
	{
#ifdef USE_POSIX
		sem_post(&s_sema_list[sema_id]);
#elif defined(USE_FUTEX)
		s_sema_list[sema_id].sema++;
		futex(&s_sema_list[sema_id].sema, FUTEX_WAKE_PRIVATE, 1);
#elif defined(_WIN32)
		NtReleaseKeyedEvent(nullptr, &s_sema_list[sema_id], 1, nullptr);
#else
		dumb_sema& sema = *s_sema_list[sema_id];

		sema.mutex.lock();
		sema.count += 1;
		sema.mutex.unlock();
		sema.cond.notify_one();
#endif
	}

	sema_free(sema_id);
}

void atomic_storage_futex::notify_all(const void* data)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	const auto slot = slot_get(iptr, &s_hashtable[(iptr) % s_hashtable_size]);

	if (!slot)
	{
		return;
	}

	const u64 value = slot->sema_var;

	if ((value & ~s_sema_mask) == 0 || !(value & s_sema_mask))
	{
		return;
	}

	const u32 sema_id = static_cast<u32>(value & s_sema_mask);

	if (!sema_get(sema_id))
	{
		return;
	}

	const auto [_, count] = slot->sema_var.fetch_op([&](u64& value) -> u32
	{
		if ((value & ~s_sema_mask) == 0 || (value & s_sema_mask) != sema_id)
		{
			return 0;
		}

		return (std::exchange(value, 0) & ~s_sema_mask) / (s_sema_mask + 1);
	});

#ifdef USE_POSIX
	for (u32 i = 0; i < count; i++)
	{
		sem_post(&s_sema_list[sema_id]);
	}
#elif defined(USE_FUTEX)
	s_sema_list[sema_id].sema += count;
	futex(&s_sema_list[sema_id].sema, FUTEX_WAKE_PRIVATE, 0x7fff'ffff);
#elif defined(_WIN32)
	for (u32 i = 0; i < count; i++)
	{
		NtReleaseKeyedEvent(nullptr, &s_sema_list[sema_id], count, nullptr);
	}
#else
	if (count)
	{
		dumb_sema& sema = *s_sema_list[sema_id];

		sema.mutex.lock();
		sema.count += count;
		sema.mutex.unlock();
		sema.cond.notify_all();
	}
#endif

	sema_free(sema_id);
}
