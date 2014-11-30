#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_cond.h"

SysCallBase sys_cond("sys_cond");

s32 sys_cond_create(vm::ptr<u32> cond_id, u32 mutex_id, vm::ptr<sys_cond_attribute> attr)
{
	sys_cond.Log("sys_cond_create(cond_id_addr=0x%x, mutex_id=%d, attr_addr=0x%x)",
		cond_id.addr(), mutex_id, attr.addr());

	LV2_LOCK(0);

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
	u32 id = sys_cond.GetNewId(cond, TYPE_COND);
	*cond_id = id;
	mutex->cond_count++;
	sys_cond.Warning("*** condition created [%s] (mutex_id=%d): id = %d", std::string(attr->name, 8).c_str(), mutex_id, id);
	Emu.GetSyncPrimManager().AddSyncPrimData(TYPE_COND, id, std::string(attr->name, 8));

	return CELL_OK;
}

s32 sys_cond_destroy(u32 cond_id)
{
	sys_cond.Warning("sys_cond_destroy(cond_id=%d)", cond_id);

	LV2_LOCK(0);

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
	Emu.GetSyncPrimManager().EraseSyncPrimData(TYPE_COND, cond_id);
	return CELL_OK;
}

s32 sys_cond_signal(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal(cond_id=%d)", cond_id);

	Cond* cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	Mutex* mutex = cond->mutex;

	if (u32 target = (mutex->protocol == SYS_SYNC_PRIORITY ? cond->m_queue.pop_prio() : cond->m_queue.pop()))
	{
		cond->signal.lock(target);

		if (Emu.IsStopped())
		{
			sys_cond.Warning("sys_cond_signal(id=%d) aborted", cond_id);
		}
	}

	return CELL_OK;
}

s32 sys_cond_signal_all(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal_all(cond_id=%d)", cond_id);

	Cond* cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	Mutex* mutex = cond->mutex;

	while (u32 target = (mutex->protocol == SYS_SYNC_PRIORITY ? cond->m_queue.pop_prio() : cond->m_queue.pop()))
	{
		cond->signaler = GetCurrentPPUThread().GetId();
		cond->signal.lock(target);

		if (Emu.IsStopped())
		{
			sys_cond.Warning("sys_cond_signal_all(id=%d) aborted", cond_id);
			break;
		}
	}

	cond->signaler = 0;
	return CELL_OK;
}

s32 sys_cond_signal_to(u32 cond_id, u32 thread_id)
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

	u32 target = thread_id;
	{
		cond->signal.lock(target);
	}

	if (Emu.IsStopped())
	{
		sys_cond.Warning("sys_cond_signal_to(id=%d, to=%d) aborted", cond_id, thread_id);
	}

	return CELL_OK;
}

s32 sys_cond_wait(u32 cond_id, u64 timeout)
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
		sys_cond.Warning("sys_cond_wait(cond_id=%d) failed (EPERM)", cond_id);
		return CELL_EPERM;
	}

	cond->m_queue.push(tid);

	if (mutex->recursive != 1)
	{
		sys_cond.Warning("sys_cond_wait(cond_id=%d): associated mutex had wrong recursive value (%d)", cond_id, mutex->recursive);
	}
	mutex->recursive = 0;
	mutex->m_mutex.unlock(tid, mutex->protocol == SYS_SYNC_PRIORITY ? mutex->m_queue.pop_prio() : mutex->m_queue.pop());

	u64 counter = 0;
	const u64 max_counter = timeout ? (timeout / 1000) : ~0ull;

	while (true)
	{
		if (cond->signal.unlock(tid, tid) == SMR_OK)
		{
			if (SMutexResult res = mutex->m_mutex.trylock(tid))
			{
				if (res != SMR_FAILED)
				{
					goto abort;
				}
				mutex->m_queue.push(tid);

				switch (mutex->m_mutex.lock(tid))
				{
				case SMR_OK:
					mutex->m_queue.invalidate(tid);
				case SMR_SIGNAL:
					break;
				default:
					goto abort;
				}
			}
			mutex->recursive = 1;
			cond->signal.unlock(tid);
			return CELL_OK;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (counter++ > max_counter)
		{
			cond->m_queue.invalidate(tid);
			GetCurrentPPUThread().owned_mutexes--; // ???
			return CELL_ETIMEDOUT;
		}
		if (Emu.IsStopped())
		{
			goto abort;
		}
	}

abort:
	sys_cond.Warning("sys_cond_wait(id=%d) aborted", cond_id);
	return CELL_OK;
}
