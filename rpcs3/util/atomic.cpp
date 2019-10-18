#include "atomic.hpp"

// USE_FUTEX takes precedence over USE_POSIX

#ifdef __linux__
#define USE_FUTEX
#define USE_POSIX
#endif

#include "Utilities/sync.h"
#include "Utilities/asm.h"

#ifdef USE_POSIX
#include <semaphore.h>
#endif

#include <map>
#include <mutex>
#include <condition_variable>
#include <iterator>
#include <memory>

// Total number of entries, should be a power of 2.
static constexpr std::uintptr_t s_hashtable_size = 1u << 22;

// TODO: it's probably better to implement more effective futex emulation for OSX/BSD here.
static atomic_t<u64> s_hashtable[s_hashtable_size]{};

// Pointer mask without bits used as hash, assuming signed 48-bit pointers
static constexpr u64 s_pointer_mask = 0xffff'ffff'ffff & ~((s_hashtable_size - 1) << 2);

// Max number of waiters is 32767
static constexpr u64 s_waiter_mask = 0x7fff'0000'0000'0000;

//
static constexpr u64 s_collision_bit = 0x8000'0000'0000'0000;

#ifdef USE_FUTEX
static constexpr u64 s_sema_mask = 0;
#else
// Number of search groups (defines max semaphore count as gcount * 64)
static constexpr u32 s_sema_gcount = 64;

// Bits encoding allocated semaphore index (zero = not allocated yet)
static constexpr u64 s_sema_mask = (64 * s_sema_gcount - 1) << 2;
#endif

// Implementation detail (remaining bits out of 32 available for futex)
static constexpr u64 s_signal_mask = 0xffffffff & ~(s_waiter_mask | s_pointer_mask | s_collision_bit | s_sema_mask);

// Callback for wait() function, returns false if wait should return
static thread_local bool(*s_tls_wait_cb)(const void* data) = [](const void*)
{
	return true;
};

#ifndef USE_FUTEX

#ifdef USE_POSIX
using sema_handle = sem_t;
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
			const u32 id = group * 64 + utils::cnttz64(~bits, false);

#ifdef USE_POSIX
			// Initialize semaphore (should be very fast)
			sem_init(&s_sema_list[id], 0, 0);
#elif defined(_WIN32)
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

#endif

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

// Fallback implementation
namespace
{
	struct waiter
	{
		std::condition_variable cond;
		void* const tls_ptr;

		explicit waiter(void* tls_ptr)
			: tls_ptr(tls_ptr)
		{
		}
	};

	struct waiter_map
	{
		std::mutex mutex;
		std::multimap<const void*, waiter> list;
	};

	// Thread's unique node to insert without allocation
	thread_local std::multimap<const void*, waiter>::node_type s_tls_waiter = []()
	{
		// Initialize node from a dummy container (there is no separate node constructor)
		std::multimap<const void*, waiter> dummy;
		return dummy.extract(dummy.emplace(nullptr, &s_tls_waiter));
	}();

	waiter_map& get_fallback_map(const void* ptr)
	{
		static waiter_map s_waiter_maps[4096];

		return s_waiter_maps[std::hash<const void*>()(ptr) % std::size(s_waiter_maps)];
	}

	void fallback_wait(const void* data, std::size_t size, u64 old_value, u64 timeout, u64 mask)
	{
		auto& wmap = get_fallback_map(data);

		if (!timeout)
		{
			return;
		}

		// Update node key
		s_tls_waiter.key() = data;

		if (std::unique_lock lock(wmap.mutex); ptr_cmp(data, size, old_value, mask) && s_tls_wait_cb(data))
		{
			// Add node to the waiter list
			const auto iter = wmap.list.insert(std::move(s_tls_waiter));

			// Wait until the node is returned to its TLS location
			if (timeout + 1)
			{
				if (!iter->second.cond.wait_for(lock, std::chrono::nanoseconds(timeout), [&]
				{
					return 1 && s_tls_waiter;
				}))
				{
					// Put it back
					s_tls_waiter = wmap.list.extract(iter);
				}

				return;
			}

			while (!s_tls_waiter)
			{
				iter->second.cond.wait(lock);
			}
		}
	}

	void fallback_notify(waiter_map& wmap, std::multimap<const void*, waiter>::iterator found)
	{
		// Return notified node to its TLS location
		const auto ptls = static_cast<std::multimap<const void*, waiter>::node_type*>(found->second.tls_ptr);
		*ptls = wmap.list.extract(found);
		ptls->mapped().cond.notify_one();
	}

	void fallback_notify_one(const void* data)
	{
		auto& wmap = get_fallback_map(data);

		std::lock_guard lock(wmap.mutex);

		if (auto found = wmap.list.find(data); found != wmap.list.end())
		{
			fallback_notify(wmap, found);
		}
	}

	void fallback_notify_all(const void* data)
	{
		auto& wmap = get_fallback_map(data);

		std::lock_guard lock(wmap.mutex);

		for (auto it = wmap.list.lower_bound(data); it != wmap.list.end() && it->first == data;)
		{
			fallback_notify(wmap, it++);
		}
	}
}

void atomic_storage_futex::wait(const void* data, std::size_t size, u64 old_value, u64 timeout, u64 mask)
{
	if (!timeout)
	{
		return;
	}

	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	atomic_t<u64>& entry = s_hashtable[(iptr >> 2) % s_hashtable_size];

	u32 new_value = 0;

	bool fallback = false;

	u32 sema_id = -1;

	const auto [_, ok] = entry.fetch_op([&](u64& value)
	{
		if ((value & s_waiter_mask) == s_waiter_mask || (value & s_signal_mask) == s_signal_mask)
		{
			// Return immediately on waiter overflow or signal overflow
			return false;
		}

#ifndef USE_FUTEX
		sema_id = (value & s_sema_mask) >> 2;
#endif

		if (!value || (value & s_pointer_mask) == (iptr & s_pointer_mask))
		{
			// Store pointer bits
			value |= (iptr & s_pointer_mask);
			fallback = false;
		}
		else
		{
			// Set collision bit
			value |= s_collision_bit;
			fallback = true;
		}

		// Add waiter
		value += s_waiter_mask & -s_waiter_mask;
		new_value = static_cast<u32>(value);
		return true;
	});

	if (!ok)
	{
		return;
	}

#ifndef USE_FUTEX
	for (u32 loop_count = 0; !fallback && loop_count < 7; loop_count++)
	{
		// Try to allocate a semaphore
		if (!sema_id)
		{
			const u32 sema = sema_alloc();

			if (!sema)
			{
				break;
			}

			sema_id = entry.atomic_op([&](u64& value) -> u32
			{
				if (value & s_sema_mask)
				{
					return (value & s_sema_mask) >> 2;
				}

				// Insert allocated semaphore
				value += s_signal_mask & -s_signal_mask;
				value |= (u64{sema} << 2);
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
		const auto [_old, ok] = entry.fetch_op([&](u64& value)
		{
			if ((value & s_signal_mask) == s_signal_mask)
			{
				return false;
			}

			if ((value & s_sema_mask) >> 2 != sema_id)
			{
				return false;
			}

			value += s_signal_mask & -s_signal_mask;
			return true;
		});

		if (!ok)
		{
			sema_free(sema_id);
			sema_id = 0;

			if ((_old & s_signal_mask) == s_signal_mask)
			{
				// Break on signal overflow
				break;
			}

			continue;
		}

		break;
	}
#endif

	if (fallback)
	{
		fallback_wait(data, size, old_value, timeout, mask);
	}
	else if (sema_id && ptr_cmp(data, size, old_value, mask) && s_tls_wait_cb(data))
	{
#ifndef USE_FUTEX
#if defined(_WIN32) && !defined(USE_POSIX)
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
#else
		struct timespec ts;
		ts.tv_sec  = timeout / 1'000'000'000;
		ts.tv_nsec = timeout % 1'000'000'000;

		futex(reinterpret_cast<char*>(&entry) + 4 * IS_BE_MACHINE, FUTEX_WAIT_PRIVATE, new_value, timeout + 1 ? &ts : nullptr);
#endif
	}

	if (!sema_id)
	{
		fallback = true;
	}

	while (true)
	{
		// Try to decrement
		const auto [prev, ok] = entry.fetch_op([&](u64& value)
		{
			if (value & s_waiter_mask)
			{
#ifndef USE_FUTEX
				// If timeout
				if (!fallback)
				{
					if ((value & s_signal_mask) == 0 || (value & s_sema_mask) >> 2 != sema_id)
					{
						return false;
					}

					value -= s_signal_mask & -s_signal_mask;

					if ((value & s_signal_mask) == 0)
					{
						value &= ~s_sema_mask;
					}
				}
#endif

				value -= s_waiter_mask & -s_waiter_mask;

				if ((value & s_waiter_mask) == 0)
				{
					// Reset on last waiter
					value = 0;
				}

				return true;
			}

			return false;
		});

		if (ok || fallback)
		{
			break;
		}

#ifndef USE_FUTEX
#if defined(_WIN32) && !defined(USE_POSIX)
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
#endif
	}

#ifndef USE_FUTEX
	if (sema_id)
	{
		sema_free(sema_id);
	}
#endif

	s_tls_wait_cb(nullptr);
}

#ifdef USE_FUTEX

void atomic_storage_futex::notify_one(const void* data)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	atomic_t<u64>& entry = s_hashtable[(iptr >> 2) % s_hashtable_size];

	const auto [prev, ok] = entry.fetch_op([&](u64& value)
	{
		if (value & s_waiter_mask && (value & s_pointer_mask) == (iptr & s_pointer_mask))
		{
			if ((value & s_signal_mask) == s_signal_mask)
			{
				// Signal overflow, do nothing
				return false;
			}

			value += s_signal_mask & -s_signal_mask;

			if ((value & s_signal_mask) == s_signal_mask)
			{
				// Signal will overflow, fallback to notify_all
				notify_all(data);
				return false;
			}

			return true;
		}
		else if (value & s_waiter_mask && value & s_collision_bit)
		{
			fallback_notify_one(data);
			return false;
		}

		return false;
	});

	if (ok)
	{
		futex(reinterpret_cast<char*>(&entry) + 4 * IS_BE_MACHINE, FUTEX_WAKE_PRIVATE, 1);
	}
}

void atomic_storage_futex::notify_all(const void* data)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	atomic_t<u64>& entry = s_hashtable[(iptr >> 2) % s_hashtable_size];

	const auto [_, ok] = entry.fetch_op([&](u64& value)
	{
		if (value & s_waiter_mask)
		{
			if ((value & s_signal_mask) == s_signal_mask)
			{
				// Signal overflow, do nothing
				return false;
			}

			if ((value & s_pointer_mask) == (iptr & s_pointer_mask))
			{
				value += s_signal_mask & -s_signal_mask;
				return true;
			}

			if (value & s_collision_bit)
			{
				fallback_notify_all(data);
				return false;
			}
		}

		return false;
	});

	if (ok)
	{
		futex(reinterpret_cast<char*>(&entry) + 4 * IS_BE_MACHINE, FUTEX_WAKE_PRIVATE, 0x7fffffff);
	}
}

#endif

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

#ifndef USE_FUTEX

void atomic_storage_futex::notify_one(const void* data)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	atomic_t<u64>& entry = s_hashtable[(iptr >> 2) % s_hashtable_size];

	const u64 value = entry;

	if (value & s_waiter_mask && (value & s_pointer_mask) == (iptr & s_pointer_mask))
	{
		if ((value & s_signal_mask) == 0 || (value & s_sema_mask) == 0)
		{
			// No relevant waiters, do nothing
			return;
		}
	}
	else if (value & s_waiter_mask && value & s_collision_bit)
	{
		fallback_notify_one(data);
		return;
	}
	else
	{
		return;
	}

	const u32 sema_id = (value & s_sema_mask) >> 2;

	if (!sema_get(sema_id))
	{
		return;
	}

	const auto [_, ok] = entry.fetch_op([&](u64& value)
	{
		if ((value & s_waiter_mask) == 0 || (value & s_pointer_mask) != (iptr & s_pointer_mask))
		{
			return false;
		}

		if ((value & s_signal_mask) == 0 || (value & s_sema_mask) >> 2 != sema_id)
		{
			return false;
		}

		value -= s_signal_mask & -s_signal_mask;

		// Reset allocated semaphore on last waiter
		if ((value & s_signal_mask) == 0)
		{
			value &= ~s_sema_mask;
		}

		return true;
	});

	if (ok)
	{
#ifdef USE_POSIX
		sem_post(&s_sema_list[sema_id]);
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

	atomic_t<u64>& entry = s_hashtable[(iptr >> 2) % s_hashtable_size];

	const u64 value = entry;

	if (value & s_waiter_mask && (value & s_pointer_mask) == (iptr & s_pointer_mask))
	{
		if ((value & s_signal_mask) == 0 || (value & s_sema_mask) == 0)
		{
			// No relevant waiters, do nothing
			return;
		}
	}
	else if (value & s_waiter_mask && value & s_collision_bit)
	{
		fallback_notify_all(data);
		return;
	}
	else
	{
		return;
	}

	const u32 sema_id = (value & s_sema_mask) >> 2;

	if (!sema_get(sema_id))
	{
		return;
	}

	const auto [_, count] = entry.fetch_op([&](u64& value) -> u32
	{
		if ((value & s_waiter_mask) == 0 || (value & s_pointer_mask) != (iptr & s_pointer_mask))
		{
			return 0;
		}

		if ((value & s_signal_mask) == 0 || (value & s_sema_mask) >> 2 != sema_id)
		{
			return 0;
		}

		const u32 r = (value & s_signal_mask) / (s_signal_mask & -s_signal_mask);
		value &= ~s_sema_mask;
		value &= ~s_signal_mask;
		return r;
	});

#ifdef USE_POSIX
	for (u32 i = 0; i < count; i++)
	{
		sem_post(&s_sema_list[sema_id]);
	}
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

#endif
