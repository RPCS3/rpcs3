#pragma once

#include "sys_sync.h"

struct sys_mutex_attribute_t
{
	be_t<u32> protocol; // SYS_SYNC_FIFO, SYS_SYNC_PRIORITY or SYS_SYNC_PRIORITY_INHERIT
	be_t<u32> recursive; // SYS_SYNC_RECURSIVE or SYS_SYNC_NOT_RECURSIVE
	be_t<u32> pshared;
	be_t<u32> adaptive;
	be_t<u64> ipc_key;
	be_t<s32> flags;
	be_t<u32> pad;

	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct lv2_mutex_t
{
	static const u32 id_base = 0x85000000;
	static const u32 id_step = 0x100;
	static const u32 id_count = 8192;

	const bool recursive;
	const u32 protocol;
	const u64 name;

	atomic_t<u32> cond_count{ 0 }; // count of condition variables associated
	atomic_t<u32> recursive_count{ 0 }; // count of recursive locks
	std::shared_ptr<cpu_thread> owner; // current mutex owner

	sleep_queue<cpu_thread> sq;

	lv2_mutex_t(bool recursive, u32 protocol, u64 name)
		: recursive(recursive)
		, protocol(protocol)
		, name(name)
	{
	}

	void unlock(lv2_lock_t);
};

class ppu_thread;

// SysCalls
s32 sys_mutex_create(vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute_t> attr);
s32 sys_mutex_destroy(u32 mutex_id);
s32 sys_mutex_lock(ppu_thread& ppu, u32 mutex_id, u64 timeout);
s32 sys_mutex_trylock(ppu_thread& ppu, u32 mutex_id);
s32 sys_mutex_unlock(ppu_thread& ppu, u32 mutex_id);
