#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_ppu_thread.h"

SysCallBase sys_ppu_thread("sys_ppu_thread");

void _sys_ppu_thread_exit(PPUThread& CPU, u64 errorcode)
{
	sys_ppu_thread.Warning("_sys_ppu_thread_exit(errorcode=0x%llx)", errorcode);

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

void sys_ppu_thread_yield()
{
	sys_ppu_thread.Log("sys_ppu_thread_yield()");
	// Note: Or do we actually want to yield?
	std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
}

s32 sys_ppu_thread_join(u32 thread_id, vm::ptr<u64> vptr)
{
	sys_ppu_thread.Warning("sys_ppu_thread_join(thread_id=0x%x, vptr=*0x%x)", thread_id, vptr);

	const auto t = Emu.GetCPU().GetThread(thread_id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	while (t->IsAlive())
	{
		if (Emu.IsStopped())
		{
			sys_ppu_thread.Warning("sys_ppu_thread_join(%d) aborted", thread_id);
			return CELL_OK;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	}

	*vptr = t->GetExitStatus();
	Emu.GetCPU().RemoveThread(thread_id);
	return CELL_OK;
}

s32 sys_ppu_thread_detach(u32 thread_id)
{
	sys_ppu_thread.Warning("sys_ppu_thread_detach(thread_id=0x%x)", thread_id);

	const auto t = Emu.GetCPU().GetThread(thread_id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	if (!t->IsJoinable())
	{
		return CELL_EINVAL;
	}
		
	t->SetJoinable(false);

	return CELL_OK;
}

void sys_ppu_thread_get_join_state(PPUThread& CPU, vm::ptr<s32> isjoinable)
{
	sys_ppu_thread.Warning("sys_ppu_thread_get_join_state(isjoinable=*0x%x)", isjoinable);

	*isjoinable = CPU.IsJoinable();
}

s32 sys_ppu_thread_set_priority(u32 thread_id, s32 prio)
{
	sys_ppu_thread.Log("sys_ppu_thread_set_priority(thread_id=0x%x, prio=%d)", thread_id, prio);

	const auto t = Emu.GetCPU().GetThread(thread_id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	t->SetPrio(prio);

	return CELL_OK;
}

s32 sys_ppu_thread_get_priority(u32 thread_id, vm::ptr<s32> priop)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_priority(thread_id=0x%x, priop=*0x%x)", thread_id, priop);

	const auto t = Emu.GetCPU().GetThread(thread_id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	*priop = static_cast<s32>(t->GetPrio());

	return CELL_OK;
}

s32 sys_ppu_thread_get_stack_information(PPUThread& CPU, vm::ptr<sys_ppu_thread_stack_t> sp)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_stack_information(sp=*0x%x)", sp);

	sp->pst_addr = CPU.GetStackAddr();
	sp->pst_size = CPU.GetStackSize();

	return CELL_OK;
}

s32 sys_ppu_thread_stop(u32 thread_id)
{
	sys_ppu_thread.Error("sys_ppu_thread_stop(thread_id=0x%x)", thread_id);

	const auto t = Emu.GetCPU().GetThread(thread_id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	t->Stop();

	return CELL_OK;
}

s32 sys_ppu_thread_restart(u32 thread_id)
{
	sys_ppu_thread.Error("sys_ppu_thread_restart(thread_id=0x%x)", thread_id);

	const auto t = Emu.GetCPU().GetThread(thread_id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	t->Stop();
	t->Run();

	return CELL_OK;
}

u32 ppu_thread_create(u32 entry, u64 arg, s32 prio, u32 stacksize, bool is_joinable, bool is_interrupt, std::string name, std::function<void(PPUThread&)> task)
{
	const auto new_thread = Emu.GetCPU().AddThread(CPU_THREAD_PPU);

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

s32 _sys_ppu_thread_create(vm::ptr<u64> thread_id, vm::ptr<ppu_thread_param_t> param, u64 arg, u64 unk, s32 prio, u32 stacksize, u64 flags, vm::ptr<const char> threadname)
{
	sys_ppu_thread.Warning("_sys_ppu_thread_create(thread_id=*0x%x, param=*0x%x, arg=0x%llx, unk=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname=*0x%x)",
		thread_id, param, arg, unk, prio, stacksize, flags, threadname);

	if (prio < 0 || prio > 3071)
	{
		return CELL_EINVAL;
	}

	const bool is_joinable = flags & SYS_PPU_THREAD_CREATE_JOINABLE;
	const bool is_interrupt = flags & SYS_PPU_THREAD_CREATE_INTERRUPT;

	if (is_joinable && is_interrupt)
	{
		return CELL_EPERM;
	}

	const auto new_thread = Emu.GetCPU().AddThread(CPU_THREAD_PPU);

	auto& ppu = static_cast<PPUThread&>(*new_thread);

	ppu.SetEntry(param->entry);
	ppu.SetPrio(prio);
	ppu.SetStackSize(stacksize < 0x4000 ? 0x4000 : stacksize); // (hack) adjust minimal stack size
	ppu.SetJoinable(is_joinable);
	ppu.SetName(threadname.get_ptr());
	ppu.Run();

	ppu.GPR[3] = arg;
	ppu.GPR[4] = unk; // actually unknown

	if (u32 tls = param->tls) // hack
	{
		ppu.GPR[13] = tls;
	}

	*thread_id = ppu.GetId();

	return CELL_OK;
}

s32 sys_ppu_thread_start(u32 thread_id)
{
	sys_ppu_thread.Warning("sys_ppu_thread_start(thread_id=0x%x)", thread_id);

	const auto t = Emu.GetCPU().GetThread(thread_id, CPU_THREAD_PPU);

	if (!t)
	{
		return CELL_ESRCH;
	}

	t->Exec();

	return CELL_OK;
}

s32 sys_ppu_thread_rename(u32 thread_id, vm::ptr<const char> name)
{
	sys_ppu_thread.Error("sys_ppu_thread_rename(thread_id=0x%x, name=*0x%x)", thread_id, name);

	const auto t = Emu.GetCPU().GetThread(thread_id, CPU_THREAD_PPU);

	if (!t)
	{
		return CELL_ESRCH;
	}
		
	t->SetThreadName(name.get_ptr());

	return CELL_OK;
}
