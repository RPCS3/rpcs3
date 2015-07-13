#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "sys_interrupt.h"

SysCallBase sys_interrupt("sys_interrupt");

s32 sys_interrupt_tag_destroy(u32 intrtag)
{
	sys_interrupt.Warning("sys_interrupt_tag_destroy(intrtag=0x%x)", intrtag);

	const u32 class_id = intrtag >> 8;

	if (class_id != 0 && class_id != 2)
	{
		return CELL_ESRCH;
	}

	const auto t = Emu.GetCPU().GetRawSPUThread(intrtag & 0xff);

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
	sys_interrupt.Warning("sys_interrupt_thread_establish(ih=*0x%x, intrtag=0x%x, intrthread=%lld, arg=0x%llx)", ih, intrtag, intrthread, arg);

	const u32 class_id = intrtag >> 8;

	if (class_id != 0 && class_id != 2)
	{
		return CELL_ESRCH;
	}

	const auto t = Emu.GetCPU().GetRawSPUThread(intrtag & 0xff);

	if (!t)
	{
		return CELL_ESRCH;
	}

	RawSPUThread& spu = static_cast<RawSPUThread&>(*t);

	auto& tag = class_id ? spu.int2 : spu.int0;

	// CELL_ESTAT is not returned (can't detect exact condition)

	const auto it = Emu.GetCPU().GetThread((u32)intrthread);

	if (!it)
	{
		return CELL_ESRCH;
	}

	PPUThread& ppu = static_cast<PPUThread&>(*it);

	{
		LV2_LOCK;

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
			const auto func = vm::ptr<void(u64 arg)>::make(CPU.entry);
			const auto pc   = vm::read32(func.addr());
			const auto rtoc = vm::read32(func.addr() + 4);

			std::unique_lock<std::mutex> cond_lock(tag.handler_mutex);

			while (!Emu.IsStopped())
			{
				// call interrupt handler until int status is clear
				if (tag.stat.read_relaxed())
				{
					//func(CPU, arg);
					CPU.GPR[3] = arg;
					CPU.FastCall2(pc, rtoc);
				}

				tag.cond.wait_for(cond_lock, std::chrono::milliseconds(1));
			}
		};
	}

	*ih = Emu.GetIdManager().make<lv2_int_handler_t>(it);
	ppu.Exec();

	return CELL_OK;
}

s32 _sys_interrupt_thread_disestablish(u32 ih, vm::ptr<u64> r13)
{
	sys_interrupt.Todo("_sys_interrupt_thread_disestablish(ih=0x%x, r13=*0x%x)", ih, r13);

	const auto handler = Emu.GetIdManager().get<lv2_int_handler_t>(ih);

	if (!handler)
	{
		return CELL_ESRCH;
	}

	PPUThread& ppu = static_cast<PPUThread&>(*handler->handler);

	// TODO: wait for sys_interrupt_thread_eoi() and destroy interrupt thread

	*r13 = ppu.GPR[13];

	return CELL_OK;
}

void sys_interrupt_thread_eoi(PPUThread& CPU)
{
	sys_interrupt.Log("sys_interrupt_thread_eoi()");

	// TODO: maybe it should actually unwind the stack (ensure that all the automatic objects are finalized)?
	CPU.GPR[1] = align(CPU.GetStackAddr() + CPU.GetStackSize(), 0x200) - 0x200; // supercrutch (just to hide error messages)

	CPU.FastStop();
}
