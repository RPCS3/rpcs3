﻿#include "atomic.hpp"

#include "Utilities/sync.h"

#include <map>
#include <mutex>
#include <condition_variable>

// Should be at least 65536, currently 2097152.
static constexpr std::uintptr_t s_hashtable_size = 1u << 21;

// TODO: it's probably better to implement more effective futex emulation for OSX/BSD here.
static atomic_t<s64> s_hashtable[s_hashtable_size];

// Max number of waiters (16383)
static constexpr s64 s_waiter_mask = 0x3fff;

// Implementation detail (remaining bits out of 32)
static constexpr s64 s_signal_mask = 0xffffffff & ~s_waiter_mask;

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

	void fallback_wait(const void* data, std::size_t size, u64 old_value)
	{
		auto& wmap = get_fallback_map(data);

		// Update node key
		s_tls_waiter.key() = data;

		if (std::unique_lock lock(wmap.mutex); ptr_cmp(data, size, old_value))
		{
			// Add node to the waiter list
			std::condition_variable& cond = wmap.list.insert(std::move(s_tls_waiter))->second.cond;

			// Wait until the node is returned to its TLS location
			while (!s_tls_waiter)
			{
				cond.wait(lock);
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

void atomic_storage_futex::wait(const void* data, std::size_t size, u64 old_value)
{
	fallback_wait(data, size, old_value);
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

void atomic_storage_futex::wait(const void* data, std::size_t size, u64 old_value)
{
#ifdef _WIN32
	if (OptWaitOnAddress)
	{
		OptWaitOnAddress(const_cast<volatile void*>(data), &old_value, size, INFINITE);
		return;
	}
#endif

	const std::intptr_t iptr = reinterpret_cast<std::intptr_t>(data);

	atomic_t<s64>& entry = s_hashtable[iptr % s_hashtable_size];

	u32 new_value = 0;

	const auto [_, ok] = entry.fetch_op([&](s64& value)
	{
		if ((value & s_waiter_mask) == s_waiter_mask || (value & s_signal_mask) == s_signal_mask)
		{
			// Return immediately on waiter overflow or signal overflow
			return false;
		}

		if (!value || (value >> 32) == (iptr >> 16))
		{
			// Store 32 highest bits of signed 48-bit pointer
			value |= (iptr >> 16) * 0x1'0000'0000;
		}
		else
		{
			// Zero highest bits (collision)
			value &= 0xffffffff;
		}

		new_value = static_cast<u32>(value += 1);
		return true;
	});

	if (!ok)
	{
		return;
	}

	if (ptr_cmp(data, size, old_value))
	{
#ifdef _WIN32
		NtWaitForKeyedEvent(nullptr, &entry, false, nullptr);
		return;
#else
		futex(reinterpret_cast<char*>(&entry) + 4 * IS_BE_MACHINE, FUTEX_WAIT_PRIVATE, new_value, nullptr);
#endif
	}

	while (true)
	{
		// Try to decrement
		const auto [prev, ok] = entry.fetch_op([&](s64& value)
		{
			if (value & s_waiter_mask)
			{
				value -= 1;

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
}

void atomic_storage_futex::notify_one(const void* data)
{
#ifdef _WIN32
	if (OptWaitOnAddress)
	{
		OptWakeByAddressSingle(const_cast<void*>(data));
		return;
	}
#endif

	const std::intptr_t iptr = reinterpret_cast<std::intptr_t>(data);

	atomic_t<s64>& entry = s_hashtable[iptr % s_hashtable_size];

	const auto [prev, ok] = entry.fetch_op([&](s64& value)
	{
		if (value & s_waiter_mask && (value >> 32) == (iptr >> 16))
		{
#ifdef _WIN32
			// Try to decrement if no collision
			value -= 1;

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
#endif

			return true;
		}

		return false;
	});

	if (ok)
	{
#ifdef _WIN32
		NtReleaseKeyedEvent(nullptr, &entry, false, nullptr);
		return;
#else
		futex(reinterpret_cast<char*>(&entry) + 4 * IS_BE_MACHINE, FUTEX_WAKE_PRIVATE, 1);
		return;
#endif
	}

	if (prev)
	{
		// Collision, notify everything
		notify_all(data);
	}
}

void atomic_storage_futex::notify_all(const void* data)
{
#ifdef _WIN32
	if (OptWaitOnAddress)
	{
		OptWakeByAddressAll(const_cast<void*>(data));
		return;
	}
#endif

	const std::intptr_t iptr = reinterpret_cast<std::intptr_t>(data);

	atomic_t<s64>& entry = s_hashtable[iptr % s_hashtable_size];

	// Consume everything
#ifdef _WIN32
	for (s64 count = entry.exchange(0) & s_waiter_mask; count; count--)
	{
		NtReleaseKeyedEvent(nullptr, &entry, false, nullptr);
	}
#else
	const auto [_, ok] = entry.fetch_op([&](s64& value)
	{
		if (value & s_waiter_mask)
		{
			if ((value & s_signal_mask) == s_signal_mask)
			{
				// Signal overflow, do nothing
				return false;
			}

			value += s_signal_mask & -s_signal_mask;
			return true;
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
