#pragma once

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
	be_t<u32> lwmutex;
	be_t<u32> lwcond_queue;
};

struct Lwcond
{
	SMutex signal;
	SleepQueue m_queue;

	Lwcond(u64 name)
		: m_queue(name)
	{
	}
};

// SysCalls
s32 sys_lwcond_create(mem_ptr_t<sys_lwcond_t> lwcond, mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwcond_attribute_t> attr);
s32 sys_lwcond_destroy(mem_ptr_t<sys_lwcond_t> lwcond);
s32 sys_lwcond_signal(mem_ptr_t<sys_lwcond_t> lwcond);
s32 sys_lwcond_signal_all(mem_ptr_t<sys_lwcond_t> lwcond);
s32 sys_lwcond_signal_to(mem_ptr_t<sys_lwcond_t> lwcond, u32 ppu_thread_id);
s32 sys_lwcond_wait(mem_ptr_t<sys_lwcond_t> lwcond, u64 timeout);
