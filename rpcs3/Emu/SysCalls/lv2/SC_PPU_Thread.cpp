#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

extern Module sysPrxForUser;

static const u32 PPU_THREAD_ID_INVALID = 0xFFFFFFFFU;
enum
{
	SYS_PPU_THREAD_ONCE_INIT,
	SYS_PPU_THREAD_DONE_INIT,
};

int sys_ppu_thread_exit(int errorcode)
{
	if(errorcode == 0)
	{
		sysPrxForUser.Log("sys_ppu_thread_exit(errorcode=%d)", errorcode);
	}
	else
	{
		sysPrxForUser.Warning("sys_ppu_thread_exit(errorcode=%d)", errorcode);
	}
	
	PPUThread& thr = GetCurrentPPUThread();
	thr.SetExitStatus(errorcode);
	wxGetApp().SendDbgCommand(DID_EXIT_THR_SYSCALL, &thr);

	return CELL_OK;
}

int sys_ppu_thread_yield()
{
	sysPrxForUser.Log("sys_ppu_thread_yield()");	
	return CELL_OK;
}

int sys_ppu_thread_join(u32 thread_id, u32 vptr_addr)
{
	sysPrxForUser.Warning("sys_ppu_thread_join(thread_id=%d, vptr_addr=0x%x)", thread_id, vptr_addr);

	PPCThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	GetCurrentPPUThread().Wait(*thr);
	return CELL_OK;
}

int sys_ppu_thread_detach(u32 thread_id)
{
	sysPrxForUser.Error("sys_ppu_thread_detach(thread_id=%d)", thread_id);

	return CELL_OK;
}

void sys_ppu_thread_get_join_state(u32 isjoinable_addr)
{
	sysPrxForUser.Warning("sys_ppu_thread_get_join_state(isjoinable_addr=0x%x)", isjoinable_addr);
	Memory.Write32(isjoinable_addr, GetCurrentPPUThread().IsJoinable());
}

int sys_ppu_thread_set_priority(u32 thread_id, int prio)
{
	sysPrxForUser.Warning("sys_ppu_thread_set_priority(thread_id=%d, prio=%d)", thread_id, prio);

	PPCThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	thr->SetPrio(prio);

	return CELL_OK;
}

int sys_ppu_thread_get_priority(u32 thread_id, u32 prio_addr)
{
	sysPrxForUser.Log("sys_ppu_thread_get_priority(thread_id=%d, prio_addr=0x%x)", thread_id, prio_addr);

	PPCThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;
	if(!Memory.IsGoodAddr(prio_addr)) return CELL_EFAULT;

	Memory.Write32(prio_addr, thr->GetPrio());

	return CELL_OK;
}

int sys_ppu_thread_get_stack_information(u32 info_addr)
{
	sysPrxForUser.Log("sys_ppu_thread_get_stack_information(info_addr=0x%x)", info_addr);

	if(!Memory.IsGoodAddr(info_addr)) return CELL_EFAULT;

	declCPU();

	Memory.Write32(info_addr,   CPU.GetStackAddr());
	Memory.Write32(info_addr+4, CPU.GetStackSize());

	return CELL_OK;
}

int sys_ppu_thread_stop(u32 thread_id)
{
	sysPrxForUser.Warning("sys_ppu_thread_stop(thread_id=%d)", thread_id);

	PPCThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	thr->Stop();

	return CELL_OK;
}

int sys_ppu_thread_restart(u32 thread_id)
{
	sysPrxForUser.Warning("sys_ppu_thread_restart(thread_id=%d)", thread_id);

	PPCThread* thr = Emu.GetCPU().GetThread(thread_id);
	if(!thr) return CELL_ESRCH;

	thr->Stop();
	thr->Run();

	return CELL_OK;
}

int sys_ppu_thread_create(u32 thread_id_addr, u32 entry, u32 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr)
{
	sysPrxForUser.Log("sys_ppu_thread_create(thread_id_addr=0x%x, entry=0x%x, arg=0x%x, prio=%d, stacksize=0x%x, flags=0x%llx, threadname_addr=0x%x('%s'))",
		thread_id_addr, entry, arg, prio, stacksize, flags, threadname_addr, Memory.ReadString(threadname_addr));

	if(!Memory.IsGoodAddr(entry) || !Memory.IsGoodAddr(thread_id_addr) || !Memory.IsGoodAddr(threadname_addr))
	{
		return CELL_EFAULT;
	}

	PPCThread& new_thread = Emu.GetCPU().AddThread(PPC_THREAD_PPU);

	Memory.Write32(thread_id_addr, new_thread.GetId());
	new_thread.SetEntry(entry);
	new_thread.SetArg(0, arg);
	new_thread.SetPrio(prio);
	new_thread.stack_size = stacksize;
	//new_thread.flags = flags;
	new_thread.SetName(Memory.ReadString(threadname_addr));
	new_thread.Run();
	new_thread.Exec();

	return CELL_OK;
}

void sys_ppu_thread_once(u32 once_ctrl_addr, u32 entry)
{
	sysPrxForUser.Warning("sys_ppu_thread_once(once_ctrl_addr=0x%x, entry=0x%x)", once_ctrl_addr, entry);

	if(Memory.IsGoodAddr(once_ctrl_addr, 4) && Memory.Read32(once_ctrl_addr) == SYS_PPU_THREAD_ONCE_INIT)
	{
		Memory.Write32(once_ctrl_addr, SYS_PPU_THREAD_DONE_INIT);

		PPCThread& new_thread = Emu.GetCPU().AddThread(PPC_THREAD_PPU);
		new_thread.SetEntry(entry);
		((PPUThread&)new_thread).LR = Emu.GetPPUThreadExit();
		new_thread.Run();
		new_thread.Exec();

		GetCurrentPPUThread().Wait(new_thread);
	}
}

int sys_ppu_thread_get_id(const u32 id_addr)
{
	sysPrxForUser.Log("sys_ppu_thread_get_id(id_addr=0x%x)", id_addr);

	Memory.Write32(id_addr, GetCurrentPPUThread().GetId());
	return CELL_OK;
}
