#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Lwmutex.h"
#include "SC_Lwcond.h"

SysCallBase sys_lwcond("sys_lwcond");

int sys_lwcond_create(mem_ptr_t<sys_lwcond_t> lwcond, mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwcond_attribute_t> attr)
{
	sys_lwcond.Log("sys_lwcond_create(lwcond_addr=0x%x, lwmutex_addr=0x%x, attr_addr=0x%x)",
		lwcond.GetAddr(), lwmutex.GetAddr(), attr.GetAddr());

	if (!lwcond.IsGood() /*|| !lwmutex.IsGood()*/ || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	lwcond->lwmutex = lwmutex.GetAddr();
	lwcond->lwcond_queue = sys_lwcond.GetNewId(new SleepQueue(attr->name_u64));

	if (lwmutex.IsGood())
	{
		if (lwmutex->attribute.ToBE() & se32(SYS_SYNC_RETRY))
		{
			sys_lwcond.Warning("Unsupported SYS_SYNC_RETRY lwmutex protocol");
		}
		if (lwmutex->attribute.ToBE() & se32(SYS_SYNC_RECURSIVE))
		{
			sys_lwcond.Warning("Recursive lwmutex(sq=%d)", (u32)lwmutex->sleep_queue);
		}
	}
	else
	{
		sys_lwcond.Warning("Invalid lwmutex address(0x%x)", lwmutex.GetAddr());
	}

	sys_lwcond.Warning("*** lwcond created [%s] (lwmutex_addr=0x%x): id = %d", 
		wxString(attr->name, 8).wx_str(), lwmutex.GetAddr(), (u32)lwcond->lwcond_queue);

	return CELL_OK;
}

int sys_lwcond_destroy(mem_ptr_t<sys_lwcond_t> lwcond)
{
	sys_lwcond.Warning("sys_lwcond_destroy(lwcond_addr=0x%x)", lwcond.GetAddr());

	if (!lwcond.IsGood())
	{
		return CELL_EFAULT;
	}

	u32 lwc = lwcond->lwcond_queue;

	SleepQueue* sq;
	if (!Emu.GetIdManager().GetIDData(lwc, sq))
	{
		return CELL_ESRCH;
	}

	if (!sq->finalize())
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(lwc);
	return CELL_OK;
}

int sys_lwcond_signal(mem_ptr_t<sys_lwcond_t> lwcond)
{
	sys_lwcond.Log("sys_lwcond_signal(lwcond_addr=0x%x)", lwcond.GetAddr());

	if (!lwcond.IsGood())
	{
		return CELL_EFAULT;
	}

	SleepQueue* sq;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, sq))
	{
		return CELL_ESRCH;
	}

	mem_ptr_t<sys_lwmutex_t> mutex(lwcond->lwmutex);
	be_t<u32> tid = GetCurrentPPUThread().GetId();

	if (be_t<u32> target = (mutex->attribute.ToBE() == se32(SYS_SYNC_PRIORITY) ? sq->pop_prio() : sq->pop()))
	{
		if (mutex->mutex.owner.trylock(target) != SMR_OK)
		{
			mutex->mutex.owner.lock(tid);
			mutex->recursive_count = 1;
			mutex->mutex.owner.unlock(tid, target);
		}
	}

	if (Emu.IsStopped())
	{
		ConLog.Warning("sys_lwcond_signal(sq=%d) aborted", (u32)lwcond->lwcond_queue);
	}

	return CELL_OK;
}

int sys_lwcond_signal_all(mem_ptr_t<sys_lwcond_t> lwcond)
{
	sys_lwcond.Log("sys_lwcond_signal_all(lwcond_addr=0x%x)", lwcond.GetAddr());

	if (!lwcond.IsGood())
	{
		return CELL_EFAULT;
	}

	SleepQueue* sq;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, sq))
	{
		return CELL_ESRCH;
	}

	mem_ptr_t<sys_lwmutex_t> mutex(lwcond->lwmutex);
	be_t<u32> tid = GetCurrentPPUThread().GetId();

	while (be_t<u32> target = (mutex->attribute.ToBE() == se32(SYS_SYNC_PRIORITY) ? sq->pop_prio() : sq->pop()))
	{
		if (mutex->mutex.owner.trylock(target) != SMR_OK)
		{
			mutex->mutex.owner.lock(tid);
			mutex->recursive_count = 1;
			mutex->mutex.owner.unlock(tid, target);
		}
	}

	if (Emu.IsStopped())
	{
		ConLog.Warning("sys_lwcond_signal_all(sq=%d) aborted", (u32)lwcond->lwcond_queue);
	}

	return CELL_OK;
}

int sys_lwcond_signal_to(mem_ptr_t<sys_lwcond_t> lwcond, u32 ppu_thread_id)
{
	sys_lwcond.Log("sys_lwcond_signal_to(lwcond_addr=0x%x, ppu_thread_id=%d)", lwcond.GetAddr(), ppu_thread_id);

	if (!lwcond.IsGood())
	{
		return CELL_EFAULT;
	}

	SleepQueue* sq;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, sq))
	{
		return CELL_ESRCH;
	}

	if (!sq->invalidate(ppu_thread_id))
	{
		return CELL_EPERM;
	}

	mem_ptr_t<sys_lwmutex_t> mutex(lwcond->lwmutex);
	be_t<u32> tid = GetCurrentPPUThread().GetId();

	be_t<u32> target = ppu_thread_id;

	if (mutex->mutex.owner.trylock(target) != SMR_OK)
	{
		mutex->mutex.owner.lock(tid);
		mutex->recursive_count = 1;
		mutex->mutex.owner.unlock(tid, target);
	}

	if (Emu.IsStopped())
	{
		ConLog.Warning("sys_lwcond_signal_to(sq=%d, to=%d) aborted", (u32)lwcond->lwcond_queue, ppu_thread_id);
	}

	return CELL_OK;
}

int sys_lwcond_wait(mem_ptr_t<sys_lwcond_t> lwcond, u64 timeout)
{
	sys_lwcond.Log("sys_lwcond_wait(lwcond_addr=0x%x, timeout=%lld)", lwcond.GetAddr(), timeout);

	if (!lwcond.IsGood())
	{
		return CELL_EFAULT;
	}

	SleepQueue* sq;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, sq))
	{
		return CELL_ESRCH;
	}

	mem_ptr_t<sys_lwmutex_t> mutex(lwcond->lwmutex);
	u32 tid_le = GetCurrentPPUThread().GetId();
	be_t<u32> tid = tid_le;

	if (mutex->mutex.owner.GetOwner() != tid)
	{
		return CELL_EPERM; // caller must own this lwmutex
	}

	sq->push(tid_le);

	mutex->recursive_count = 0;
	mutex->mutex.owner.unlock(tid);

	u32 counter = 0;
	const u32 max_counter = timeout ? (timeout / 1000) : ~0;
	while (true)
	{
		/* switch (mutex->trylock(tid))
		{
		case SMR_OK: mutex->unlock(tid); break;
		case SMR_SIGNAL: return CELL_OK;
		} */
		if (mutex->mutex.owner.GetOwner() == tid)
		{
			_mm_mfence();
			mutex->recursive_count = 1;
			return CELL_OK;
		}

		Sleep(1);

		if (counter++ > max_counter)
		{
			sq->invalidate(tid_le);
			return CELL_ETIMEDOUT;
		}
		if (Emu.IsStopped())
		{
			ConLog.Warning("sys_lwcond_wait(sq=%d) aborted", (u32)lwcond->lwcond_queue);
			return CELL_OK;
		}
	}
}
