#pragma once

#include "vm.h"
#include "Utilities/cond.h"
#include "util/atomic.hpp"

namespace vm
{
	// Get reservation status for further atomic update: last update timestamp
	inline atomic_t<u64>& reservation_acquire(u32 addr, u32 size)
	{
		// Access reservation info: stamp and the lock bit
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	// Update reservation status
	inline void reservation_update(u32 addr, u32 size, bool lsb = false)
	{
		// Update reservation info with new timestamp
		reservation_acquire(addr, size) += 128;
	}

	// Get reservation sync variable
	inline atomic_t<u64>& reservation_notifier(u32 addr, u32 size)
	{
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	bool reservation_lock_internal(u32, atomic_t<u64>&);

	inline atomic_t<u64>& reservation_lock(u32 addr, u32 size)
	{
		auto res = &vm::reservation_acquire(addr, size);

		if (res->bts(0)) [[unlikely]]
		{
			static atomic_t<u64> no_lock{};

			if (!reservation_lock_internal(addr, *res))
			{
				res = &no_lock;
			}
		}

		return *res;
	}

	inline bool reservation_trylock(atomic_t<u64>& res, u64 rtime)
	{
		if (res.compare_and_swap_test(rtime, rtime | 1)) [[likely]]
		{
			return true;
		}

		return false;
	}

} // namespace vm
