#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

struct sys_lwmutex_t;

struct sys_lwcond_attribute_t
{
	union
	{
		nse_t<u64, 1> name_u64;
		char name[sizeof(u64)];
	};
};

struct sys_lwcond_t
{
	vm::bptr<sys_lwmutex_t> lwmutex;
	be_t<u32> lwcond_queue; // lwcond pseudo-id
};

struct lv2_lwcond final : lv2_obj
{
	static const u32 id_base = 0x97000000;

	const be_t<u64> name;
	const u32 lwid;
	const lv2_protocol protocol;
	vm::ptr<sys_lwcond_t> control;

	shared_mutex mutex;
	ppu_thread* sq{};

	atomic_t<s32> lwmutex_waiters = 0;

	lv2_lwcond(u64 name, u32 lwid, u32 protocol, vm::ptr<sys_lwcond_t> control) noexcept
		: name(std::bit_cast<be_t<u64>>(name))
		, lwid(lwid)
		, protocol{static_cast<u8>(protocol)}
		, control(control)
	{
	}

	lv2_lwcond(utils::serial& ar);
	void save(utils::serial& ar);
};

// Aux
class ppu_thread;

// Syscalls

error_code _sys_lwcond_create(ppu_thread& ppu, vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name);
error_code _sys_lwcond_destroy(ppu_thread& ppu, u32 lwcond_id);
error_code _sys_lwcond_signal(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u64 ppu_thread_id, u32 mode);
error_code _sys_lwcond_signal_all(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u32 mode);
error_code _sys_lwcond_queue_wait(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u64 timeout);
