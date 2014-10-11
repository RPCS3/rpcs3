#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_time.h"
#include "sys_semaphore.h"

SysCallBase sys_semaphore("sys_semaphore");

u32 semaphore_create(s32 initial_count, s32 max_count, u32 protocol, u64 name_u64)
{
	LV2_LOCK(0);

	const std::string name((const char*)&name_u64, 8);
	const u32 id = sys_semaphore.GetNewId(new Semaphore(initial_count, max_count, protocol, name_u64), TYPE_SEMAPHORE);
	sys_semaphore.Notice("*** semaphore created [%s] (protocol=0x%x): id = %d", name.c_str(), protocol, id);
	Emu.GetSyncPrimManager().AddSemaphoreData(id, name, initial_count, max_count);
	return id;
}

s32 sys_semaphore_create(vm::ptr<u32> sem, vm::ptr<sys_semaphore_attribute> attr, s32 initial_count, s32 max_count)
{
	sys_semaphore.Warning("sys_semaphore_create(sem_addr=0x%x, attr_addr=0x%x, initial_count=%d, max_count=%d)",
		sem.addr(), attr.addr(), initial_count, max_count);

	if (max_count <= 0 || initial_count > max_count || initial_count < 0)
	{
		sys_semaphore.Error("sys_semaphore_create(): invalid parameters (initial_count=%d, max_count=%d)", initial_count, max_count);
		return CELL_EINVAL;
	}
	
	if (attr->pshared.ToBE() != se32(0x200))
	{
		sys_semaphore.Error("sys_semaphore_create(): invalid pshared value(0x%x)", (u32)attr->pshared);
		return CELL_EINVAL;
	}

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_FIFO): break;
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_semaphore.Todo("SYS_SYNC_PRIORITY_INHERIT"); break;
	case se32(SYS_SYNC_RETRY): sys_semaphore.Error("SYS_SYNC_RETRY"); return CELL_EINVAL;
	default: sys_semaphore.Error("Unknown protocol attribute(0x%x)", (u32)attr->protocol); return CELL_EINVAL;
	}

	*sem = semaphore_create(initial_count, max_count, attr->protocol, attr->name_u64);
	return CELL_OK;
}

s32 sys_semaphore_destroy(u32 sem_id)
{
	sys_semaphore.Warning("sys_semaphore_destroy(sem_id=%d)", sem_id);

	LV2_LOCK(0);

	Semaphore* sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}
	
	if (!sem->m_queue.finalize())
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(sem_id);
	Emu.GetSyncPrimManager().EraseSemaphoreData(sem_id);
	return CELL_OK;
}

s32 sys_semaphore_wait(u32 sem_id, u64 timeout)
{
	sys_semaphore.Log("sys_semaphore_wait(sem_id=%d, timeout=%lld)", sem_id, timeout);

	Semaphore* sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	const u32 tid = GetCurrentPPUThread().GetId();
	const u64 start_time = get_system_time();

	{
		std::lock_guard<std::mutex> lock(sem->m_mutex);
		if (sem->m_value > 0)
		{
			sem->m_value--;
			return CELL_OK;
		}
		sem->m_queue.push(tid);
	}

	while (true)
	{
		if (Emu.IsStopped())
		{
			sys_semaphore.Warning("sys_semaphore_wait(%d) aborted", sem_id);
			return CELL_OK;
		}

		if (timeout && get_system_time() - start_time > timeout)
		{
			sem->m_queue.invalidate(tid);
			return CELL_ETIMEDOUT;
		}

		if (tid == sem->signal)
		{
			std::lock_guard<std::mutex> lock(sem->m_mutex);

			if (tid != sem->signal)
			{
				continue;
			}
			sem->signal = 0;
			// TODO: notify signaler
			return CELL_OK;
		}

		SM_Sleep();
	}
}

s32 sys_semaphore_trywait(u32 sem_id)
{
	sys_semaphore.Log("sys_semaphore_trywait(sem_id=%d)", sem_id);

	Semaphore* sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	std::lock_guard<std::mutex> lock(sem->m_mutex);

	if (sem->m_value > 0)
	{
		sem->m_value--;
		return CELL_OK;
	}
	else
	{
		return CELL_EBUSY;
	}
}

s32 sys_semaphore_post(u32 sem_id, s32 count)
{
	sys_semaphore.Log("sys_semaphore_post(sem_id=%d, count=%d)", sem_id, count);

	Semaphore* sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	if (count < 0)
	{
		return CELL_EINVAL;
	}

	if (count + sem->m_value - (s32)sem->m_queue.count() > sem->max)
	{
		return CELL_EBUSY;
	}

	while (count > 0)
	{
		if (Emu.IsStopped())
		{
			sys_semaphore.Warning("sys_semaphore_post(%d) aborted", sem_id);
			return CELL_OK;
		}

		std::lock_guard<std::mutex> lock(sem->m_mutex);

		if (sem->signal && sem->m_queue.count())
		{
			SM_Sleep();
			continue;
		}

		if (u32 target = (sem->protocol == SYS_SYNC_FIFO) ? sem->m_queue.pop() : sem->m_queue.pop_prio())
		{
			count--;
			sem->signal = target;
			Emu.GetCPU().NotifyThread(target);
		}
		else
		{
			sem->m_value += count;
			count = 0;
		}
	}

	return CELL_OK;
}

s32 sys_semaphore_get_value(u32 sem_id, vm::ptr<s32> count)
{
	sys_semaphore.Log("sys_semaphore_get_value(sem_id=%d, count_addr=0x%x)", sem_id, count.addr());

	Semaphore* sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	std::lock_guard<std::mutex> lock(sem->m_mutex);
	
	*count = sem->m_value;

	return CELL_OK;
}
