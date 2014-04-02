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
	lwcond->lwcond_queue = sys_lwcond.GetNewId(new Lwcond(attr->name_u64));

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
		std::string(attr->name, 8).c_str(), lwmutex.GetAddr(), (u32) lwcond->lwcond_queue);

	return CELL_OK;
}

int sys_lwcond_destroy(mem_ptr_t<sys_lwcond_t> lwcond)
{
	sys_lwcond.Warning("sys_lwcond_destroy(lwcond_addr=0x%x)", lwcond.GetAddr());

	if (!lwcond.IsGood())
	{
		return CELL_EFAULT;
	}

	u32 id = lwcond->lwcond_queue;

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData(id, lw))
	{
		return CELL_ESRCH;
	}

	if (!lw->m_queue.finalize())
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(id);
	return CELL_OK;
}

int sys_lwcond_signal(mem_ptr_t<sys_lwcond_t> lwcond)
{
	sys_lwcond.Log("sys_lwcond_signal(lwcond_addr=0x%x)", lwcond.GetAddr());

	if (!lwcond.IsGood())
	{
		return CELL_EFAULT;
	}

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	mem_ptr_t<sys_lwmutex_t> mutex(lwcond->lwmutex);

	if (u32 target = (mutex->attribute.ToBE() == se32(SYS_SYNC_PRIORITY) ? lw->m_queue.pop_prio() : lw->m_queue.pop()))
	{
		lw->signal.lock(target);

		if (Emu.IsStopped())
		{
			ConLog.Warning("sys_lwcond_signal(id=%d) aborted", (u32)lwcond->lwcond_queue);
			return CELL_OK;
		}
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

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	mem_ptr_t<sys_lwmutex_t> mutex(lwcond->lwmutex);

	while (u32 target = (mutex->attribute.ToBE() == se32(SYS_SYNC_PRIORITY) ? lw->m_queue.pop_prio() : lw->m_queue.pop()))
	{
		lw->signal.lock(target);

		if (Emu.IsStopped())
		{
			ConLog.Warning("sys_lwcond_signal_all(id=%d) aborted", (u32)lwcond->lwcond_queue);
			return CELL_OK;
		}
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

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	if (!Emu.GetIdManager().CheckID(ppu_thread_id))
	{
		return CELL_ESRCH;
	}

	if (!lw->m_queue.invalidate(ppu_thread_id))
	{
		return CELL_EPERM;
	}

	u32 target = ppu_thread_id;
	{
		lw->signal.lock(target);

		if (Emu.IsStopped())
		{
			ConLog.Warning("sys_lwcond_signal_to(id=%d, to=%d) aborted", (u32)lwcond->lwcond_queue, ppu_thread_id);
			return CELL_OK;
		}
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

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	mem_ptr_t<sys_lwmutex_t> mutex(lwcond->lwmutex);
	u32 tid_le = GetCurrentPPUThread().GetId();
	be_t<u32> tid = tid_le;

	SleepQueue* sq = nullptr;
	Emu.GetIdManager().GetIDData((u32)mutex->sleep_queue, sq);

	if (mutex->mutex.GetOwner() != tid)
	{
		sys_lwcond.Warning("sys_lwcond_wait(id=%d) failed (EPERM)", (u32)lwcond->lwcond_queue);
		return CELL_EPERM; // caller must own this lwmutex
	}

	lw->m_queue.push(tid_le);

	if (mutex->recursive_count.ToBE() != se32(1))
	{
		sys_lwcond.Warning("sys_lwcond_wait(id=%d): associated mutex had wrong recursive value (%d)",
			(u32)lwcond->lwcond_queue, (u32)mutex->recursive_count);
	}
	mutex->recursive_count = 0;

	if (sq)
	{
		mutex->mutex.unlock(tid, mutex->attribute.ToBE() == se32(SYS_SYNC_PRIORITY) ? sq->pop_prio() : sq->pop());
	}
	else if (mutex->attribute.ToBE() == se32(SYS_SYNC_RETRY))
	{
		mutex->mutex.unlock(tid); // SYS_SYNC_RETRY
	}
	else
	{
		sys_lwcond.Warning("sys_lwcond_wait(id=%d): associated mutex had invalid sleep queue (%d)",
			(u32)lwcond->lwcond_queue, (u32)mutex->sleep_queue);
	}

	u32 counter = 0;
	const u32 max_counter = timeout ? (timeout / 1000) : ~0;

	while (true)
	{
		if (lw->signal.unlock(tid, tid) == SMR_OK)
		{
			switch (mutex->lock(tid, 0))
			{
			case CELL_OK: break;
			case CELL_EDEADLK: sys_lwcond.Warning("sys_lwcond_wait(id=%d): associated mutex was locked",
								   (u32)lwcond->lwcond_queue); return CELL_OK;
			case CELL_ESRCH: sys_lwcond.Warning("sys_lwcond_wait(id=%d): associated mutex not found (%d)",
								 (u32)lwcond->lwcond_queue, (u32)mutex->sleep_queue); return CELL_ESRCH;
			case CELL_EINVAL: goto abort;
			}

			mutex->recursive_count = 1;
			lw->signal.unlock(tid);
			return CELL_OK;
		}

		Sleep(1);

		if (counter++ > max_counter)
		{
			lw->m_queue.invalidate(tid_le);
			return CELL_ETIMEDOUT;
		}
		if (Emu.IsStopped())
		{
			goto abort;
		}
	}

abort:
	ConLog.Warning("sys_lwcond_wait(id=%d) aborted", (u32)lwcond->lwcond_queue);
	return CELL_OK;
}
