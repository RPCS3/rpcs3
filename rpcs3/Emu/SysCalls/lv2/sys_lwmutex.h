#pragma once

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

struct sys_lwmutex_t
{
	atomic_t<u32> owner;
	atomic_t<u32> waiter; // currently not used
	be_t<u32> attribute;
	be_t<u32> recursive_count;
	be_t<u32> sleep_queue;
	be_t<u32> pad;

	u64& all_info()
	{
		return *(reinterpret_cast<u64*>(this));
	}

	s32 trylock(be_t<u32> tid);
	s32 unlock(be_t<u32> tid);
	s32 lock(be_t<u32> tid, u64 timeout);
};

// Aux
s32 lwmutex_create(sys_lwmutex_t& lwmutex, u32 protocol, u32 recursive, u64 name_u64);

class PPUThread;

// SysCalls
s32 sys_lwmutex_create(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr);
s32 sys_lwmutex_destroy(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_lock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout);
s32 sys_lwmutex_trylock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_unlock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex);
