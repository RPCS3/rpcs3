#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

struct sys_semaphore_attribute_t
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

struct lv2_sema final : lv2_obj
{
	static const u32 id_base = 0x96000000;

	const u32 protocol;
	const u32 shared;
	const u64 key;
	const u64 name;
	const s32 flags;
	const s32 max;

	shared_mutex mutex;
	atomic_t<s32> val;
	std::deque<cpu_thread*> sq;

	lv2_sema(u32 protocol, u32 shared, u64 key, s32 flags, u64 name, s32 max, s32 value)
		: protocol(protocol)
		, shared(shared)
		, key(key)
		, name(name)
		, flags(flags)
		, max(max)
		, val(value)
	{
	}
};

// Aux
class ppu_thread;

// Syscalls

error_code sys_semaphore_create(ppu_thread& ppu, vm::ptr<u32> sem_id, vm::ptr<sys_semaphore_attribute_t> attr, s32 initial_val, s32 max_val);
error_code sys_semaphore_destroy(ppu_thread& ppu, u32 sem_id);
error_code sys_semaphore_wait(ppu_thread& ppu, u32 sem_id, u64 timeout);
error_code sys_semaphore_trywait(ppu_thread& ppu, u32 sem_id);
error_code sys_semaphore_post(ppu_thread& ppu, u32 sem_id, s32 count);
error_code sys_semaphore_get_value(ppu_thread& ppu, u32 sem_id, vm::ptr<s32> count);
