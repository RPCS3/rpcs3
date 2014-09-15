#pragma once
#include "sys_lwmutex.h"

struct sys_mutex_attribute
{
	be_t<u32> protocol; // SYS_SYNC_FIFO, SYS_SYNC_PRIORITY or SYS_SYNC_PRIORITY_INHERIT
	be_t<u32> recursive; // SYS_SYNC_RECURSIVE or SYS_SYNC_NOT_RECURSIVE
	be_t<u32> pshared; // always 0x200 (not shared)
	be_t<u32> adaptive;
	be_t<u64> ipc_key;
	be_t<int> flags;
	be_t<u32> pad;
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct Mutex
{
	u32 id;
	SMutex m_mutex;
	SleepQueue m_queue;
	u32 recursive; // recursive locks count
	std::atomic<u32> cond_count; // count of condition variables associated

	const u32 protocol;
	const bool is_recursive;

	Mutex(u32 protocol, bool is_recursive, u64 name)
		: protocol(protocol)
		, is_recursive(is_recursive)
		, m_queue(name)
		, cond_count(0)
	{
	}

	~Mutex();
};

// SysCalls
s32 sys_mutex_create(vm::ptr<be_t<u32>> mutex_id, vm::ptr<sys_mutex_attribute> attr);
s32 sys_mutex_destroy(u32 mutex_id);
s32 sys_mutex_lock(u32 mutex_id, u64 timeout);
s32 sys_mutex_trylock(u32 mutex_id);
s32 sys_mutex_unlock(u32 mutex_id);
