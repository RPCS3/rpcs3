#pragma once

#include "sys_sync.h"

struct sys_lwmutex_attribute_t
{
	be_t<u32> protocol;
	be_t<u32> recursive;
	char name[8];
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

struct lv2_lwmutex_t
{
	static const u32 id_base = 0x95000000;
	static const u32 id_step = 0x100;
	static const u32 id_count = 8192;

	const u32 protocol;
	const u64 name;

	// this object is not truly a mutex and its syscall names may be wrong, it's probably a sleep queue or something
	atomic_t<u32> signaled{ 0 };

	sleep_queue<cpu_thread> sq;

	lv2_lwmutex_t(u32 protocol, u64 name)
		: protocol(protocol)
		, name(name)
	{
	}

	void unlock(lv2_lock_t);
};

// Aux
class ppu_thread;

// SysCalls
s32 _sys_lwmutex_create(vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, u32 arg4, u64 name, u32 arg6);
s32 _sys_lwmutex_destroy(u32 lwmutex_id);
s32 _sys_lwmutex_lock(ppu_thread& ppu, u32 lwmutex_id, u64 timeout);
s32 _sys_lwmutex_trylock(u32 lwmutex_id);
s32 _sys_lwmutex_unlock(u32 lwmutex_id);
