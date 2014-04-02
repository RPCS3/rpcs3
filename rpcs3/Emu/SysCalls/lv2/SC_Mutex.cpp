#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Mutex.h"
#include "Utilities/SMutex.h"

SysCallBase sys_mtx("sys_mutex");

int sys_mutex_create(mem32_t mutex_id, mem_ptr_t<sys_mutex_attribute> attr)
{
	sys_mtx.Log("sys_mutex_create(mutex_id_addr=0x%x, attr_addr=0x%x)", mutex_id.GetAddr(), attr.GetAddr());

	if (!mutex_id.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_FIFO): break;
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_mtx.Warning("TODO: SYS_SYNC_PRIORITY_INHERIT protocol"); break;
	case se32(SYS_SYNC_RETRY): sys_mtx.Error("Invalid SYS_SYNC_RETRY protocol"); return CELL_EINVAL;
	default: sys_mtx.Error("Unknown protocol attribute(0x%x)", (u32)attr->protocol); return CELL_EINVAL;
	}

	bool is_recursive;
	switch (attr->recursive.ToBE())
	{
	case se32(SYS_SYNC_RECURSIVE): is_recursive = true; break;
	case se32(SYS_SYNC_NOT_RECURSIVE): is_recursive = false; break;
	default: sys_mtx.Error("Unknown recursive attribute(0x%x)", (u32)attr->recursive); return CELL_EINVAL;
	}

	if (attr->pshared.ToBE() != se32(0x200))
	{
		sys_mtx.Error("Unknown pshared attribute(0x%x)", (u32)attr->pshared);
		return CELL_EINVAL;
	}

	u32 tid = GetCurrentPPUThread().GetId();
	Mutex* mutex = new Mutex((u32)attr->protocol, is_recursive, attr->name_u64);
	u32 id = sys_mtx.GetNewId(mutex);
	mutex->m_mutex.lock(tid);
	mutex->id = id;
	mutex_id = id;
	mutex->m_mutex.unlock(tid);
	sys_mtx.Warning("*** mutex created [%s] (protocol=0x%x, recursive=%s): id = %d",
		std::string(attr->name, 8).c_str(), (u32) attr->protocol,
		(is_recursive ? "true" : "false"), mutex_id.GetValue());

	// TODO: unlock mutex when owner thread does exit

	return CELL_OK;
}

int sys_mutex_destroy(u32 mutex_id)
{
	sys_mtx.Warning("sys_mutex_destroy(mutex_id=%d)", mutex_id);

	Mutex* mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	if (mutex->cond_count) // check if associated condition variable exists
	{
		return CELL_EPERM;
	}

	u32 tid = GetCurrentPPUThread().GetId();

	if (mutex->m_mutex.trylock(tid)) // check if locked
	{
		return CELL_EBUSY;
	}

	if (!mutex->m_queue.finalize())
	{
		mutex->m_mutex.unlock(tid);
		return CELL_EBUSY;
	}

	mutex->m_mutex.unlock(tid, ~0);
	Emu.GetIdManager().RemoveID(mutex_id);
	return CELL_OK;
}

int sys_mutex_lock(u32 mutex_id, u64 timeout)
{
	sys_mtx.Log("sys_mutex_lock(mutex_id=%d, timeout=0x%llx)", mutex_id, timeout);

	Mutex* mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	PPUThread& t = GetCurrentPPUThread();
	u32 tid = t.GetId();

	if (mutex->m_mutex.unlock(tid, tid) == SMR_OK)
	{
		if (mutex->is_recursive)
		{
			if (++mutex->recursive == 0)
			{
				return CELL_EKRESOURCE;
			}
			return CELL_OK;
		}
		else
		{
			return CELL_EDEADLK;
		}
	}
	else if (u32 owner = mutex->m_mutex.GetOwner())
	{
		if (CPUThread* tt = Emu.GetCPU().GetThread(owner))
		{
		}
		else
		{
			sys_mtx.Error("sys_mutex_lock(%d): deadlock on invalid thread(%d)", mutex_id, owner);
		}
	}

	switch (mutex->m_mutex.trylock(tid))
	{
	case SMR_OK: mutex->recursive = 1; t.owned_mutexes++; return CELL_OK;
	case SMR_FAILED: break;
	default: goto abort;
	}

	mutex->m_queue.push(tid);

	switch (mutex->m_mutex.lock(tid, timeout ? ((timeout < 1000) ? 1 : (timeout / 1000)) : 0))
	{
	case SMR_OK:
		mutex->m_queue.invalidate(tid);
	case SMR_SIGNAL:
		mutex->recursive = 1; t.owned_mutexes++; return CELL_OK;
	case SMR_TIMEOUT:
		mutex->m_queue.invalidate(tid); return CELL_ETIMEDOUT;
	default:
		mutex->m_queue.invalidate(tid); goto abort;
	}

abort:
	if (Emu.IsStopped())
	{
		sys_mtx.Warning("sys_mutex_lock(id=%d) aborted", mutex_id);
		return CELL_OK;
	}
	return CELL_ESRCH;
}

int sys_mutex_trylock(u32 mutex_id)
{
	sys_mtx.Log("sys_mutex_trylock(mutex_id=%d)", mutex_id);

	Mutex* mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	PPUThread& t = GetCurrentPPUThread();
	u32 tid = t.GetId();

	if (mutex->m_mutex.unlock(tid, tid) == SMR_OK)
	{
		if (mutex->is_recursive)
		{
			if (++mutex->recursive == 0)
			{
				return CELL_EKRESOURCE;
			}
			return CELL_OK;
		}
		else
		{
			return CELL_EDEADLK;
		}
	}
	else if (u32 owner = mutex->m_mutex.GetOwner())
	{
		if (CPUThread* tt = Emu.GetCPU().GetThread(owner))
		{
		}
		else
		{
			sys_mtx.Error("sys_mutex_trylock(%d): deadlock on invalid thread(%d)", mutex_id, owner);
		}
	}

	switch (mutex->m_mutex.trylock(tid))
	{
	case SMR_OK: mutex->recursive = 1; t.owned_mutexes++; return CELL_OK;
	}

	return CELL_EBUSY;
}

int sys_mutex_unlock(u32 mutex_id)
{
	sys_mtx.Log("sys_mutex_unlock(mutex_id=%d)", mutex_id);

	Mutex* mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	PPUThread& t = GetCurrentPPUThread();
	u32 tid = t.GetId();

	if (mutex->m_mutex.unlock(tid, tid) == SMR_OK)
	{
		if (!mutex->recursive || (mutex->recursive != 1 && !mutex->is_recursive))
		{
			sys_mtx.Error("sys_mutex_unlock(%d): wrong recursive value fixed (%d)", mutex_id, mutex->recursive);
			mutex->recursive = 1;
		}
		mutex->recursive--;
		if (!mutex->recursive)
		{
			mutex->m_mutex.unlock(tid, mutex->protocol == SYS_SYNC_PRIORITY ? mutex->m_queue.pop_prio() : mutex->m_queue.pop());
			t.owned_mutexes--;
		}
		return CELL_OK;
	}

	return CELL_EPERM;
}
