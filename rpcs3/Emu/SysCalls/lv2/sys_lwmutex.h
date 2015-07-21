#pragma once

#include "sleep_queue.h"

namespace vm { using namespace ps3; }

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
	struct sync_var_t
	{
		be_t<u32> owner;
		be_t<u32> waiter;
	};

	union
	{
		atomic_be_t<sync_var_t> lock_var;

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
	const u32 protocol;
	const u64 name;

	// this object is not truly a mutex and its syscall names may be wrong, it's probably a sleep queue or something
	std::atomic<u32> signaled{ 0 };

	sleep_queue_t sq;

	lv2_lwmutex_t(u32 protocol, u64 name)
		: protocol(protocol)
		, name(name)
	{
	}

	void unlock(lv2_lock_t& lv2_lock);
};

REG_ID_TYPE(lv2_lwmutex_t, 0x95); // SYS_LWMUTEX_OBJECT

// Aux
class PPUThread;

// SysCalls
s32 _sys_lwmutex_create(vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, u32 arg4, u64 name, u32 arg6);
s32 _sys_lwmutex_destroy(u32 lwmutex_id);
s32 _sys_lwmutex_lock(PPUThread& ppu, u32 lwmutex_id, u64 timeout);
s32 _sys_lwmutex_trylock(u32 lwmutex_id);
s32 _sys_lwmutex_unlock(u32 lwmutex_id);
