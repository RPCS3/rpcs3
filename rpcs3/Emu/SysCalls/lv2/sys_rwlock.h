#pragma once

struct sys_rwlock_attribute_t
{
	be_t<u32> protocol;
	be_t<u32> pshared;
	be_t<u64> ipc_key;
	be_t<s32> flags;
	be_t<u32> pad;

	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct rwlock_t
{
	const u32 protocol;
	const u64 name;

	std::atomic<u32> readers; // reader count
	std::atomic<u32> writer; // writer id

	// TODO: use sleep queue, possibly remove condition variable
	std::condition_variable cv;
	std::atomic<s32> waiters;

	rwlock_t(u32 protocol, u64 name)
		: protocol(protocol)
		, name(name)
		, readers(0)
		, writer(0)
		, waiters(0)
	{
	}
};

// SysCalls
s32 sys_rwlock_create(vm::ptr<u32> rw_lock_id, vm::ptr<sys_rwlock_attribute_t> attr);
s32 sys_rwlock_destroy(u32 rw_lock_id);
s32 sys_rwlock_rlock(u32 rw_lock_id, u64 timeout);
s32 sys_rwlock_tryrlock(u32 rw_lock_id);
s32 sys_rwlock_runlock(u32 rw_lock_id);
s32 sys_rwlock_wlock(PPUThread& CPU, u32 rw_lock_id, u64 timeout);
s32 sys_rwlock_trywlock(PPUThread& CPU, u32 rw_lock_id);
s32 sys_rwlock_wunlock(PPUThread& CPU, u32 rw_lock_id);
