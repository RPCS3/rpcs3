#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/lv2/SC_Condition.h"

SysCallBase sys_cond("sys_cond");

int sys_cond_create(mem32_t cond_id, u32 mutex_id, mem_ptr_t<sys_cond_attribute> attr)
{
	sys_cond.Log("sys_cond_create(cond_id_addr=0x%x, mutex_id=%d, attr_addr=0x%x)",
		cond_id.GetAddr(), mutex_id, attr.GetAddr());

	if (!cond_id.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	if (attr->pshared.ToBE() != se32(0x200))
	{
		sys_cond.Error("Invalid pshared attribute(0x%x)", (u32)attr->pshared);
		return CELL_EINVAL;
	}

	Mutex* mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	if (mutex->is_recursive)
	{
		sys_cond.Warning("*** condition on recursive mutex(%d)", mutex_id);
	}

	Cond* cond = new Cond(mutex, attr->name_u64);
	u32 id = sys_cond.GetNewId(cond);
	cond_id = id;
	mutex->cond_count++;
	sys_cond.Warning("*** condition created [%s] (mutex_id=%d): id = %d", wxString(attr->name, 8).wx_str(), mutex_id, cond_id.GetValue());

	return CELL_OK;
}

int sys_cond_destroy(u32 cond_id)
{
	sys_cond.Warning("sys_cond_destroy(cond_id=%d)", cond_id);

	Cond* cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	if (!cond->m_queue.finalize())
	{
		return CELL_EBUSY;
	}

	cond->mutex->cond_count--;
	Emu.GetIdManager().RemoveID(cond_id);
	return CELL_OK;
}

int sys_cond_signal(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal(cond_id=%d)", cond_id);

	Cond* cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	Mutex* mutex = cond->mutex;
	u32 tid = GetCurrentPPUThread().GetId();

	bool was_locked = (mutex->m_mutex.GetOwner() == tid);

	if (u32 target = (mutex->protocol == SYS_SYNC_PRIORITY ? cond->m_queue.pop_prio() : cond->m_queue.pop()))
	{
		CPUThread* tt = Emu.GetCPU().GetThread(target);
		bool valid = tt && tt->IsAlive();
		if (!valid)
		{
			sys_cond.Error("sys_cond_signal(%d): signal to invalid thread(%d)", cond_id, target);
			return CELL_OK;
		}

		if (!was_locked) // mutex hasn't been locked (don't care about mutex state)
		{
			if (u32 owner = mutex->m_mutex.GetOwner())
			{
				tt = Emu.GetCPU().GetThread(owner);
				valid = tt && tt->IsAlive();
				if (!valid)
				{
					sys_cond.Error("sys_cond_signal(%d): deadlock on invalid thread(%d)", cond_id, owner);
					return CELL_OK;
				}
			}
			mutex->m_mutex.lock(tid);
			mutex->recursive = 1;
			mutex->m_mutex.unlock(tid, target);
		}
		else // mutex has been locked (should preserve original mutex state)
		{
			mutex->recursive = 1;
			mutex->m_mutex.unlock(tid, target);
			mutex->m_mutex.lock(tid);
			mutex->recursive = 1;
		}
	}

	if (Emu.IsStopped())
	{
		ConLog.Warning("sys_cond_signal(id=%d) aborted", cond_id);
	}

	return CELL_OK;
}

int sys_cond_signal_all(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal_all(cond_id=%d)", cond_id);

	Cond* cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	Mutex* mutex = cond->mutex;
	u32 tid = GetCurrentPPUThread().GetId();

	bool was_locked = (mutex->m_mutex.GetOwner() == tid);

	while (u32 target = (mutex->protocol == SYS_SYNC_PRIORITY ? cond->m_queue.pop_prio() : cond->m_queue.pop()))
	{
		CPUThread* tt = Emu.GetCPU().GetThread(target);
		bool valid = tt && tt->IsAlive();
		if (!valid)
		{
			sys_cond.Error("sys_cond_signal_all(%d): signal to invalid thread(%d)", cond_id, target);
			return CELL_OK;
		}

		if (!was_locked)
		{
			if (u32 owner = mutex->m_mutex.GetOwner())
			{
				tt = Emu.GetCPU().GetThread(owner);
				valid = tt && tt->IsAlive();
				if (!valid)
				{
					sys_cond.Error("sys_cond_signal_all(%d): deadlock on invalid thread(%d)", cond_id, owner);
					return CELL_OK;
				}
			}
			mutex->m_mutex.lock(tid);
			mutex->recursive = 1;
			mutex->m_mutex.unlock(tid, target);
		}
		else
		{
			mutex->recursive = 1;
			mutex->m_mutex.unlock(tid, target);
			mutex->m_mutex.lock(tid);
			mutex->recursive = 1;
		}

		if (Emu.IsStopped())
		{
			ConLog.Warning("sys_cond_signal_all(id=%d) aborted", cond_id);
		}
	}

	return CELL_OK;
}

int sys_cond_signal_to(u32 cond_id, u32 thread_id)
{
	sys_cond.Log("sys_cond_signal_to(cond_id=%d, thread_id=%d)", cond_id, thread_id);

	Cond* cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	if (!Emu.GetIdManager().CheckID(thread_id))
	{
		return CELL_ESRCH;
	}

	if (!cond->m_queue.invalidate(thread_id))
	{
		return CELL_EPERM;
	}

	Mutex* mutex = cond->mutex;
	u32 tid = GetCurrentPPUThread().GetId();

	bool was_locked = (mutex->m_mutex.GetOwner() == tid);

	u32 target = thread_id;
	{
		if (!was_locked)
		{
			if (u32 owner = mutex->m_mutex.GetOwner())
			{
				CPUThread* tt = Emu.GetCPU().GetThread(owner);
				bool valid = tt && tt->IsAlive();
				if (!valid)
				{
					sys_cond.Error("sys_cond_signal_to(%d): deadlock on invalid thread(%d)", cond_id, owner);
					return CELL_OK;
				}
			}
			mutex->m_mutex.lock(tid);
			mutex->recursive = 1;
			mutex->m_mutex.unlock(tid, target);
		}
		else
		{
			mutex->recursive = 1;
			mutex->m_mutex.unlock(tid, target);
			mutex->m_mutex.lock(tid);
			mutex->recursive = 1;
		}
	}

	if (Emu.IsStopped())
	{
		ConLog.Warning("sys_cond_signal_to(id=%d, to=%d) aborted", cond_id, thread_id);
	}

	return CELL_OK;
}

int sys_cond_wait(u32 cond_id, u64 timeout)
{
	sys_cond.Log("sys_cond_wait(cond_id=%d, timeout=%lld)", cond_id, timeout);

	Cond* cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	Mutex* mutex = cond->mutex;
	u32 tid = GetCurrentPPUThread().GetId();

	if (mutex->m_mutex.GetOwner() != tid)
	{
		return CELL_EPERM;
	}

	cond->m_queue.push(tid);

	mutex->recursive = 0;
	mutex->m_mutex.unlock(tid);

	u32 counter = 0;
	const u32 max_counter = timeout ? (timeout / 1000) : ~0;

	while (true)
	{
		/* switch (mutex->m_mutex.trylock(tid))
		{
		case SMR_OK: mutex->m_mutex.unlock(tid); break;
		case SMR_SIGNAL: mutex->recursive = 1; return CELL_OK;
		} */
		if (mutex->m_mutex.GetOwner() == tid)
		{
			_mm_mfence();
			mutex->recursive = 1;
			return CELL_OK;
		}

		Sleep(1);

		if (counter++ > max_counter)
		{
			cond->m_queue.invalidate(tid);
			return CELL_ETIMEDOUT;
		}
		if (Emu.IsStopped())
		{
			ConLog.Warning("sys_cond_wait(id=%d) aborted", cond_id);
			return CELL_OK;
		}
	}
}