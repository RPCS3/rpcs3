#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/SysCalls.h"
#include "sys_ppu_thread.h"

extern Module *sysPrxForUser;

static const u32 PPU_THREAD_ID_INVALID = 0xFFFFFFFFU;

void sys_ppu_thread_exit(u64 errorcode)
{
	sysPrxForUser->Log("sys_ppu_thread_exit(0x%llx)", errorcode);
	
	PPUThread& thr = GetCurrentPPUThread();
	u32 tid = thr.GetId();

	if (thr.owned_mutexes)
	{
		LOG_ERROR(PPU, "Owned mutexes found (%d)", thr.owned_mutexes);
		thr.owned_mutexes = 0;
	}

	thr.SetExitStatus(errorcode);
	thr.Stop();
}

s32 sys_ppu_thread_yield()
{
	sysPrxForUser->Log("sys_ppu_thread_yield()");
	// Note: Or do we actually want to yield?
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return CELL_OK;
}

s32 sys_ppu_thread_join(u64 thread_id, mem64_t vptr)
{
	sysPrxForUser->Warning("sys_ppu_thread_join(thread_id=%lld, vptr_addr=0x%x)", thread_id, vptr.GetAddr());

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	while (thr->IsAlive())
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(PPU, "sys_ppu_thread_join(%d) aborted", thread_id);
			return CELL_OK;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	vptr = thr->GetExitStatus();
	return CELL_OK;
}

s32 sys_ppu_thread_detach(u64 thread_id)
{
	sysPrxForUser->Error("sys_ppu_thread_detach(thread_id=%lld)", thread_id);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	if(!thr->IsJoinable())
		return CELL_EINVAL;
	thr->SetJoinable(false);

	return CELL_OK;
}

void sys_ppu_thread_get_join_state(u32 isjoinable_addr)
{
	sysPrxForUser->Warning("sys_ppu_thread_get_join_state(isjoinable_addr=0x%x)", isjoinable_addr);
	Memory.Write32(isjoinable_addr, GetCurrentPPUThread().IsJoinable());
}

s32 sys_ppu_thread_set_priority(u64 thread_id, s32 prio)
{
	sysPrxForUser->Log("sys_ppu_thread_set_priority(thread_id=%lld, prio=%d)", thread_id, prio);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	thr->SetPrio(prio);

	return CELL_OK;
}

s32 sys_ppu_thread_get_priority(u64 thread_id, u32 prio_addr)
{
	sysPrxForUser->Log("sys_ppu_thread_get_priority(thread_id=%lld, prio_addr=0x%x)", thread_id, prio_addr);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;
	if(!Memory.IsGoodAddr(prio_addr)) return CELL_EFAULT;

	Memory.Write32(prio_addr, thr->GetPrio());

	return CELL_OK;
}

s32 sys_ppu_thread_get_stack_information(u32 info_addr)
{
	sysPrxForUser->Log("sys_ppu_thread_get_stack_information(info_addr=0x%x)", info_addr);

	if(!Memory.IsGoodAddr(info_addr)) return CELL_EFAULT;

	declCPU();

	Memory.Write32(info_addr,   CPU.GetStackAddr());
	Memory.Write32(info_addr+4, CPU.GetStackSize());

	return CELL_OK;
}

s32 sys_ppu_thread_stop(u64 thread_id)
{
	sysPrxForUser->Warning("sys_ppu_thread_stop(thread_id=%lld)", thread_id);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	thr->Stop();

	return CELL_OK;
}

s32 sys_ppu_thread_restart(u64 thread_id)
{
	sysPrxForUser->Warning("sys_ppu_thread_restart(thread_id=%lld)", thread_id);

	CPUThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	thr->Stop();
	thr->Run();

	return CELL_OK;
}

s32 sys_ppu_thread_create(mem64_t thread_id, u32 entry, u64 arg, s32 prio, u32 stacksize, u64 flags, u32 threadname_addr)
{
	std::string threadname = "";
	if (Memory.IsGoodAddr(threadname_addr))
	{
		threadname = Memory.ReadString(threadname_addr);
		sysPrxForUser->Log("sys_ppu_thread_create(thread_id_addr=0x%x, entry=0x%x, arg=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname_addr=0x%x('%s'))",
			thread_id.GetAddr(), entry, arg, prio, stacksize, flags, threadname_addr, threadname.c_str());
	}
	else
	{
		sysPrxForUser->Log("sys_ppu_thread_create(thread_id_addr=0x%x, entry=0x%x, arg=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname_addr=0x%x)",
			thread_id.GetAddr(), entry, arg, prio, stacksize, flags, threadname_addr);
		if (threadname_addr != 0) return CELL_EFAULT;
	}

	if (!Memory.IsGoodAddr(entry) || !thread_id.IsGood())
	{
		return CELL_EFAULT;
	}

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
	default: sysPrxForUser->Error("sys_ppu_thread_create(): unknown flags value (0x%llx)", flags); return CELL_EPERM;
	}

	CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_PPU);

	thread_id = new_thread.GetId();
	new_thread.SetEntry(entry);
	new_thread.SetArg(0, arg);
	new_thread.SetPrio(prio);
	new_thread.SetStackSize(stacksize);
	//new_thread.flags = flags;
	new_thread.m_has_interrupt = false;
	new_thread.m_is_interrupt = is_interrupt;
	new_thread.SetName(threadname);

	LOG_NOTICE(PPU, "*** New PPU Thread [%s] (flags=0x%llx, entry=0x%x): id = %d", new_thread.GetName().c_str(), flags, entry, new_thread.GetId());

	if (!is_interrupt)
	{
		new_thread.Run();
		new_thread.Exec();
	}

	return CELL_OK;
}

void sys_ppu_thread_once(mem_ptr_t<std::atomic<be_t<u32>>> once_ctrl, u32 entry)
{
	sysPrxForUser->Warning("sys_ppu_thread_once(once_ctrl_addr=0x%x, entry=0x%x)", once_ctrl.GetAddr(), entry);

	be_t<u32> old = SYS_PPU_THREAD_ONCE_INIT;
	if (once_ctrl->compare_exchange_weak(old, SYS_PPU_THREAD_DONE_INIT))
	{
		CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_PPU);
		new_thread.SetEntry(entry);
		new_thread.Run();
		new_thread.Exec();

		while (new_thread.IsAlive()) SM_Sleep();
	}
}

s32 sys_ppu_thread_get_id(const u32 id_addr)
{
	sysPrxForUser->Log("sys_ppu_thread_get_id(id_addr=0x%x)", id_addr);

	Memory.Write64(id_addr, GetCurrentPPUThread().GetId());
	return CELL_OK;
}
