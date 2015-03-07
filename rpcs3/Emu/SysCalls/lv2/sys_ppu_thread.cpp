#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_ppu_thread.h"

SysCallBase sys_ppu_thread("sys_ppu_thread");

void ppu_thread_exit(PPUThread& CPU, u64 errorcode)
{
	CPU.SetExitStatus(errorcode);
	CPU.Stop();

	if (!CPU.IsJoinable())
	{
		const u32 id = CPU.GetId();
		CallAfter([id]()
		{
			Emu.GetCPU().RemoveThread(id);
		});
	}
}

void sys_ppu_thread_exit(PPUThread& CPU, u64 errorcode)
{
	sys_ppu_thread.Log("sys_ppu_thread_exit(0x%llx)", errorcode);
	
	ppu_thread_exit(CPU, errorcode);
}

void sys_internal_ppu_thread_exit(PPUThread& CPU, u64 errorcode)
{
	sys_ppu_thread.Warning("sys_internal_ppu_thread_exit(0x%llx)", errorcode);

	ppu_thread_exit(CPU, errorcode);
}

s32 sys_ppu_thread_yield()
{
	sys_ppu_thread.Log("sys_ppu_thread_yield()");
	// Note: Or do we actually want to yield?
	std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	return CELL_OK;
}

s32 sys_ppu_thread_join(u64 thread_id, vm::ptr<u64> vptr)
{
	sys_ppu_thread.Warning("sys_ppu_thread_join(thread_id=%lld, vptr_addr=0x%x)", thread_id, vptr.addr());

	std::shared_ptr<CPUThread> thr = Emu.GetCPU().GetThread(thread_id);
	if (!thr) return
		CELL_ESRCH;

	while (thr->IsAlive())
	{
		if (Emu.IsStopped())
		{
			sys_ppu_thread.Warning("sys_ppu_thread_join(%d) aborted", thread_id);
			return CELL_OK;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	}

	*vptr = thr->GetExitStatus();
	Emu.GetCPU().RemoveThread(thread_id);
	return CELL_OK;
}

s32 sys_ppu_thread_detach(u64 thread_id)
{
	sys_ppu_thread.Todo("sys_ppu_thread_detach(thread_id=%lld)", thread_id);

	std::shared_ptr<CPUThread> thr = Emu.GetCPU().GetThread(thread_id);
	if (!thr)
		return CELL_ESRCH;

	if (!thr->IsJoinable())
		return CELL_EINVAL;
	thr->SetJoinable(false);

	return CELL_OK;
}

void sys_ppu_thread_get_join_state(PPUThread& CPU, vm::ptr<s32> isjoinable)
{
	sys_ppu_thread.Warning("sys_ppu_thread_get_join_state(isjoinable_addr=0x%x)", isjoinable.addr());

	*isjoinable = CPU.IsJoinable();
}

s32 sys_ppu_thread_set_priority(u64 thread_id, s32 prio)
{
	sys_ppu_thread.Log("sys_ppu_thread_set_priority(thread_id=%lld, prio=%d)", thread_id, prio);

	std::shared_ptr<CPUThread> thr = Emu.GetCPU().GetThread(thread_id);
	if (!thr)
		return CELL_ESRCH;

	thr->SetPrio(prio);

	return CELL_OK;
}

s32 sys_ppu_thread_get_priority(u64 thread_id, u32 prio_addr)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_priority(thread_id=%lld, prio_addr=0x%x)", thread_id, prio_addr);

	std::shared_ptr<CPUThread> thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	vm::write32(prio_addr, (s32)thr->GetPrio());

	return CELL_OK;
}

s32 sys_ppu_thread_get_stack_information(PPUThread& CPU, u32 info_addr)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_stack_information(info_addr=0x%x)", info_addr);

	vm::write32(info_addr, (u32)CPU.GetStackAddr());
	vm::write32(info_addr + 4, CPU.GetStackSize());

	return CELL_OK;
}

s32 sys_ppu_thread_stop(u64 thread_id)
{
	sys_ppu_thread.Warning("sys_ppu_thread_stop(thread_id=%lld)", thread_id);

	std::shared_ptr<CPUThread> thr = Emu.GetCPU().GetThread(thread_id);
	if (!thr)
		return CELL_ESRCH;

	thr->Stop();

	return CELL_OK;
}

s32 sys_ppu_thread_restart(u64 thread_id)
{
	sys_ppu_thread.Warning("sys_ppu_thread_restart(thread_id=%lld)", thread_id);

	std::shared_ptr<CPUThread> thr = Emu.GetCPU().GetThread(thread_id);
	if (!thr)
		return CELL_ESRCH;

	thr->Stop();
	thr->Run();

	return CELL_OK;
}

u32 ppu_thread_create(u32 entry, u64 arg, s32 prio, u32 stacksize, bool is_joinable, bool is_interrupt, std::string name, std::function<void(PPUThread&)> task)
{
	auto new_thread = Emu.GetCPU().AddThread(CPU_THREAD_PPU);

	auto& ppu = static_cast<PPUThread&>(*new_thread);

	ppu.SetEntry(entry);
	ppu.SetPrio(prio);
	ppu.SetStackSize(stacksize < 0x4000 ? 0x4000 : stacksize); // (hack) adjust minimal stack size
	ppu.SetJoinable(is_joinable);
	ppu.SetName(name);
	ppu.custom_task = task;
	ppu.Run();

	if (!is_interrupt)
	{
		ppu.GPR[3] = arg;
		ppu.Exec();
	}

	return ppu.GetId();
}

s32 sys_ppu_thread_create(vm::ptr<u64> thread_id, u32 entry, u64 arg, s32 prio, u32 stacksize, u64 flags, vm::ptr<const char> threadname)
{
	sys_ppu_thread.Warning("sys_ppu_thread_create(thread_id=*0x%x, entry=0x%x, arg=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname=*0x%x)", thread_id, entry, arg, prio, stacksize, flags, threadname);

	if (prio < 0 || prio > 3071)
	{
		return CELL_EINVAL;
	}

	bool is_joinable = flags & SYS_PPU_THREAD_CREATE_JOINABLE;
	bool is_interrupt = flags & SYS_PPU_THREAD_CREATE_INTERRUPT;

	if (is_joinable && is_interrupt)
	{
		return CELL_EPERM;
	}

	*thread_id = ppu_thread_create(entry, arg, prio, stacksize, is_joinable, is_interrupt, threadname ? threadname.get_ptr() : "");
	return CELL_OK;
}

void sys_ppu_thread_once(PPUThread& CPU, vm::ptr<atomic_t<u32>> once_ctrl, vm::ptr<void()> init)
{
	sys_ppu_thread.Warning("sys_ppu_thread_once(once_ctrl_addr=0x%x, init_addr=0x%x)", once_ctrl.addr(), init.addr());

	LV2_LOCK;

	if (once_ctrl->compare_and_swap_test(be_t<u32>::make(SYS_PPU_THREAD_ONCE_INIT), be_t<u32>::make(SYS_PPU_THREAD_DONE_INIT)))
	{
		init(CPU);
	}
}

s32 sys_ppu_thread_get_id(PPUThread& CPU, vm::ptr<u64> thread_id)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_id(thread_id_addr=0x%x)", thread_id.addr());

	*thread_id = CPU.GetId();
	return CELL_OK;
}

s32 sys_ppu_thread_rename(u64 thread_id, vm::ptr<const char> name)
{
	sys_ppu_thread.Log("sys_ppu_thread_rename(thread_id=0x%llx, name=*0x%x)", thread_id, name);

	std::shared_ptr<CPUThread> t = Emu.GetCPU().GetThread(thread_id, CPU_THREAD_PPU);

	if (!t)
	{
		return CELL_ESRCH;
	}
		
	t->SetThreadName(name.get_ptr());
	return CELL_OK;
}
