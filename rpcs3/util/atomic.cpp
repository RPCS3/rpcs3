#include "atomic.hpp"

#include "Utilities/sync.h"

// Should be at least 65536, currently 1048576.
static constexpr std::uintptr_t s_hashtable_size = 1u << 20;

// ^2 means adjacent addresses within the same aligned u32 word will always collide.
static constexpr uint s_ignored_lsbits = 2;

// TODO: it's probably better to implement more effective futex emulation for OSX/BSD here.
static atomic_t<s64> s_hashtable[s_hashtable_size];

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

	atomic_t<s64>& entry = s_hashtable[(iptr >> s_ignored_lsbits) % s_hashtable_size];

	u32 new_value = 0;

	const auto [_, ok] = entry.fetch_op([&](s64& value)
	{
		if ((value & 0xffff) == 0xffff || (value & 0xffff0000) == s64{0xffff0000})
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

		new_value = static_cast<u32>(++value);
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
			if (value & 0xffff)
			{
				value--;

				if ((value & 0xffff) == 0)
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

	atomic_t<s64>& entry = s_hashtable[(iptr >> s_ignored_lsbits) % s_hashtable_size];

	const auto [prev, ok] = entry.fetch_op([&](s64& value)
	{
		if (value & 0xffff && (value >> 32) == (iptr >> 16))
		{
#ifdef _WIN32
			// Try to decrement if no collision
			value--;

			if ((value & 0xffff) == 0)
			{
				// Reset on last waiter
				value = 0;
			}
#else
			if ((value & 0xffff0000) == s64{0xffff0000})
			{
				// Signal overflow, do nothing
				return false;
			}

			value += 0x10000;
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

	atomic_t<s64>& entry = s_hashtable[(iptr >> s_ignored_lsbits) % s_hashtable_size];

	// Consume everything
#ifdef _WIN32
	for (uint count = static_cast<u16>(entry.exchange(0)); count; count--)
	{
		NtReleaseKeyedEvent(nullptr, &entry, false, nullptr);
	}
#else
	const auto [_, ok] = entry.fetch_op([&](s64& value)
	{
		if (value & 0xffff)
		{
			if ((value & 0xffff0000) == s64{0xffff0000})
			{
				// Signal overflow, do nothing
				return false;
			}

			value += 0x10000;
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
