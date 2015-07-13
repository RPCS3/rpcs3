#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sleep_queue.h"
#include "sys_time.h"
#include "sys_rwlock.h"

SysCallBase sys_rwlock("sys_rwlock");

s32 sys_rwlock_create(vm::ptr<u32> rw_lock_id, vm::ptr<sys_rwlock_attribute_t> attr)
{
	sys_rwlock.Warning("sys_rwlock_create(rw_lock_id=*0x%x, attr=*0x%x)", rw_lock_id, attr);

	if (!rw_lock_id || !attr)
	{
		return CELL_EFAULT;
	}

	const u32 protocol = attr->protocol;

	switch (protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_PRIORITY: break;
	case SYS_SYNC_PRIORITY_INHERIT: break;
	default: sys_rwlock.Error("sys_rwlock_create(): unknown protocol (0x%x)", protocol); return CELL_EINVAL;
	}

	if (attr->pshared.data() != se32(0x200) || attr->ipc_key.data() || attr->flags.data())
	{
		sys_rwlock.Error("sys_rwlock_create(): unknown attributes (pshared=0x%x, ipc_key=0x%llx, flags=0x%x)", attr->pshared, attr->ipc_key, attr->flags);
		return CELL_EINVAL;
	}

	*rw_lock_id = Emu.GetIdManager().make<lv2_rwlock_t>(attr->protocol, attr->name_u64);

	return CELL_OK;
}

s32 sys_rwlock_destroy(u32 rw_lock_id)
{
	sys_rwlock.Warning("sys_rwlock_destroy(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = Emu.GetIdManager().get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->readers || rwlock->writer || rwlock->rwaiters || rwlock->wwaiters)
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().remove<lv2_rwlock_t>(rw_lock_id);

	return CELL_OK;
}

s32 sys_rwlock_rlock(u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.Log("sys_rwlock_rlock(rw_lock_id=0x%x, timeout=0x%llx)", rw_lock_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto rwlock = Emu.GetIdManager().get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	// waiting threads are not properly registered in current implementation
	rwlock->rwaiters++;

	while (rwlock->writer || rwlock->wwaiters)
	{
		if (timeout && get_system_time() - start_time > timeout)
		{
			rwlock->rwaiters--;
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_rwlock.Warning("sys_rwlock_rlock(rw_lock_id=0x%x) aborted", rw_lock_id);
			return CELL_OK;
		}

		rwlock->rcv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	rwlock->readers++;
	rwlock->rwaiters--;

	return CELL_OK;
}

s32 sys_rwlock_tryrlock(u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_tryrlock(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = Emu.GetIdManager().get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->writer || rwlock->wwaiters)
	{
		return CELL_EBUSY;
	}

	rwlock->readers++;

	return CELL_OK;
}

s32 sys_rwlock_runlock(u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_runlock(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = Emu.GetIdManager().get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (!rwlock->readers)
	{
		return CELL_EPERM;
	}

	if (!--rwlock->readers && rwlock->wwaiters)
	{
		rwlock->wcv.notify_one();
	}

	return CELL_OK;
}

s32 sys_rwlock_wlock(PPUThread& CPU, u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.Log("sys_rwlock_wlock(rw_lock_id=0x%x, timeout=0x%llx)", rw_lock_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto rwlock = Emu.GetIdManager().get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->writer == CPU.GetId())
	{
		return CELL_EDEADLK;
	}

	// protocol is ignored in current implementation
	rwlock->wwaiters++;

	while (rwlock->readers || rwlock->writer)
	{
		if (timeout && get_system_time() - start_time > timeout)
		{
			rwlock->wwaiters--;
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_rwlock.Warning("sys_rwlock_wlock(rw_lock_id=0x%x) aborted", rw_lock_id);
			return CELL_OK;
		}

		rwlock->wcv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	rwlock->writer = CPU.GetId();
	rwlock->wwaiters--;

	return CELL_OK;
}

s32 sys_rwlock_trywlock(PPUThread& CPU, u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_trywlock(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = Emu.GetIdManager().get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->writer == CPU.GetId())
	{
		return CELL_EDEADLK;
	}

	if (rwlock->readers || rwlock->writer || rwlock->wwaiters)
	{
		return CELL_EBUSY;
	}

	rwlock->writer = CPU.GetId();

	return CELL_OK;
}

s32 sys_rwlock_wunlock(PPUThread& CPU, u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_wunlock(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = Emu.GetIdManager().get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->writer != CPU.GetId())
	{
		return CELL_EPERM;
	}

	rwlock->writer = 0;

	if (rwlock->wwaiters)
	{
		rwlock->wcv.notify_one();
	}
	else if (rwlock->rwaiters)
	{
		rwlock->rcv.notify_all();
	}

	return CELL_OK;
}
