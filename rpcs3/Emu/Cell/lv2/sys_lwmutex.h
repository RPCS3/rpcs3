#pragma once

#include "sys_sync.h"

struct sys_lwmutex_attribute_t
{
	be_t<u32> protocol;
	be_t<u32> recursive;

	union
	{
		char name[8];
		u64 name_u64;
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
	const u64 name;

	semaphore<> mutex;
	atomic_t<u32> signaled{0};
	std::deque<cpu_thread*> sq;

	lv2_lwmutex(u32 protocol, vm::ptr<sys_lwmutex_t> control, u64 name)
		: protocol(protocol)
		, control(control)
		, name(name)
	{
	}
};

// Aux
class ppu_thread;

// Syscalls

error_code _sys_lwmutex_create(vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, u32 arg4, u64 name, u32 arg6);
error_code _sys_lwmutex_destroy(u32 lwmutex_id);
error_code _sys_lwmutex_lock(ppu_thread& ppu, u32 lwmutex_id, u64 timeout);
error_code _sys_lwmutex_trylock(u32 lwmutex_id);
error_code _sys_lwmutex_unlock(ppu_thread& ppu, u32 lwmutex_id);
