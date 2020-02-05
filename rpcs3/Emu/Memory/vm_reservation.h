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
		return reinterpret_cast<atomic_t<u64>*>(g_reservations)[addr / 128];
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
		return reinterpret_cast<atomic_t<u64>*>(g_reservations)[addr / 128];
	}

	void reservation_lock_internal(atomic_t<u64>&);

	inline atomic_t<u64>& reservation_lock(u32 addr, u32 size)
	{
		auto& res = vm::reservation_acquire(addr, size);

		if (res.bts(0)) [[unlikely]]
		{
			reservation_lock_internal(res);
		}

		return res;
	}

} // namespace vm
