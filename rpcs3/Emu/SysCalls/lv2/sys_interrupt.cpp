#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_interrupt.h"

SysCallBase sys_interrupt("sys_interrupt");

lv2_int_tag_t::lv2_int_tag_t()
	: id(Emu.GetIdManager().get_current_id())
{
}

lv2_int_serv_t::lv2_int_serv_t(const std::shared_ptr<PPUThread>& thread)
	: thread(thread)
	, id(Emu.GetIdManager().get_current_id())
{
}

void lv2_int_serv_t::join(PPUThread& ppu, lv2_lock_t& lv2_lock)
{
	CHECK_LV2_LOCK(lv2_lock);

	// Use is_joining to stop interrupt thread and signal
	thread->is_joining = true;

	thread->cv.notify_one();

	// Start joining
	while (thread->is_alive())
	{
		CHECK_EMU_STATUS;

		ppu.cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	// Cleanup
	Emu.GetIdManager().remove<lv2_int_serv_t>(id);
	Emu.GetIdManager().remove<PPUThread>(thread->get_id());
}

s32 sys_interrupt_tag_destroy(u32 intrtag)
{
	sys_interrupt.Warning("sys_interrupt_tag_destroy(intrtag=0x%x)", intrtag);

	LV2_LOCK;

	const auto tag = Emu.GetIdManager().get<lv2_int_tag_t>(intrtag);

	if (!tag)
	{
		return CELL_ESRCH;
	}

	if (tag->handler)
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().remove<lv2_int_tag_t>(intrtag);

	return CELL_OK;
}

s32 _sys_interrupt_thread_establish(vm::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg1, u64 arg2)
{
	sys_interrupt.Warning("_sys_interrupt_thread_establish(ih=*0x%x, intrtag=0x%x, intrthread=0x%x, arg1=0x%llx, arg2=0x%llx)", ih, intrtag, intrthread, arg1, arg2);

	LV2_LOCK;

	// Get interrupt tag
	const auto tag = Emu.GetIdManager().get<lv2_int_tag_t>(intrtag);

	if (!tag)
	{
		return CELL_ESRCH;
	}

	// Get interrupt thread
	const auto it = Emu.GetIdManager().get<PPUThread>(intrthread);

	if (!it)
	{
		return CELL_ESRCH;
	}

	// If interrupt thread is running, it's already established on another interrupt tag
	if (!it->is_stopped())
	{
		return CELL_EAGAIN;
	}

	// It's unclear if multiple handlers can be established on single interrupt tag
	if (tag->handler)
	{
		sys_interrupt.Error("_sys_interrupt_thread_establish(): handler service already exists (intrtag=0x%x) -> CELL_ESTAT", intrtag);
		return CELL_ESTAT;
	}

	const auto handler = Emu.GetIdManager().make_ptr<lv2_int_serv_t>(it);

	tag->handler = handler;

	it->custom_task = [handler, arg1, arg2](PPUThread& ppu)
	{
		const u32 pc   = ppu.PC;
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

			ppu.cv.wait(lv2_lock);
		}

		ppu.exit();
	};

	it->exec();

	*ih = handler->id;

	return CELL_OK;
}

s32 _sys_interrupt_thread_disestablish(PPUThread& ppu, u32 ih, vm::ptr<u64> r13)
{
	sys_interrupt.Warning("_sys_interrupt_thread_disestablish(ih=0x%x, r13=*0x%x)", ih, r13);

	LV2_LOCK;

	const auto handler = Emu.GetIdManager().get<lv2_int_serv_t>(ih);

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
	sys_interrupt.Log("sys_interrupt_thread_eoi()");

	// TODO: maybe it should actually unwind the stack of PPU thread?

	ppu.GPR[1] = align(ppu.stack_addr + ppu.stack_size, 0x200) - 0x200; // supercrutch to bypass stack check

	ppu.fast_stop();
}
