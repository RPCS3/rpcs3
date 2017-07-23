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

struct lv2_rwlock final : lv2_obj
{
	static const u32 id_base = 0x88000000;

	const u32 protocol;
	const u32 shared;
	const u64 key;
	const u64 name;
	const s32 flags;

	semaphore<> mutex;
	atomic_t<s64> owner{0};
	std::deque<cpu_thread*> rq;
	std::deque<cpu_thread*> wq;

	lv2_rwlock(u32 protocol, u64 name)
		: protocol(protocol)
		, shared(0)
		, key(0)
		, flags(0)
		, name(name)
	{
	}
};

// Aux
class ppu_thread;

// Syscalls

error_code sys_rwlock_create(vm::ps3::ptr<u32> rw_lock_id, vm::ps3::ptr<sys_rwlock_attribute_t> attr);
error_code sys_rwlock_destroy(u32 rw_lock_id);
error_code sys_rwlock_rlock(ppu_thread& ppu, u32 rw_lock_id, u64 timeout);
error_code sys_rwlock_tryrlock(u32 rw_lock_id);
error_code sys_rwlock_runlock(ppu_thread& ppu, u32 rw_lock_id);
error_code sys_rwlock_wlock(ppu_thread& ppu, u32 rw_lock_id, u64 timeout);
error_code sys_rwlock_trywlock(ppu_thread& ppu, u32 rw_lock_id);
error_code sys_rwlock_wunlock(ppu_thread& ppu, u32 rw_lock_id);

constexpr auto _sys_rwlock_trywlock = sys_rwlock_trywlock;
