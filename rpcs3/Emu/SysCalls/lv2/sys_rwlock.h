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

struct RWLock
{
	struct sync_var_t
	{
		u32 readers; // reader count
		u32 writer; // writer thread id
	};

	sleep_queue_t wqueue;
	atomic_le_t<sync_var_t> sync;

	const u32 protocol;

	RWLock(u32 protocol, u64 name)
		: protocol(protocol)
		, wqueue(name)
	{
		sync.write_relaxed({ 0, 0 });
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
