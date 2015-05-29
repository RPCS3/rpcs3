#pragma once

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
	const bool recursive;
	const u32 protocol;
	const u64 name;

	std::atomic<u32> cond_count; // count of condition variables associated
	std::atomic<u32> recursive_count;
	std::weak_ptr<CPUThread> owner;

	// TODO: use sleep queue, possibly remove condition variable
	std::condition_variable cv;
	std::atomic<u32> waiters;

	lv2_mutex_t(bool recursive, u32 protocol, u64 name)
		: recursive(recursive)
		, protocol(protocol)
		, name(name)
		, cond_count(0)
		, recursive_count(0)
		, waiters(0)
	{
	}
};

REG_ID_TYPE(lv2_mutex_t, 0x85); // SYS_MUTEX_OBJECT

class PPUThread;

// SysCalls
s32 sys_mutex_create(vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute_t> attr);
s32 sys_mutex_destroy(u32 mutex_id);
s32 sys_mutex_lock(PPUThread& CPU, u32 mutex_id, u64 timeout);
s32 sys_mutex_trylock(PPUThread& CPU, u32 mutex_id);
s32 sys_mutex_unlock(PPUThread& CPU, u32 mutex_id);
