#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/Memory/atomic_type.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sleep_queue_type.h"
#include "sys_time.h"
#include "sys_mutex.h"

SysCallBase sys_mutex("sys_mutex");

Mutex::~Mutex()
{
	if (u32 tid = owner.read_sync())
	{
		sys_mutex.Notice("Mutex(%d) was owned by thread %d (recursive=%d)", id, tid, recursive);
	}

	if (!queue.m_mutex.try_lock()) return;

	for (u32 i = 0; i < queue.list.size(); i++)
	{
		if (u32 owner = queue.list[i])
		{
			sys_mutex.Notice("Mutex(%d) was waited by thread %d", id, owner);
		}
	}

	queue.m_mutex.unlock();
}

s32 sys_mutex_create(PPUThread& CPU, vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute> attr)
{
	sys_mutex.Log("sys_mutex_create(mutex_id_addr=0x%x, attr_addr=0x%x)", mutex_id.addr(), attr.addr());

	LV2_LOCK(0);

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_FIFO): break;
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_mutex.Todo("sys_mutex_create(): SYS_SYNC_PRIORITY_INHERIT"); break;
	case se32(SYS_SYNC_RETRY): sys_mutex.Error("sys_mutex_create(): SYS_SYNC_RETRY"); return CELL_EINVAL;
	default: sys_mutex.Error("Unknown protocol attribute(0x%x)", (u32)attr->protocol); return CELL_EINVAL;
	}

	bool is_recursive;
	switch (attr->recursive.ToBE())
	{
	case se32(SYS_SYNC_RECURSIVE): is_recursive = true; break;
	case se32(SYS_SYNC_NOT_RECURSIVE): is_recursive = false; break;
	default: sys_mutex.Error("Unknown recursive attribute(0x%x)", (u32)attr->recursive); return CELL_EINVAL;
	}

	if (attr->pshared.ToBE() != se32(0x200))
	{
		sys_mutex.Error("Unknown pshared attribute(0x%x)", (u32)attr->pshared);
		return CELL_EINVAL;
	}

	Mutex* mutex = new Mutex((u32)attr->protocol, is_recursive, attr->name_u64);
	const u32 id = sys_mutex.GetNewId(mutex, TYPE_MUTEX);
	mutex->id.exchange(id);
	*mutex_id = id;
	sys_mutex.Warning("*** mutex created [%s] (protocol=0x%x, recursive=%s): id = %d",
		std::string(attr->name, 8).c_str(), (u32) attr->protocol, (is_recursive ? "true" : "false"), id);

	Emu.GetSyncPrimManager().AddSyncPrimData(TYPE_MUTEX, id, std::string(attr->name, 8));
	// TODO: unlock mutex when owner thread does exit

	return CELL_OK;
}

s32 sys_mutex_destroy(PPUThread& CPU, u32 mutex_id)
{
	sys_mutex.Warning("sys_mutex_destroy(mutex_id=%d)", mutex_id);

	LV2_LOCK(0);

	Mutex* mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	if (mutex->cond_count) // check if associated condition variable exists
	{
		return CELL_EPERM;
	}

	const u32 tid = CPU.GetId();

	if (mutex->owner.compare_and_swap_test(0, tid)) // check if locked
	{
		return CELL_EBUSY;
	}

	if (!mutex->queue.finalize())
	{
		if (!mutex->owner.compare_and_swap_test(tid, 0))
		{
			assert(!"sys_mutex_destroy() failed (busy)");
		}
		return CELL_EBUSY;
	}

	if (!mutex->owner.compare_and_swap_test(tid, ~0))
	{
		assert(!"sys_mutex_destroy() failed");
	}
	Emu.GetIdManager().RemoveID(mutex_id);
	Emu.GetSyncPrimManager().EraseSyncPrimData(TYPE_MUTEX, mutex_id);
	return CELL_OK;
}

s32 sys_mutex_lock(PPUThread& CPU, u32 mutex_id, u64 timeout)
{
	sys_mutex.Log("sys_mutex_lock(mutex_id=%d, timeout=%lld)", mutex_id, timeout);

	Mutex* mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	const u32 tid = CPU.GetId();

	if (mutex->owner.read_sync() == tid)
	{
		if (mutex->is_recursive)
		{
			if (!++mutex->recursive)
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

	if (mutex->owner.compare_and_swap_test(0, tid))
	{
		mutex->recursive = 1;
		CPU.owned_mutexes++;
		return CELL_OK;
	}

	mutex->queue.push(tid, mutex->protocol);

	const u64 time_start = get_system_time();
	while (true)
	{
		auto old_owner = mutex->owner.compare_and_swap(0, tid);
		if (!old_owner)
		{
			mutex->queue.invalidate(tid);
			break;
		}
		if (old_owner == tid)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (timeout && get_system_time() - time_start > timeout)
		{
			mutex->queue.invalidate(tid);
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_mutex.Warning("sys_mutex_lock(id=%d) aborted", mutex_id);
			return CELL_OK;
		}
	}

	mutex->recursive = 1;
	CPU.owned_mutexes++;
	return CELL_OK;
}

s32 sys_mutex_trylock(PPUThread& CPU, u32 mutex_id)
{
	sys_mutex.Log("sys_mutex_trylock(mutex_id=%d)", mutex_id);

	Mutex* mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	const u32 tid = CPU.GetId();

	if (mutex->owner.read_sync() == tid)
	{
		if (mutex->is_recursive)
		{
			if (!++mutex->recursive)
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

	if (!mutex->owner.compare_and_swap_test(0, tid))
	{
		return CELL_EBUSY;
	}

	mutex->recursive = 1;
	CPU.owned_mutexes++;
	return CELL_OK;
}

s32 sys_mutex_unlock(PPUThread& CPU, u32 mutex_id)
{
	sys_mutex.Log("sys_mutex_unlock(mutex_id=%d)", mutex_id);

	Mutex* mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	const u32 tid = CPU.GetId();

	if (mutex->owner.read_sync() != tid)
	{
		return CELL_EPERM;
	}

	if (!mutex->recursive || (mutex->recursive != 1 && !mutex->is_recursive))
	{
		sys_mutex.Error("sys_mutex_unlock(%d): wrong recursive value fixed (%d)", mutex_id, mutex->recursive);
		mutex->recursive = 1;
	}

	if (!--mutex->recursive)
	{
		if (!mutex->owner.compare_and_swap_test(tid, mutex->queue.pop(mutex->protocol)))
		{
			assert(!"sys_mutex_unlock() failed");
		}
		CPU.owned_mutexes--;
	}
	return CELL_OK;
}
