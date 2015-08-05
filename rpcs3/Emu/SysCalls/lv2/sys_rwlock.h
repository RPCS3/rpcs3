#pragma once

#include "sleep_queue.h"

namespace vm { using namespace ps3; }

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

	std::atomic<u32> readers{ 0 }; // reader lock count
	std::shared_ptr<CPUThread> writer; // writer lock owner

	sleep_queue_t rsq; // threads trying to acquire readed lock
	sleep_queue_t wsq; // threads trying to acquire writer lock

	lv2_rwlock_t(u32 protocol, u64 name)
		: protocol(protocol)
		, name(name)
	{
	}

	void notify_all(lv2_lock_t& lv2_lock);
};

// Aux
class PPUThread;

// SysCalls
s32 sys_rwlock_create(vm::ptr<u32> rw_lock_id, vm::ptr<sys_rwlock_attribute_t> attr);
s32 sys_rwlock_destroy(u32 rw_lock_id);
s32 sys_rwlock_rlock(PPUThread& ppu, u32 rw_lock_id, u64 timeout);
s32 sys_rwlock_tryrlock(u32 rw_lock_id);
s32 sys_rwlock_runlock(u32 rw_lock_id);
s32 sys_rwlock_wlock(PPUThread& ppu, u32 rw_lock_id, u64 timeout);
s32 sys_rwlock_trywlock(PPUThread& ppu, u32 rw_lock_id);
s32 sys_rwlock_wunlock(PPUThread& ppu, u32 rw_lock_id);
