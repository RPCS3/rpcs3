#pragma once

#include "sleep_queue.h"

namespace vm { using namespace ps3; }

struct lv2_mutex_t;

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

struct lv2_cond_t
{
	const u64 name;
	const std::shared_ptr<lv2_mutex_t> mutex; // associated mutex

	sleep_queue_t sq;

	std::atomic<u32> sent{ 0 };
	std::atomic<u32> recv{ 0 };

	lv2_cond_t(const std::shared_ptr<lv2_mutex_t>& mutex, u64 name)
		: mutex(mutex)
		, name(name)
	{
	}
};

REG_ID_TYPE(lv2_cond_t, 0x86); // SYS_COND_OBJECT

class PPUThread;

// SysCalls
s32 sys_cond_create(vm::ptr<u32> cond_id, u32 mutex_id, vm::ptr<sys_cond_attribute_t> attr);
s32 sys_cond_destroy(u32 cond_id);
s32 sys_cond_wait(PPUThread& ppu, u32 cond_id, u64 timeout);
s32 sys_cond_signal(u32 cond_id);
s32 sys_cond_signal_all(u32 cond_id);
s32 sys_cond_signal_to(u32 cond_id, u32 thread_id);
