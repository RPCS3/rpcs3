#pragma once

struct sys_cond_attribute
{
	be_t<u32> pshared;
	be_t<u64> ipc_key;
	be_t<int> flags;
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct Cond
{
	Mutex* mutex; // associated with mutex
	SQueue<u32, 32> signal;
	sleep_queue_t queue;

	Cond(Mutex* mutex, u64 name)
		: mutex(mutex)
		, queue(name)
	{
	}
};

class PPUThread;

// SysCalls
s32 sys_cond_create(vm::ptr<u32> cond_id, u32 mutex_id, vm::ptr<sys_cond_attribute> attr);
s32 sys_cond_destroy(u32 cond_id);
s32 sys_cond_wait(PPUThread& CPU, u32 cond_id, u64 timeout);
s32 sys_cond_signal(u32 cond_id);
s32 sys_cond_signal_all(u32 cond_id);
s32 sys_cond_signal_to(u32 cond_id, u32 thread_id);
