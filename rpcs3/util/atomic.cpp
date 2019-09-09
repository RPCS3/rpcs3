#include "atomic.hpp"

#include "Utilities/sync.h"

#include <map>
#include <mutex>
#include <condition_variable>

// Should be at least 65536, currently 2097152.
static constexpr std::uintptr_t s_hashtable_size = 1u << 21;

// TODO: it's probably better to implement more effective futex emulation for OSX/BSD here.
static atomic_t<u64> s_hashtable[s_hashtable_size];

// Pointer mask without bits used as hash, assuming signed 48-bit pointers
static constexpr u64 s_pointer_mask = 0xffff'ffff'ffff & ~(s_hashtable_size - 1);

// Max number of waiters is 65535
static constexpr u64 s_waiter_mask = 0xffff'0000'0000'0000;

// Implementation detail (remaining bits out of 32 available for futex)
static constexpr u64 s_signal_mask = 0xffffffff & ~(s_waiter_mask | s_pointer_mask);

// Callback for wait() function, returns false if wait should return
static thread_local bool(*s_tls_wait_cb)(const void* data) = [](const void*)
{
	return true;
};

static inline bool ptr_cmp(const void* data, std::size_t size, u64 old_value)
{
	switch (size)
	{
	case 1: return reinterpret_cast<const atomic_t<u8>*>(data)->load() == old_value;
	case 2: return reinterpret_cast<const atomic_t<u16>*>(data)->load() == old_value;
	case 4: return reinterpret_cast<const atomic_t<u32>*>(data)->load() == old_value;
	case 8: return reinterpret_cast<const atomic_t<u64>*>(data)->load() == old_value;
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

	void fallback_wait(const void* data, std::size_t size, u64 old_value, u64 timeout)
	{
		auto& wmap = get_fallback_map(data);

		if (!timeout)
		{
			return;
		}

		// Update node key
		s_tls_waiter.key() = data;

		if (std::unique_lock lock(wmap.mutex); ptr_cmp(data, size, old_value) && s_tls_wait_cb(data))
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

#if !defined(_WIN32) && !defined(__linux__)

void atomic_storage_futex::wait(const void* data, std::size_t size, u64 old_value, u64 timeout)
{
	fallback_wait(data, size, old_value, timeout);
}

void atomic_storage_futex::notify_one(const void* data)
{
	fallback_notify_one(data);
}

void atomic_storage_futex::notify_all(const void* data)
{
	fallback_notify_all(data);
}

#else

void atomic_storage_futex::wait(const void* data, std::size_t size, u64 old_value, u64 timeout)
{
	if (!timeout)
	{
		return;
	}

	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	atomic_t<u64>& entry = s_hashtable[iptr % s_hashtable_size];

	u32 new_value = 0;

	const auto [_, ok] = entry.fetch_op([&](u64& value)
	{
		if ((value & s_waiter_mask) == s_waiter_mask || (value & s_signal_mask) == s_signal_mask)
		{
			// Return immediately on waiter overflow or signal overflow
			return false;
		}

		if (!value || (value & s_pointer_mask) == (iptr & s_pointer_mask))
		{
			// Store pointer bits
			value |= (iptr & s_pointer_mask);
		}
		else
		{
			// Set pointer bits to all ones (collision, TODO)
			value |= s_pointer_mask;
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

	if (ptr_cmp(data, size, old_value) && s_tls_wait_cb(data))
	{
#ifdef _WIN32
		LARGE_INTEGER qw;
		qw.QuadPart = -static_cast<s64>(timeout / 100);

		if (timeout % 100)
		{
			// Round up to closest 100ns unit
			qw.QuadPart -= 1;
		}

		if (!NtWaitForKeyedEvent(nullptr, &entry, false, timeout + 1 ? &qw : nullptr))
		{
			// Return if no errors, continue if timed out
			s_tls_wait_cb(nullptr);
			return;
		}
#else
		struct timespec ts;
		ts.tv_sec  = timeout / 1'000'000'000;
		ts.tv_nsec = timeout % 1'000'000'000;

		futex(reinterpret_cast<char*>(&entry) + 4 * IS_BE_MACHINE, FUTEX_WAIT_PRIVATE, new_value, timeout + 1 ? &ts : nullptr);
#endif
	}

	while (true)
	{
		// Try to decrement
		const auto [prev, ok] = entry.fetch_op([&](u64& value)
		{
			if (value & s_waiter_mask)
			{
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

		if (ok)
		{
			break;
		}

#ifdef _WIN32
		static LARGE_INTEGER instant{};

		if (!NtWaitForKeyedEvent(nullptr, &entry, false, &instant))
		{
			break;
		}
#else
		// Unreachable
		std::terminate();
#endif
	}

	s_tls_wait_cb(nullptr);
}

void atomic_storage_futex::notify_one(const void* data)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	atomic_t<u64>& entry = s_hashtable[iptr % s_hashtable_size];

	bool fallback = false;

	const auto [prev, ok] = entry.fetch_op([&](u64& value)
	{
		if (value & s_waiter_mask && (value & s_pointer_mask) == (iptr & s_pointer_mask))
		{
#ifdef _WIN32
			// Try to decrement if no collision
			value -= s_waiter_mask & -s_waiter_mask;

			if ((value & s_waiter_mask) == 0)
			{
				// Reset on last waiter
				value = 0;
			}
#else
			if ((value & s_signal_mask) == s_signal_mask)
			{
				// Signal overflow, do nothing
				return false;
			}

			value += s_signal_mask & -s_signal_mask;

			if ((value & s_signal_mask) == s_signal_mask)
			{
				// Signal will overflow, fallback
				fallback = true;
				return false;
			}
#endif

			return true;
		}

		if (value & s_waiter_mask && (value & s_pointer_mask) == s_pointer_mask)
		{
			// Collision, notify everything
			fallback = true;
		}

		return false;
	});

	if (fallback)
	{
		notify_all(data);
		return;
	}

	if (ok)
	{
#ifdef _WIN32
		NtReleaseKeyedEvent(nullptr, &entry, false, nullptr);
#else
		futex(reinterpret_cast<char*>(&entry) + 4 * IS_BE_MACHINE, FUTEX_WAKE_PRIVATE, 1);
#endif
	}
}

void atomic_storage_futex::notify_all(const void* data)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data);

	atomic_t<u64>& entry = s_hashtable[iptr % s_hashtable_size];

	// Try to consume everything
#ifdef _WIN32
	const auto [old, ok] = entry.fetch_op([&](u64& value)
	{
		if (value & s_waiter_mask)
		{
			if ((value & s_pointer_mask) == s_pointer_mask || (value & s_pointer_mask) == (iptr & s_pointer_mask))
			{
				value = 0;
				return true;
			}
		}

		return false;
	});

	if (!ok)
	{
		return;
	}

	for (u64 count = old & s_waiter_mask; count; count -= s_waiter_mask & -s_waiter_mask)
	{
		NtReleaseKeyedEvent(nullptr, &entry, false, nullptr);
	}
#else
	const auto [_, ok] = entry.fetch_op([&](u64& value)
	{
		if (value & s_waiter_mask)
		{
			if ((value & s_signal_mask) == s_signal_mask)
			{
				// Signal overflow, do nothing
				return false;
			}

			if ((value & s_pointer_mask) == s_pointer_mask || (value & s_pointer_mask) == (iptr & s_pointer_mask))
			{
				value += s_signal_mask & -s_signal_mask;
				return true;
			}
		}

		return false;
	});

	if (ok)
	{
		futex(reinterpret_cast<char*>(&entry) + 4 * IS_BE_MACHINE, FUTEX_WAKE_PRIVATE, 0x7fffffff);
	}
#endif
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
