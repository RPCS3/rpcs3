#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_interrupt.h"

logs::channel sys_interrupt("sys_interrupt", logs::level::notice);

void lv2_int_serv_t::join(PPUThread& ppu, lv2_lock_t lv2_lock)
{
	// Use is_joining to stop interrupt thread and signal
	thread->is_joining = true;
	(*thread)->notify();

	// Start joining
	while (!(thread->state & cpu_state::exit))
	{
		CHECK_EMU_STATUS;

		get_current_thread_cv().wait_for(lv2_lock, 1ms);
	}

	// Cleanup
	idm::remove<lv2_int_serv_t>(id);
	idm::remove<PPUThread>(thread->id);
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
	const auto it = idm::get<PPUThread>(intrthread);

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

	const auto handler = idm::make_ptr<lv2_int_serv_t>(it);

	tag->handler = handler;

	it->custom_task = [handler, arg1, arg2](PPUThread& ppu)
	{
		const u32 pc   = ppu.pc;
		const u32 rtoc = ppu.GPR[2];

		LV2_LOCK;

		while (!ppu.is_joining)
		{
			CHECK_EMU_STATUS;

			// call interrupt handler until int status is clear
			if (handler->signal)
			{
				if (lv2_lock) lv2_lock.unlock();

				ppu.GPR[3] = arg1;
				ppu.GPR[4] = arg2;
				ppu.fast_call(pc, rtoc);

				handler->signal--;
				continue;
			}

			if (!lv2_lock)
			{
				lv2_lock.lock();
				continue;
			}

			get_current_thread_cv().wait(lv2_lock);
		}

		ppu.state += cpu_state::exit;
	};

	it->state -= cpu_state::stop;
	(*it)->lock_notify();

	*ih = handler->id;

	return CELL_OK;
}

s32 _sys_interrupt_thread_disestablish(PPUThread& ppu, u32 ih, vm::ptr<u64> r13)
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
	*r13 = handler->thread->GPR[13];

	return CELL_OK;
}

void sys_interrupt_thread_eoi(PPUThread& ppu)
{
	sys_interrupt.trace("sys_interrupt_thread_eoi()");

	// TODO: maybe it should actually unwind the stack of PPU thread?

	ppu.GPR[1] = align(ppu.stack_addr + ppu.stack_size, 0x200) - 0x200; // supercrutch to bypass stack check

	ppu.state += cpu_state::ret;
}
