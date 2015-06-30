#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sleep_queue.h"
#include "sys_time.h"
#include "sys_mutex.h"

SysCallBase sys_mutex("sys_mutex");

s32 sys_mutex_create(vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute_t> attr)
{
	sys_mutex.Warning("sys_mutex_create(mutex_id=*0x%x, attr=*0x%x)", mutex_id, attr);

	if (!mutex_id || !attr)
	{
		return CELL_EFAULT;
	}

	const u32 protocol = attr->protocol;

	switch (protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_PRIORITY: break;
	case SYS_SYNC_PRIORITY_INHERIT: break;
	default: sys_mutex.Error("sys_mutex_create(): unknown protocol (0x%x)", protocol); return CELL_EINVAL;
	}

	const bool recursive = attr->recursive == SYS_SYNC_RECURSIVE;

	if ((!recursive && attr->recursive != SYS_SYNC_NOT_RECURSIVE) || attr->pshared != SYS_SYNC_NOT_PROCESS_SHARED || attr->adaptive != SYS_SYNC_NOT_ADAPTIVE || attr->ipc_key.data() || attr->flags.data())
	{
		sys_mutex.Error("sys_mutex_create(): unknown attributes (recursive=0x%x, pshared=0x%x, adaptive=0x%x, ipc_key=0x%llx, flags=0x%x)",
			attr->recursive, attr->pshared, attr->adaptive, attr->ipc_key, attr->flags);

		return CELL_EINVAL;
	}

	*mutex_id = Emu.GetIdManager().make<lv2_mutex_t>(recursive, protocol, attr->name_u64);

	return CELL_OK;
}

s32 sys_mutex_destroy(u32 mutex_id)
{
	sys_mutex.Warning("sys_mutex_destroy(mutex_id=0x%x)", mutex_id);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (!mutex->owner.expired())
	{
		return CELL_EBUSY;
	}

	// assuming that the mutex is locked immediately by another waiting thread when unlocked
	if (mutex->waiters)
	{
		return CELL_EBUSY;
	}

	if (mutex->cond_count)
	{
		return CELL_EPERM;
	}

	Emu.GetIdManager().remove<lv2_mutex_t>(mutex_id);

	return CELL_OK;
}

s32 sys_mutex_lock(PPUThread& CPU, u32 mutex_id, u64 timeout)
{
	sys_mutex.Log("sys_mutex_lock(mutex_id=0x%x, timeout=0x%llx)", mutex_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	const auto thread = Emu.GetIdManager().get<PPUThread>(CPU.GetId());

	if (!mutex->owner.owner_before(thread) && !thread.owner_before(mutex->owner)) // check equality
	{
		if (mutex->recursive)
		{
			if (mutex->recursive_count == 0xffffffffu)
			{
				return CELL_EKRESOURCE;
			}

			mutex->recursive_count++;

			return CELL_OK;
		}

		return CELL_EDEADLK;
	}

	// protocol is ignored in current implementation
	mutex->waiters++;

	while (!mutex->owner.expired())
	{
		if (timeout && get_system_time() - start_time > timeout)
		{
			mutex->waiters--;
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_mutex.Warning("sys_mutex_lock(mutex_id=0x%x) aborted", mutex_id);
			return CELL_OK;
		}

		mutex->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	mutex->owner = thread;
	mutex->waiters--;

	return CELL_OK;
}

s32 sys_mutex_trylock(PPUThread& CPU, u32 mutex_id)
{
	sys_mutex.Log("sys_mutex_trylock(mutex_id=0x%x)", mutex_id);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	const auto thread = Emu.GetIdManager().get<PPUThread>(CPU.GetId());

	if (!mutex->owner.owner_before(thread) && !thread.owner_before(mutex->owner)) // check equality
	{
		if (mutex->recursive)
		{
			if (mutex->recursive_count == 0xffffffffu)
			{
				return CELL_EKRESOURCE;
			}

			mutex->recursive_count++;

			return CELL_OK;
		}

		return CELL_EDEADLK;
	}

	if (!mutex->owner.expired())
	{
		return CELL_EBUSY;
	}

	mutex->owner = thread;

	return CELL_OK;
}

s32 sys_mutex_unlock(PPUThread& CPU, u32 mutex_id)
{
	sys_mutex.Log("sys_mutex_unlock(mutex_id=0x%x)", mutex_id);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	const auto thread = Emu.GetIdManager().get<PPUThread>(CPU.GetId());

	if (mutex->owner.owner_before(thread) || thread.owner_before(mutex->owner)) // check inequality
	{
		return CELL_EPERM;
	}

	if (mutex->recursive_count)
	{
		if (!mutex->recursive)
		{
			throw __FUNCTION__;
		}
		
		mutex->recursive_count--;
	}
	else
	{
		mutex->owner.reset();

		if (mutex->waiters)
		{
			mutex->cv.notify_one();
		}
	}

	return CELL_OK;
}
