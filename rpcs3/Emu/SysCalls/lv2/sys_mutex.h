#pragma once

struct sys_mutex_attribute
{
	be_t<u32> protocol; // SYS_SYNC_FIFO, SYS_SYNC_PRIORITY or SYS_SYNC_PRIORITY_INHERIT
	be_t<u32> recursive; // SYS_SYNC_RECURSIVE or SYS_SYNC_NOT_RECURSIVE
	be_t<u32> pshared; // always 0x200 (not shared)
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

struct Mutex
{
	atomic_le_t<u32> id;
	atomic_le_t<u32> owner;
	std::atomic<u32> recursive_count; // recursive locks count
	std::atomic<u32> cond_count; // count of condition variables associated
	sleep_queue_t queue;

	const u32 protocol;
	const bool is_recursive;

	Mutex(u32 protocol, bool is_recursive, u64 name)
		: protocol(protocol)
		, is_recursive(is_recursive)
		, queue(name)
		, cond_count(0)
	{
		owner.write_relaxed(0);
	}

	~Mutex();
};

class PPUThread;

// SysCalls
s32 sys_mutex_create(PPUThread& CPU, vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute> attr);
s32 sys_mutex_destroy(PPUThread& CPU, u32 mutex_id);
s32 sys_mutex_lock(PPUThread& CPU, u32 mutex_id, u64 timeout);
s32 sys_mutex_trylock(PPUThread& CPU, u32 mutex_id);
s32 sys_mutex_unlock(PPUThread& CPU, u32 mutex_id);
