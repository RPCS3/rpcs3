#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_ppu_thread.h"

static SysCallBase sys_ppu_thread("sys_ppu_thread");

static const u32 PPU_THREAD_ID_INVALID = 0xFFFFFFFFU/*UUUUUUUUUUuuuuuuuuuu~~~~~~~~*/;

void ppu_thread_exit(u64 errorcode)
{
	PPUThread& thr = GetCurrentPPUThread();
	u32 tid = thr.GetId();

	if (thr.owned_mutexes)
	{
		sys_ppu_thread.Error("Owned mutexes found (%d)", thr.owned_mutexes);
		thr.owned_mutexes = 0;
	}

	thr.SetExitStatus(errorcode);
	thr.Stop();
}

void sys_ppu_thread_exit(u64 errorcode)
{
	sys_ppu_thread.Log("sys_ppu_thread_exit(0x%llx)", errorcode);
	
	ppu_thread_exit(errorcode);
}

void sys_internal_ppu_thread_exit(u64 errorcode)
{
	sys_ppu_thread.Log("sys_internal_ppu_thread_exit(0x%llx)", errorcode);

	ppu_thread_exit(errorcode);
}

s32 sys_ppu_thread_yield()
{
	sys_ppu_thread.Log("sys_ppu_thread_yield()");
	// Note: Or do we actually want to yield?
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return CELL_OK;
}

s32 sys_ppu_thread_join(u64 thread_id, vm::ptr<be_t<u64>> vptr)
{
	sys_ppu_thread.Warning("sys_ppu_thread_join(thread_id=%lld, vptr_addr=0x%x)", thread_id, vptr.addr());

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	while (thr->IsAlive())
	{
		if (Emu.IsStopped())
		{
			sys_ppu_thread.Warning("sys_ppu_thread_join(%d) aborted", thread_id);
			return CELL_OK;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	*vptr = thr->GetExitStatus();
	return CELL_OK;
}

s32 sys_ppu_thread_detach(u64 thread_id)
{
	sys_ppu_thread.Todo("sys_ppu_thread_detach(thread_id=%lld)", thread_id);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	if(!thr->IsJoinable())
		return CELL_EINVAL;
	thr->SetJoinable(false);

	return CELL_OK;
}

void sys_ppu_thread_get_join_state(u32 isjoinable_addr)
{
	sys_ppu_thread.Warning("sys_ppu_thread_get_join_state(isjoinable_addr=0x%x)", isjoinable_addr);
	vm::write32(isjoinable_addr, GetCurrentPPUThread().IsJoinable());
}

s32 sys_ppu_thread_set_priority(u64 thread_id, s32 prio)
{
	sys_ppu_thread.Log("sys_ppu_thread_set_priority(thread_id=%lld, prio=%d)", thread_id, prio);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	thr->SetPrio(prio);

	return CELL_OK;
}

s32 sys_ppu_thread_get_priority(u64 thread_id, u32 prio_addr)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_priority(thread_id=%lld, prio_addr=0x%x)", thread_id, prio_addr);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	vm::write32(prio_addr, (s32)thr->GetPrio());

	return CELL_OK;
}

s32 sys_ppu_thread_get_stack_information(u32 info_addr)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_stack_information(info_addr=0x%x)", info_addr);

	declCPU();

	vm::write32(info_addr, (u32)CPU.GetStackAddr());
	vm::write32(info_addr + 4, CPU.GetStackSize());

	return CELL_OK;
}

s32 sys_ppu_thread_stop(u64 thread_id)
{
	sys_ppu_thread.Warning("sys_ppu_thread_stop(thread_id=%lld)", thread_id);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	thr->Stop();

	return CELL_OK;
}

s32 sys_ppu_thread_restart(u64 thread_id)
{
	sys_ppu_thread.Warning("sys_ppu_thread_restart(thread_id=%lld)", thread_id);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	thr->Stop();
	thr->Run();

	return CELL_OK;
}

s32 sys_ppu_thread_create(vm::ptr<be_t<u64>> thread_id, u32 entry, u64 arg, s32 prio, u32 stacksize, u64 flags, vm::ptr<const char> threadname)
{
	sys_ppu_thread.Log("sys_ppu_thread_create(thread_id_addr=0x%x, entry=0x%x, arg=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname_addr=0x%x('%s'))",
		thread_id.addr(), entry, arg, prio, stacksize, flags, threadname.addr(), threadname ? threadname.get_ptr() : "");

	bool is_joinable = false;
	bool is_interrupt = false;

	switch (flags)
	{
	case 0: break;
	case SYS_PPU_THREAD_CREATE_JOINABLE:
	{
		is_joinable = true;
		break;
	}
	case SYS_PPU_THREAD_CREATE_INTERRUPT:
	{
		is_interrupt = true;
		break;
	}
	default: sys_ppu_thread.Error("sys_ppu_thread_create(): unknown flags value (0x%llx)", flags); return CELL_EPERM;
	}

	PPUThread& new_thread = *(PPUThread*)&Emu.GetCPU().AddThread(CPU_THREAD_PPU);

	*thread_id = new_thread.GetId();
	new_thread.SetEntry(entry);
	new_thread.SetPrio(prio);
	new_thread.SetStackSize(stacksize);
	//new_thread.flags = flags;
	new_thread.m_has_interrupt = false;
	new_thread.m_is_interrupt = is_interrupt;
	new_thread.SetName(threadname ? threadname.get_ptr() : "");

	sys_ppu_thread.Notice("*** New PPU Thread [%s] (flags=0x%llx, entry=0x%x): id = %d", new_thread.GetName().c_str(), flags, entry, new_thread.GetId());

	if (!is_interrupt)
	{
		new_thread.Run();
		new_thread.GPR[3] = arg;
		new_thread.Exec();
	}
	else
	{
		new_thread.InitStack();
		new_thread.InitRegs();
		new_thread.DoRun();
	}

	return CELL_OK;
}

void sys_ppu_thread_once(vm::ptr<std::atomic<be_t<u32>>> once_ctrl, u32 entry)
{
	sys_ppu_thread.Warning("sys_ppu_thread_once(once_ctrl_addr=0x%x, entry=0x%x)", once_ctrl.addr(), entry);

	be_t<u32> old = be_t<u32>::MakeFromBE(se32(SYS_PPU_THREAD_ONCE_INIT));
	if (once_ctrl->compare_exchange_weak(old, be_t<u32>::MakeFromBE(se32(SYS_PPU_THREAD_DONE_INIT))))
	{
		GetCurrentPPUThread().FastCall2(vm::read32(entry), vm::read32(entry + 4));
	}
}

s32 sys_ppu_thread_get_id(vm::ptr<be_t<u64>> thread_id)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_id(thread_id_addr=0x%x)", thread_id.addr());

	*thread_id = GetCurrentPPUThread().GetId();
	return CELL_OK;
}

s32 sys_ppu_thread_rename(u64 thread_id, vm::ptr<const char> name)
{
	sys_ppu_thread.Log("sys_ppu_thread_rename(thread_id=%d, name_addr=0x%x('%s'))", thread_id, name.addr(), name.get_ptr());

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if (!thr) {
		return CELL_ESRCH;
	}

	thr->SetThreadName(name.get_ptr());
	return CELL_OK;
}
