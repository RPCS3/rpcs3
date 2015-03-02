#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "sys_interrupt.h"

SysCallBase sys_interrupt("sys_interrupt");

s32 sys_interrupt_tag_destroy(u32 intrtag)
{
	sys_interrupt.Warning("sys_interrupt_tag_destroy(intrtag=%d)", intrtag);

	const u32 class_id = intrtag >> 8;

	if (class_id != 0 && class_id != 2)
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<CPUThread> t = Emu.GetCPU().GetRawSPUThread(intrtag & 0xff);

	if (!t)
	{
		return CELL_ESRCH;
	}

	RawSPUThread& spu = static_cast<RawSPUThread&>(*t);

	auto& tag = class_id ? spu.int2 : spu.int0;

	if (s32 old = tag.assigned.compare_and_swap(0, -1))
	{
		if (old > 0)
		{
			return CELL_EBUSY;
		}

		return CELL_ESRCH;
	}

	return CELL_OK;
}

s32 sys_interrupt_thread_establish(vm::ptr<u32> ih, u32 intrtag, u64 intrthread, u64 arg)
{
	sys_interrupt.Warning("sys_interrupt_thread_establish(ih_addr=0x%x, intrtag=%d, intrthread=%lld, arg=0x%llx)", ih.addr(), intrtag, intrthread, arg);

	const u32 class_id = intrtag >> 8;

	if (class_id != 0 && class_id != 2)
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<CPUThread> t = Emu.GetCPU().GetRawSPUThread(intrtag & 0xff);

	if (!t)
	{
		return CELL_ESRCH;
	}

	RawSPUThread& spu = static_cast<RawSPUThread&>(*t);

	auto& tag = class_id ? spu.int2 : spu.int0;

	// CELL_ESTAT is never returned (can't detect exact condition)

	std::shared_ptr<CPUThread> it = Emu.GetCPU().GetThread((u32)intrthread);

	if (!it)
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<interrupt_handler_t> handler(new interrupt_handler_t{ it });

	PPUThread& ppu = static_cast<PPUThread&>(*it);

	{
		LV2_LOCK(0);

		if (ppu.custom_task)
		{
			return CELL_EAGAIN;
		}

		if (s32 res = tag.assigned.atomic_op<s32>(CELL_OK, [](s32& value) -> s32
		{
			if (value < 0)
			{
				return CELL_ESRCH;
			}

			value++;
			return CELL_OK;
		}))
		{
			return res;
		}

		ppu.custom_task = [t, &tag, arg](PPUThread& CPU)
		{
			auto func = vm::ptr<void(u64 arg)>::make(CPU.entry);

			std::unique_lock<std::mutex> cond_lock(tag.handler_mutex);

			while (!Emu.IsStopped())
			{
				if (tag.stat.read_relaxed())
				{
					func(CPU, arg); // call interrupt handler until int status is clear
				}

				tag.cond.wait_for(cond_lock, std::chrono::milliseconds(1));
			}
		};
	}

	*ih = sys_interrupt.GetNewId(handler, TYPE_INTR_SERVICE_HANDLE);
	ppu.Exec();

	return CELL_OK;
}

s32 _sys_interrupt_thread_disestablish(u32 ih, vm::ptr<u64> r13)
{
	sys_interrupt.Todo("_sys_interrupt_thread_disestablish(ih=%d)", ih);

	std::shared_ptr<interrupt_handler_t> handler;
	if (!sys_interrupt.CheckId(ih, handler))
	{
		return CELL_ESRCH;
	}

	PPUThread& ppu = static_cast<PPUThread&>(*handler->handler);

	// TODO: wait for sys_interrupt_thread_eoi() and destroy interrupt thread

	*r13 = ppu.GPR[13];

	return CELL_OK;
}

void sys_interrupt_thread_eoi()
{
	sys_interrupt.Log("sys_interrupt_thread_eoi()");

	GetCurrentPPUThread().FastStop();
}
