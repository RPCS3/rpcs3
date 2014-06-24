#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "sys_semaphore.h"

SysCallBase sys_sem("sys_semaphore");

s32 sys_semaphore_create(mem32_t sem, mem_ptr_t<sys_semaphore_attribute> attr, int initial_count, int max_count)
{
	sys_sem.Warning("sys_semaphore_create(sem_addr=0x%x, attr_addr=0x%x, initial_count=%d, max_count=%d)",
		sem.GetAddr(), attr.GetAddr(), initial_count, max_count);

	if (!sem.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	if (max_count <= 0 || initial_count > max_count || initial_count < 0)
	{
		sys_sem.Error("sys_semaphore_create(): invalid parameters (initial_count=%d, max_count=%d)", initial_count, max_count);
		return CELL_EINVAL;
	}
	
	if (attr->pshared.ToBE() != se32(0x200))
	{
		sys_sem.Error("sys_semaphore_create(): invalid pshared value(0x%x)", (u32)attr->pshared);
		return CELL_EINVAL;
	}

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_FIFO): break;
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_sem.Warning("TODO: SYS_SYNC_PRIORITY_INHERIT protocol"); break;
	case se32(SYS_SYNC_RETRY): sys_sem.Error("Invalid SYS_SYNC_RETRY protocol"); return CELL_EINVAL;
	default: sys_sem.Error("Unknown protocol attribute(0x%x)", (u32)attr->protocol); return CELL_EINVAL;
	}

	sem = sys_sem.GetNewId(new Semaphore(initial_count, max_count, attr->protocol, attr->name_u64));
	LOG_NOTICE(HLE, "*** semaphore created [%s] (protocol=0x%x): id = %d",
		std::string(attr->name, 8).c_str(), (u32)attr->protocol, sem.GetValue());

	return CELL_OK;
}

s32 sys_semaphore_destroy(u32 sem_id)
{
	sys_sem.Warning("sys_semaphore_destroy(sem_id=%d)", sem_id);

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
	return CELL_OK;
}

s32 sys_semaphore_wait(u32 sem_id, u64 timeout)
{
	sys_sem.Log("sys_semaphore_wait(sem_id=%d, timeout=%lld)", sem_id, timeout);

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
			LOG_WARNING(HLE, "sys_semaphore_wait(%d) aborted", sem_id);
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
	sys_sem.Log("sys_semaphore_trywait(sem_id=%d)", sem_id);

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

s32 sys_semaphore_post(u32 sem_id, int count)
{
	sys_sem.Log("sys_semaphore_post(sem_id=%d, count=%d)", sem_id, count);

	Semaphore* sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	if (count < 0)
	{
		return CELL_EINVAL;
	}

	if (count + sem->m_value - (int)sem->m_queue.count() > sem->max)
	{
		return CELL_EBUSY;
	}

	while (count > 0)
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(HLE, "sys_semaphore_post(%d) aborted", sem_id);
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

s32 sys_semaphore_get_value(u32 sem_id, mem32_t count)
{
	sys_sem.Log("sys_semaphore_get_value(sem_id=%d, count_addr=0x%x)", sem_id, count.GetAddr());

	if (!count.IsGood())
	{
		return CELL_EFAULT;
	}

	Semaphore* sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	std::lock_guard<std::mutex> lock(sem->m_mutex);
	
	count = sem->m_value;

	return CELL_OK;
}
