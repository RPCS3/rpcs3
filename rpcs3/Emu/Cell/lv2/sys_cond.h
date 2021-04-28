#pragma once

#include "sys_sync.h"
#include "sys_mutex.h"

struct lv2_mutex;

struct sys_cond_attribute_t
{
	be_t<u32> pshared;
	be_t<s32> flags;
	be_t<u64> ipc_key;

	union
	{
		nse_t<u64, 1> name_u64;
		char name[sizeof(u64)];
	};
};

struct lv2_cond final : lv2_obj
{
	static const u32 id_base = 0x86000000;

	const u32 shared;
	const u64 key;
	const u64 name;
	const u32 mtx_id;

	std::shared_ptr<lv2_mutex> mutex; // Associated Mutex
	atomic_t<u32> waiters{0};
	std::deque<cpu_thread*> sq;

	lv2_cond(u32 shared, u64 key, u64 name, u32 mtx_id, std::shared_ptr<lv2_mutex> mutex)
		: shared(shared)
		, key(key)
		, name(name)
		, mtx_id(mtx_id)
		, mutex(std::move(mutex))
	{
	}

	CellError on_id_create()
	{
		if (mutex->exists)
		{
			mutex->cond_count++;
			exists++;
			return {};
		}

		// Mutex has been destroyed, cannot create conditional variable
		return CELL_ESRCH;
	}
};

class ppu_thread;

// Syscalls

error_code sys_cond_create(ppu_thread& ppu, vm::ptr<u32> cond_id, u32 mutex_id, vm::ptr<sys_cond_attribute_t> attr);
error_code sys_cond_destroy(ppu_thread& ppu, u32 cond_id);
error_code sys_cond_wait(ppu_thread& ppu, u32 cond_id, u64 timeout);
error_code sys_cond_signal(ppu_thread& ppu, u32 cond_id);
error_code sys_cond_signal_all(ppu_thread& ppu, u32 cond_id);
error_code sys_cond_signal_to(ppu_thread& ppu, u32 cond_id, u32 thread_id);
