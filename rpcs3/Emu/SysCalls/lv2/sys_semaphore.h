#pragma once

#include "sleep_queue.h"

namespace vm { using namespace ps3; }

struct sys_semaphore_attribute_t
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

struct lv2_sema_t
{
	const u32 protocol;
	const s32 max;
	const u64 name;

	std::atomic<s32> value;

	sleep_queue_t sq;

	lv2_sema_t(u32 protocol, s32 max, u64 name, s32 value)
		: protocol(protocol)
		, max(max)
		, name(name)
		, value(value)
	{
	}
};

// Aux
class PPUThread;

// SysCalls
s32 sys_semaphore_create(vm::ptr<u32> sem_id, vm::ptr<sys_semaphore_attribute_t> attr, s32 initial_val, s32 max_val);
s32 sys_semaphore_destroy(u32 sem_id);
s32 sys_semaphore_wait(PPUThread& ppu, u32 sem_id, u64 timeout);
s32 sys_semaphore_trywait(u32 sem_id);
s32 sys_semaphore_post(u32 sem_id, s32 count);
s32 sys_semaphore_get_value(u32 sem_id, vm::ptr<s32> count);
