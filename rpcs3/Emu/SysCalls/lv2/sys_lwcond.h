#pragma once

struct sys_lwmutex_t;

struct sys_lwcond_attribute_t
{
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct sys_lwcond_t
{
	vm::bptr<sys_lwmutex_t> lwmutex;
	be_t<u32> lwcond_queue; // lwcond pseudo-id
};

struct lwcond_t
{
	const u64 name;

	std::atomic<u32> signaled1; // mode 1 signals
	std::atomic<u32> signaled2; // mode 2 signals

	// TODO: use sleep queue
	std::condition_variable cv;
	std::atomic<s32> waiters;

	lwcond_t(u64 name)
		: name(name)
		, signaled1(0)
		, signaled2(0)
		, waiters(0)
	{
	}
};

// Aux
void lwcond_create(sys_lwcond_t& lwcond, sys_lwmutex_t& lwmutex, u64 name);

class PPUThread;

// SysCalls
s32 _sys_lwcond_create(vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name, u32 arg5);
s32 _sys_lwcond_destroy(u32 lwcond_id);
s32 _sys_lwcond_signal(u32 lwcond_id, u32 lwmutex_id, u32 ppu_thread_id, u32 mode);
s32 _sys_lwcond_signal_all(u32 lwcond_id, u32 lwmutex_id, u32 mode);
s32 _sys_lwcond_queue_wait(u32 lwcond_id, u32 lwmutex_id, u64 timeout);
