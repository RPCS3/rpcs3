#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sleep_queue.h"
#include "sys_time.h"
#include "sys_event_flag.h"

SysCallBase sys_event_flag("sys_event_flag");

s32 sys_event_flag_create(vm::ptr<u32> id, vm::ptr<sys_event_flag_attr> attr, u64 init)
{
	sys_event_flag.Warning("sys_event_flag_create(id=*0x%x, attr=*0x%x, init=0x%llx)", id, attr, init);

	if (!id || !attr)
	{
		return CELL_EFAULT;
	}

	const u32 protocol = attr->protocol;

	switch (protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_RETRY: break;
	case SYS_SYNC_PRIORITY: break;
	case SYS_SYNC_PRIORITY_INHERIT: break;
	default: sys_event_flag.Error("sys_event_flag_create(): unknown protocol (0x%x)", attr->protocol); return CELL_EINVAL;
	}

	if (attr->pshared.data() != se32(0x200) || attr->ipc_key.data() || attr->flags.data())
	{
		sys_event_flag.Error("sys_event_flag_create(): unknown attributes (pshared=0x%x, ipc_key=0x%llx, flags=0x%x)", attr->pshared, attr->ipc_key, attr->flags);
		return CELL_EINVAL;
	}

	const u32 type = attr->type;

	switch (type)
	{
	case SYS_SYNC_WAITER_SINGLE: break;
	case SYS_SYNC_WAITER_MULTIPLE: break;
	default: sys_event_flag.Error("sys_event_flag_create(): unknown type (0x%x)", attr->type); return CELL_EINVAL;
	}

	std::shared_ptr<event_flag_t> ef(new event_flag_t(init, protocol, type, attr->name_u64));

	*id = Emu.GetIdManager().GetNewID(ef, TYPE_EVENT_FLAG);

	return CELL_OK;
}

s32 sys_event_flag_destroy(u32 id)
{
	sys_event_flag.Warning("sys_event_flag_destroy(id=0x%x)", id);

	LV2_LOCK;

	const auto ef = Emu.GetIdManager().GetIDData<event_flag_t>(id);

	if (!ef)
	{
		return CELL_ESRCH;
	}

	if (ef->waiters)
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID<event_flag_t>(id);

	return CELL_OK;
}

s32 sys_event_flag_wait(u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result, u64 timeout)
{
	sys_event_flag.Log("sys_event_flag_wait(id=0x%x, bitptn=0x%llx, mode=0x%x, result=*0x%x, timeout=0x%llx)", id, bitptn, mode, result, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	if (result)
	{
		*result = 0;
	}

	switch (mode & 0xf)
	{
	case SYS_EVENT_FLAG_WAIT_AND: break;
	case SYS_EVENT_FLAG_WAIT_OR: break;
	default: return CELL_EINVAL;
	}

	switch (mode & ~0xf)
	{
	case 0: break;
	case SYS_EVENT_FLAG_WAIT_CLEAR: break;
	case SYS_EVENT_FLAG_WAIT_CLEAR_ALL: break;
	default: return CELL_EINVAL;
	}

	const auto ef = Emu.GetIdManager().GetIDData<event_flag_t>(id);

	if (!ef)
	{
		return CELL_ESRCH;
	}

	if (ef->type == SYS_SYNC_WAITER_SINGLE && ef->waiters)
	{
		return CELL_EPERM;
	}

	while (ef->cancelled)
	{
		// wait until other threads return CELL_ECANCELED (to prevent modifying bit pattern)
		ef->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	// protocol is ignored in current implementation
	ef->waiters++;

	while (true)
	{
		if (result)
		{
			*result = ef->flags;
		}

		if (mode & SYS_EVENT_FLAG_WAIT_AND && (ef->flags & bitptn) == bitptn)
		{
			break;
		}

		if (mode & SYS_EVENT_FLAG_WAIT_OR && ef->flags & bitptn)
		{
			break;
		}

		if (ef->cancelled)
		{
			if (!--ef->cancelled)
			{
				ef->cv.notify_all();
			}

			return CELL_ECANCELED;
		}

		if (timeout && get_system_time() - start_time > timeout)
		{
			ef->waiters--;
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_event_flag.Warning("sys_event_flag_wait(id=0x%x) aborted", id);
			return CELL_OK;
		}

		ef->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	if (mode & SYS_EVENT_FLAG_WAIT_CLEAR)
	{
		ef->flags &= ~bitptn;
	}

	if (mode & SYS_EVENT_FLAG_WAIT_CLEAR_ALL)
	{
		ef->flags = 0;
	}

	if (--ef->waiters && ef->flags)
	{
		ef->cv.notify_one();
	}

	return CELL_OK;
}

s32 sys_event_flag_trywait(u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result)
{
	sys_event_flag.Log("sys_event_flag_trywait(id=0x%x, bitptn=0x%llx, mode=0x%x, result=*0x%x)", id, bitptn, mode, result);

	LV2_LOCK;

	if (result)
	{
		*result = 0;
	}

	switch (mode & 0xf)
	{
	case SYS_EVENT_FLAG_WAIT_AND: break;
	case SYS_EVENT_FLAG_WAIT_OR: break;
	default: return CELL_EINVAL;
	}

	switch (mode & ~0xf)
	{
	case 0: break;
	case SYS_EVENT_FLAG_WAIT_CLEAR: break;
	case SYS_EVENT_FLAG_WAIT_CLEAR_ALL: break;
	default: return CELL_EINVAL;
	}

	const auto ef = Emu.GetIdManager().GetIDData<event_flag_t>(id);

	if (!ef)
	{
		return CELL_ESRCH;
	}

	if (!((mode & SYS_EVENT_FLAG_WAIT_AND) && (ef->flags & bitptn) == bitptn) && !((mode & SYS_EVENT_FLAG_WAIT_OR) && (ef->flags & bitptn)))
	{
		return CELL_EBUSY;
	}

	while (ef->cancelled)
	{
		ef->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	if (mode & SYS_EVENT_FLAG_WAIT_CLEAR)
	{
		ef->flags &= ~bitptn;
	}
	else if (mode & SYS_EVENT_FLAG_WAIT_CLEAR_ALL)
	{
		ef->flags &= 0;
	}

	if (result)
	{
		*result = ef->flags;
	}

	return CELL_OK;
}

s32 sys_event_flag_set(u32 id, u64 bitptn)
{
	sys_event_flag.Log("sys_event_flag_set(id=0x%x, bitptn=0x%llx)", id, bitptn);

	LV2_LOCK;

	const auto ef = Emu.GetIdManager().GetIDData<event_flag_t>(id);

	if (!ef)
	{
		return CELL_ESRCH;
	}

	while (ef->cancelled)
	{
		ef->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	ef->flags |= bitptn;

	if (ef->waiters)
	{
		ef->cv.notify_all();
	}
	
	return CELL_OK;
}

s32 sys_event_flag_clear(u32 id, u64 bitptn)
{
	sys_event_flag.Log("sys_event_flag_clear(id=0x%x, bitptn=0x%llx)", id, bitptn);

	LV2_LOCK;

	const auto ef = Emu.GetIdManager().GetIDData<event_flag_t>(id);

	if (!ef)
	{
		return CELL_ESRCH;
	}

	while (ef->cancelled)
	{
		ef->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	ef->flags &= bitptn;

	return CELL_OK;
}

s32 sys_event_flag_cancel(u32 id, vm::ptr<u32> num)
{
	sys_event_flag.Log("sys_event_flag_cancel(id=0x%x, num=*0x%x)", id, num);

	LV2_LOCK;

	if (num)
	{
		*num = 0;
	}

	const auto ef = Emu.GetIdManager().GetIDData<event_flag_t>(id);

	if (!ef)
	{
		return CELL_ESRCH;
	}

	while (ef->cancelled)
	{
		ef->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	if (num)
	{
		*num = ef->waiters;
	}

	if ((ef->cancelled = ef->waiters.exchange(0)))
	{
		ef->cv.notify_all();
	}
	
	return CELL_OK;
}

s32 sys_event_flag_get(u32 id, vm::ptr<u64> flags)
{
	sys_event_flag.Log("sys_event_flag_get(id=0x%x, flags=*0x%x)", id, flags);

	LV2_LOCK;

	if (!flags)
	{
		return CELL_EFAULT;
	}

	const auto ef = Emu.GetIdManager().GetIDData<event_flag_t>(id);

	if (!ef)
	{
		*flags = 0;

		return CELL_ESRCH;
	}

	*flags = ef->flags;

	return CELL_OK;
}
