#pragma once

#include "sleep_queue.h"

namespace vm { using namespace ps3; }

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

struct lv2_lwcond_t
{
	const u64 name;

	sleep_queue_t sq;

	lv2_lwcond_t(u64 name)
		: name(name)
	{
	}

	void notify(lv2_lock_t& lv2_lock, sleep_queue_t::value_type& thread, const std::shared_ptr<lv2_lwmutex_t>& mutex, bool mode2);
};

// Aux
class PPUThread;

// SysCalls
s32 _sys_lwcond_create(vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name, u32 arg5);
s32 _sys_lwcond_destroy(u32 lwcond_id);
s32 _sys_lwcond_signal(u32 lwcond_id, u32 lwmutex_id, u32 ppu_thread_id, u32 mode);
s32 _sys_lwcond_signal_all(u32 lwcond_id, u32 lwmutex_id, u32 mode);
s32 _sys_lwcond_queue_wait(PPUThread& ppu, u32 lwcond_id, u32 lwmutex_id, u64 timeout);
