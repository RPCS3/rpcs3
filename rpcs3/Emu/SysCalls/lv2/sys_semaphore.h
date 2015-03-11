#pragma once

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

struct semaphore_t
{
	const u32 protocol;
	const s32 max;
	const u64 name;

	std::atomic<s32> value;

	// TODO: use sleep queue, possibly remove condition variable
	std::condition_variable cv;
	std::atomic<u32> waiters;

	semaphore_t(u32 protocol, s32 max, u64 name, s32 value)
		: protocol(protocol)
		, max(max)
		, name(name)
		, value(value)
		, waiters(0)
	{
	}
};

// Aux
u32 semaphore_create(s32 initial_val, s32 max_val, u32 protocol, u64 name_u64);

// SysCalls
s32 sys_semaphore_create(vm::ptr<u32> sem, vm::ptr<sys_semaphore_attribute_t> attr, s32 initial_val, s32 max_val);
s32 sys_semaphore_destroy(u32 sem);
s32 sys_semaphore_wait(u32 sem, u64 timeout);
s32 sys_semaphore_trywait(u32 sem);
s32 sys_semaphore_post(u32 sem, s32 count);
s32 sys_semaphore_get_value(u32 sem, vm::ptr<s32> count);
