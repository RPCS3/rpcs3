#pragma once

#include "sys_sync.h"

struct lv2_mutex;

struct sys_cond_attribute_t
{
	be_t<u32> pshared;
	be_t<s32> flags;
	be_t<u64> ipc_key;

	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct lv2_cond final : lv2_obj
{
	static const u32 id_base = 0x86000000;

	const u64 name;
	const std::shared_ptr<lv2_mutex> mutex; // associated mutex

	sleep_queue<cpu_thread> sq;

	lv2_cond(const std::shared_ptr<lv2_mutex>& mutex, u64 name)
		: mutex(mutex)
		, name(name)
	{
	}

	void notify(lv2_lock_t, cpu_thread* thread);
};

class ppu_thread;

// SysCalls
s32 sys_cond_create(vm::ps3::ptr<u32> cond_id, u32 mutex_id, vm::ps3::ptr<sys_cond_attribute_t> attr);
s32 sys_cond_destroy(u32 cond_id);
s32 sys_cond_wait(ppu_thread& ppu, u32 cond_id, u64 timeout);
s32 sys_cond_signal(u32 cond_id);
s32 sys_cond_signal_all(u32 cond_id);
s32 sys_cond_signal_to(u32 cond_id, u32 thread_id);
