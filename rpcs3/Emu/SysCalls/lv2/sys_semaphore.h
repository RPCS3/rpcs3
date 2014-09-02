#pragma once

#include "sys_lwmutex.h"

struct sys_semaphore_attribute
{
	be_t<u32> protocol;
	be_t<u32> pshared; // undefined
	be_t<u64> ipc_key; // undefined
	be_t<int> flags; // undefined
	be_t<u32> pad; // not used
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct Semaphore
{
	std::mutex m_mutex;
	SleepQueue m_queue;
	int m_value;
	u32 signal;

	const int max;
	const u32 protocol;
	const u64 name;

	Semaphore(int initial_count, int max_count, u32 protocol, u64 name)
		: m_value(initial_count)
		, signal(0)
		, max(max_count)
		, protocol(protocol)
		, name(name)
	{
	}
};

// SysCalls
s32 sys_semaphore_create(vm::ptr<be_t<u32>> sem, vm::ptr<sys_semaphore_attribute> attr, int initial_count, int max_count);
s32 sys_semaphore_destroy(u32 sem_id);
s32 sys_semaphore_wait(u32 sem_id, u64 timeout);
s32 sys_semaphore_trywait(u32 sem_id);
s32 sys_semaphore_post(u32 sem_id, int count);
s32 sys_semaphore_get_value(u32 sem_id, vm::ptr<be_t<s32>> count);
