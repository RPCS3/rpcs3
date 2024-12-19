#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

struct sys_rwlock_attribute_t
{
	be_t<u32> protocol;
	be_t<u32> pshared;
	be_t<u64> ipc_key;
	be_t<s32> flags;
	be_t<u32> pad;

	union
	{
		nse_t<u64, 1> name_u64;
		char name[sizeof(u64)];
	};
};

struct lv2_rwlock final : lv2_obj
{
	static const u32 id_base = 0x88000000;

	const lv2_protocol protocol;
	const u64 key;
	const u64 name;

	shared_mutex mutex;
	atomic_t<s64> owner{0};
	ppu_thread* rq{};
	ppu_thread* wq{};

	lv2_rwlock(u32 protocol, u64 key, u64 name) noexcept
		: protocol{static_cast<u8>(protocol)}
		, key(key)
		, name(name)
	{
	}

	lv2_rwlock(utils::serial& ar);
	static std::function<void(void*)> load(utils::serial& ar);
	void save(utils::serial& ar);
};

// Aux
class ppu_thread;

// Syscalls

error_code sys_rwlock_create(ppu_thread& ppu, vm::ptr<u32> rw_lock_id, vm::ptr<sys_rwlock_attribute_t> attr);
error_code sys_rwlock_destroy(ppu_thread& ppu, u32 rw_lock_id);
error_code sys_rwlock_rlock(ppu_thread& ppu, u32 rw_lock_id, u64 timeout);
error_code sys_rwlock_tryrlock(ppu_thread& ppu, u32 rw_lock_id);
error_code sys_rwlock_runlock(ppu_thread& ppu, u32 rw_lock_id);
error_code sys_rwlock_wlock(ppu_thread& ppu, u32 rw_lock_id, u64 timeout);
error_code sys_rwlock_trywlock(ppu_thread& ppu, u32 rw_lock_id);
error_code sys_rwlock_wunlock(ppu_thread& ppu, u32 rw_lock_id);

constexpr auto _sys_rwlock_trywlock = sys_rwlock_trywlock;
