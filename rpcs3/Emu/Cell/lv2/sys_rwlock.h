#pragma once

#include "sys_sync.h"

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

struct lv2_rwlock_t
{
	const u64 name;
	const u32 protocol;

	atomic_t<u32> readers{ 0 }; // reader lock count
	std::shared_ptr<cpu_thread> writer; // writer lock owner

	sleep_queue<cpu_thread> rsq; // threads trying to acquire readed lock
	sleep_queue<cpu_thread> wsq; // threads trying to acquire writer lock

	lv2_rwlock_t(u32 protocol, u64 name)
		: protocol(protocol)
		, name(name)
	{
	}

	void notify_all(lv2_lock_t);
};

// Aux
class ppu_thread;

// SysCalls
s32 sys_rwlock_create(vm::ptr<u32> rw_lock_id, vm::ptr<sys_rwlock_attribute_t> attr);
s32 sys_rwlock_destroy(u32 rw_lock_id);
s32 sys_rwlock_rlock(ppu_thread& ppu, u32 rw_lock_id, u64 timeout);
s32 sys_rwlock_tryrlock(u32 rw_lock_id);
s32 sys_rwlock_runlock(u32 rw_lock_id);
s32 sys_rwlock_wlock(ppu_thread& ppu, u32 rw_lock_id, u64 timeout);
s32 sys_rwlock_trywlock(ppu_thread& ppu, u32 rw_lock_id);
s32 sys_rwlock_wunlock(ppu_thread& ppu, u32 rw_lock_id);
