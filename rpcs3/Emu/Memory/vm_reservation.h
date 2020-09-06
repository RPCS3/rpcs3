#pragma once

#include "vm.h"
#include "Utilities/cond.h"
#include "util/atomic.hpp"

namespace vm
{
	enum reservation_lock_bit : u64
	{
		stcx_lockb = 1 << 0, // Exclusive conditional reservation lock
		dma_lockb = 1 << 1, // Inexclusive unconditional reservation lock
		putlluc_lockb = 1 << 6, // Exclusive unconditional reservation lock
	};

	// Get reservation status for further atomic update: last update timestamp
	inline atomic_t<u64>& reservation_acquire(u32 addr, u32 size)
	{
		// Access reservation info: stamp and the lock bit
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	// Update reservation status
	inline std::pair<bool, u64> try_reservation_update(u32 addr, u32 size, bool lsb = false)
	{
		// Update reservation info with new timestamp
		auto& res = reservation_acquire(addr, size);
		const u64 rtime = res;

		return {!(rtime & 127) && res.compare_and_swap_test(rtime, rtime + 128), rtime};
	}

	void reservation_update(u32 addr, u32 size, bool lsb = false);

	// Get reservation sync variable
	inline atomic_t<u64>& reservation_notifier(u32 addr, u32 size)
	{
		return *reinterpret_cast<atomic_t<u64>*>(g_reservations + (addr & 0xff80) / 2);
	}

	u64 reservation_lock_internal(u32, atomic_t<u64>&, u64);

	inline bool reservation_trylock(atomic_t<u64>& res, u64 rtime, u64 lock_bits = stcx_lockb)
	{
		if (res.compare_and_swap_test(rtime, rtime + lock_bits)) [[likely]]
		{
			return true;
		}

		return false;
	}

	inline std::pair<atomic_t<u64>&, u64> reservation_lock(u32 addr, u32 size, u64 lock_bits = stcx_lockb)
	{
		auto res = &vm::reservation_acquire(addr, size);
		auto rtime = res->load();

		if (rtime & 127 || !reservation_trylock(*res, rtime, lock_bits)) [[unlikely]]
		{
			static atomic_t<u64> no_lock{};

			rtime = reservation_lock_internal(addr, *res, lock_bits);

			if (rtime == umax)
			{
				res = &no_lock;
			}
		}

		return {*res, rtime};
	}
} // namespace vm
