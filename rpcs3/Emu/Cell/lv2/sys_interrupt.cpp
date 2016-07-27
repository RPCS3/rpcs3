#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUOpcodes.h"
#include "sys_interrupt.h"

logs::channel sys_interrupt("sys_interrupt", logs::level::notice);

void lv2_int_serv_t::exec()
{
	thread->cmd_list
	({
		{ ppu_cmd::set_args, 2 }, arg1, arg2,
		{ ppu_cmd::lle_call, 2 },
	});

	thread->lock_notify();
}

void lv2_int_serv_t::join(ppu_thread& ppu, lv2_lock_t lv2_lock)
{
	// Enqueue _sys_ppu_thread_exit call
	thread->cmd_list
	({
		{ ppu_cmd::set_args, 1 }, u64{0},
		{ ppu_cmd::set_gpr, 11 }, u64{41},
		{ ppu_cmd::opcode, ppu_instructions::SC(0) },
	});

	thread->lock_notify();

	// Join thread (TODO)
	while (!(thread->state & cpu_state::exit))
	{
		CHECK_EMU_STATUS;

		get_current_thread_cv().wait_for(lv2_lock, 1ms);
	}

	// Cleanup
	idm::remove<lv2_int_serv_t>(id);
}

s32 sys_interrupt_tag_destroy(u32 intrtag)
{
	sys_interrupt.warning("sys_interrupt_tag_destroy(intrtag=0x%x)", intrtag);

	LV2_LOCK;

	const auto tag = idm::get<lv2_int_tag_t>(intrtag);

	if (!tag)
	{
		return CELL_ESRCH;
	}

	if (tag->handler)
	{
		return CELL_EBUSY;
	}

	idm::remove<lv2_int_tag_t>(intrtag);

	return CELL_OK;
}

s32 _sys_interrupt_thread_establish(vm::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg1, u64 arg2)
{
	sys_interrupt.warning("_sys_interrupt_thread_establish(ih=*0x%x, intrtag=0x%x, intrthread=0x%x, arg1=0x%llx, arg2=0x%llx)", ih, intrtag, intrthread, arg1, arg2);

	LV2_LOCK;

	// Get interrupt tag
	const auto tag = idm::get<lv2_int_tag_t>(intrtag);

	if (!tag)
	{
		return CELL_ESRCH;
	}

	// Get interrupt thread
	const auto it = idm::get<ppu_thread>(intrthread);

	if (!it)
	{
		return CELL_ESRCH;
	}

	// If interrupt thread is running, it's already established on another interrupt tag
	if (!(it->state & cpu_state::stop))
	{
		return CELL_EAGAIN;
	}

	// It's unclear if multiple handlers can be established on single interrupt tag
	if (tag->handler)
	{
		sys_interrupt.error("_sys_interrupt_thread_establish(): handler service already exists (intrtag=0x%x) -> CELL_ESTAT", intrtag);
		return CELL_ESTAT;
	}

	tag->handler = idm::make_ptr<lv2_int_serv_t>(it, arg1, arg2);

	it->run();

	*ih = tag->handler->id;

	return CELL_OK;
}

s32 _sys_interrupt_thread_disestablish(ppu_thread& ppu, u32 ih, vm::ptr<u64> r13)
{
	sys_interrupt.warning("_sys_interrupt_thread_disestablish(ih=0x%x, r13=*0x%x)", ih, r13);

	LV2_LOCK;

	const auto handler = idm::get<lv2_int_serv_t>(ih);

	if (!handler)
	{
		return CELL_ESRCH;
	}

	// Wait for sys_interrupt_thread_eoi() and destroy interrupt thread
	handler->join(ppu, lv2_lock);

	// Save TLS base
	*r13 = handler->thread->gpr[13];

	return CELL_OK;
}

void sys_interrupt_thread_eoi(ppu_thread& ppu) // Low-level PPU function example
{
	// Low-level function body must guard all C++-ish calls and all objects with non-trivial destructors
	thread_guard{ppu}, sys_interrupt.trace("sys_interrupt_thread_eoi()");

	ppu.state += cpu_state::ret;

	// Throw if this syscall was not called directly by the SC instruction (hack)
	if (ppu.lr == 0 || ppu.gpr[11] != 88)
	{
		// Low-level function must disable interrupts before throwing (not related to sys_interrupt_*, it's rather coincidence)
		ppu->interrupt_disable();
		throw cpu_state::ret;
	}
}
