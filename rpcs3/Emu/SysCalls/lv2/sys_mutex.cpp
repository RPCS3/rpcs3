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
		sys_mutex.Notice("Mutex(%d) was owned by thread %d (recursive=%d)", id.read_relaxed(), tid, recursive_count.load());
	}

	if (u32 count = queue.count())
	{
		sys_mutex.Notice("Mutex(%d) was waited by %d threads", id.read_relaxed(), count);
	}
}

s32 sys_mutex_create(PPUThread& CPU, vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute> attr)
{
	sys_mutex.Log("sys_mutex_create(mutex_id_addr=0x%x, attr_addr=0x%x)", mutex_id.addr(), attr.addr());

	switch (attr->protocol.data())
	{
	case se32(SYS_SYNC_FIFO): break;
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_mutex.Todo("SYS_SYNC_PRIORITY_INHERIT"); break;
	case se32(SYS_SYNC_RETRY): sys_mutex.Error("Invalid protocol (SYS_SYNC_RETRY)"); return CELL_EINVAL;
	default: sys_mutex.Error("Unknown protocol (0x%x)", attr->protocol); return CELL_EINVAL;
	}

	bool is_recursive;
	switch (attr->recursive.data())
	{
	case se32(SYS_SYNC_RECURSIVE): is_recursive = true; break;
	case se32(SYS_SYNC_NOT_RECURSIVE): is_recursive = false; break;
	default: sys_mutex.Error("Unknown recursive attribute (0x%x)", attr->recursive); return CELL_EINVAL;
	}

	if (attr->pshared.data() != se32(0x200))
	{
		sys_mutex.Error("Unknown pshared attribute (0x%x)", attr->pshared);
		return CELL_EINVAL;
	}

	std::shared_ptr<Mutex> mutex(new Mutex(attr->protocol, is_recursive, attr->name_u64));

	const u32 id = sys_mutex.GetNewId(mutex, TYPE_MUTEX);
	mutex->id.exchange(id);
	*mutex_id = id;
	mutex->queue.set_full_name(fmt::Format("Mutex(%d)", id));

	sys_mutex.Warning("*** mutex created [%s] (protocol=0x%x, recursive=%s): id = %d", std::string(attr->name, 8).c_str(), attr->protocol, is_recursive, id);
	// TODO: unlock mutex when owner thread does exit
	return CELL_OK;
}

s32 sys_mutex_destroy(PPUThread& CPU, u32 mutex_id)
{
	sys_mutex.Warning("sys_mutex_destroy(mutex_id=%d)", mutex_id);

	std::shared_ptr<Mutex> mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	// check if associated condition variable exists
	if (mutex->cond_count) // TODO: check safety
	{
		return CELL_EPERM;
	}

	if (!mutex->owner.compare_and_swap_test(0, ~0)) // check if locked and make unusable
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(mutex_id);
	return CELL_OK;
}

s32 sys_mutex_lock(PPUThread& CPU, u32 mutex_id, u64 timeout)
{
	sys_mutex.Log("sys_mutex_lock(mutex_id=%d, timeout=%lld)", mutex_id, timeout);

	const u64 start_time = get_system_time();

	std::shared_ptr<Mutex> mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	const u32 tid = CPU.GetId();

	const u32 old_owner = mutex->owner.compare_and_swap(0, tid);
	if (!~old_owner)
	{
		return CELL_ESRCH; // mutex is going to be destroyed
	}
	if (old_owner == tid)
	{
		if (mutex->is_recursive)
		{
			if (!~mutex->recursive_count)
			{
				return CELL_EKRESOURCE;
			}
			mutex->recursive_count++;
			return CELL_OK;
		}
		else
		{
			return CELL_EDEADLK;
		}
	}
	else if (!old_owner)
	{
		mutex->recursive_count = 1;
		CPU.owned_mutexes++;
		return CELL_OK;
	}

	mutex->queue.push(tid, mutex->protocol);

	while (true)
	{
		auto old_owner = mutex->owner.compare_and_swap(0, tid);
		if (!old_owner || old_owner == tid)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (timeout && get_system_time() - start_time > timeout)
		{
			if (!mutex->queue.invalidate(tid, mutex->protocol))
			{
				assert(!"sys_mutex_lock() failed (timeout)");
			}
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_mutex.Warning("sys_mutex_lock(id=%d) aborted", mutex_id);
			return CELL_OK;
		}
	}

	if (!mutex->queue.invalidate(tid, mutex->protocol) && !mutex->queue.pop(tid, mutex->protocol))
	{
		assert(!"sys_mutex_lock() failed (locking)");
	}
	mutex->recursive_count = 1;
	CPU.owned_mutexes++;
	return CELL_OK;
}

s32 sys_mutex_trylock(PPUThread& CPU, u32 mutex_id)
{
	sys_mutex.Log("sys_mutex_trylock(mutex_id=%d)", mutex_id);

	std::shared_ptr<Mutex> mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	const u32 tid = CPU.GetId();

	const u32 old_owner = mutex->owner.compare_and_swap(0, tid);
	if (!~old_owner)
	{
		return CELL_ESRCH; // mutex is going to be destroyed
	}
	if (old_owner == tid)
	{
		if (mutex->is_recursive)
		{
			if (!~mutex->recursive_count)
			{
				return CELL_EKRESOURCE;
			}
			mutex->recursive_count++;
			return CELL_OK;
		}
		else
		{
			return CELL_EDEADLK;
		}
	}
	else if (!old_owner)
	{
		mutex->recursive_count = 1;
		CPU.owned_mutexes++;
		return CELL_OK;
	}

	return CELL_EBUSY;
}

s32 sys_mutex_unlock(PPUThread& CPU, u32 mutex_id)
{
	sys_mutex.Log("sys_mutex_unlock(mutex_id=%d)", mutex_id);

	std::shared_ptr<Mutex> mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	const u32 tid = CPU.GetId();

	const u32 owner = mutex->owner.read_sync();
	if (!~owner)
	{
		return CELL_ESRCH; // mutex is going to be destroyed
	}
	if (owner != tid)
	{
		return CELL_EPERM;
	}

	if (!mutex->recursive_count || (mutex->recursive_count != 1 && !mutex->is_recursive))
	{
		sys_mutex.Error("sys_mutex_unlock(%d): wrong recursive value fixed (%d)", mutex_id, mutex->recursive_count.load());
		mutex->recursive_count = 1;
	}

	if (!--mutex->recursive_count)
	{
		if (!mutex->owner.compare_and_swap_test(tid, mutex->queue.signal(mutex->protocol)))
		{
			assert(!"sys_mutex_unlock() failed");
		}
		CPU.owned_mutexes--;
	}
	return CELL_OK;
}
