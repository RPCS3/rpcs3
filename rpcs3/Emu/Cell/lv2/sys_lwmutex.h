#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

struct sys_lwmutex_attribute_t
{
	be_t<u32> protocol;
	be_t<u32> recursive;

	union
	{
		nse_t<u64, 1> name_u64;
		char name[sizeof(u64)];
	};
};

enum : u32
{
	lwmutex_free     = 0xffffffffu,
	lwmutex_dead     = 0xfffffffeu,
	lwmutex_reserved = 0xfffffffdu,
};

struct sys_lwmutex_t
{
	struct alignas(8) sync_var_t
	{
		be_t<u32> owner;
		be_t<u32> waiter;
	};

	union
	{
		atomic_t<sync_var_t> lock_var;

		struct
		{
			atomic_be_t<u32> owner;
			atomic_be_t<u32> waiter;
		}
		vars;

		atomic_be_t<u64> all_info;
	};

	be_t<u32> attribute;
	be_t<u32> recursive_count;
	be_t<u32> sleep_queue; // lwmutex pseudo-id
	be_t<u32> pad;
};

struct lv2_lwmutex final : lv2_obj
{
	static const u32 id_base = 0x95000000;

	const u32 protocol;
	const vm::ptr<sys_lwmutex_t> control;
	const be_t<u64> name;

	shared_mutex mutex;
	atomic_t<s32> signaled{0};
	std::deque<cpu_thread*> sq;
	atomic_t<s32> lwcond_waiters{0};

	lv2_lwmutex(u32 protocol, vm::ptr<sys_lwmutex_t> control, u64 name)
		: protocol(protocol)
		, control(control)
		, name(std::bit_cast<be_t<u64>>(name))
	{
	}

	// Try to add a waiter 
	bool add_waiter(cpu_thread* cpu)
	{
		if (const auto old = lwcond_waiters.fetch_op([](s32& val)
		{
			if (val + 0u <= 1u << 31)
			{
				// Value was either positive or INT32_MIN
				return false;
			}

			// lwmutex was set to be destroyed, but there are lwcond waiters
			// Turn off the "destroying" bit as we are adding an lwmutex waiter
			val &= 0x7fff'ffff;
			return true;
		}).first; old != INT32_MIN)
		{
			sq.emplace_back(cpu);

			if (old < 0)
			{
				// Notify lwmutex destroyer (may cause EBUSY to be returned for it)
				lwcond_waiters.notify_all();
			}

			return true;
		}

		// Failed - lwmutex was set to be destroyed and all lwcond waiters quit 
		return false;
	}
};

// Aux
class ppu_thread;

// Syscalls

error_code _sys_lwmutex_create(ppu_thread& ppu, vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, s32 has_name, u64 name);
error_code _sys_lwmutex_destroy(ppu_thread& ppu, u32 lwmutex_id);
error_code _sys_lwmutex_lock(ppu_thread& ppu, u32 lwmutex_id, u64 timeout);
error_code _sys_lwmutex_trylock(ppu_thread& ppu, u32 lwmutex_id);
error_code _sys_lwmutex_unlock(ppu_thread& ppu, u32 lwmutex_id);
error_code _sys_lwmutex_unlock2(ppu_thread& ppu, u32 lwmutex_id);
