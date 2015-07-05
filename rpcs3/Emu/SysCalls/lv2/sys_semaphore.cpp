#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sleep_queue.h"
#include "sys_semaphore.h"

SysCallBase sys_semaphore("sys_semaphore");

extern u64 get_system_time();

s32 sys_semaphore_create(vm::ptr<u32> sem, vm::ptr<sys_semaphore_attribute_t> attr, s32 initial_val, s32 max_val)
{
	sys_semaphore.Warning("sys_semaphore_create(sem=*0x%x, attr=*0x%x, initial_val=%d, max_val=%d)", sem, attr, initial_val, max_val);

	if (!sem || !attr)
	{
		return CELL_EFAULT;
	}

	if (max_val <= 0 || initial_val > max_val || initial_val < 0)
	{
		sys_semaphore.Error("sys_semaphore_create(): invalid parameters (initial_val=%d, max_val=%d)", initial_val, max_val);
		return CELL_EINVAL;
	}

	const u32 protocol = attr->protocol;

	switch (protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_PRIORITY: break;
	case SYS_SYNC_PRIORITY_INHERIT: break;
	default: sys_semaphore.Error("sys_semaphore_create(): unknown protocol (0x%x)", protocol); return CELL_EINVAL;
	}

	if (attr->pshared != SYS_SYNC_NOT_PROCESS_SHARED || attr->ipc_key.data() || attr->flags.data())
	{
		sys_semaphore.Error("sys_semaphore_create(): unknown attributes (pshared=0x%x, ipc_key=0x%x, flags=0x%x)", attr->pshared, attr->ipc_key, attr->flags);
		return CELL_EINVAL;
	}

	*sem = Emu.GetIdManager().make<lv2_sema_t>(protocol, max_val, attr->name_u64, initial_val);

	return CELL_OK;
}

s32 sys_semaphore_destroy(u32 sem)
{
	sys_semaphore.Warning("sys_semaphore_destroy(sem=0x%x)", sem);

	LV2_LOCK;

	const auto semaphore = Emu.GetIdManager().get<lv2_sema_t>(sem);

	if (!semaphore)
	{
		return CELL_ESRCH;
	}
	
	if (semaphore->waiters)
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().remove<lv2_sema_t>(sem);

	return CELL_OK;
}

s32 sys_semaphore_wait(u32 sem, u64 timeout)
{
	sys_semaphore.Log("sys_semaphore_wait(sem=0x%x, timeout=0x%llx)", sem, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto semaphore = Emu.GetIdManager().get<lv2_sema_t>(sem);

	if (!semaphore)
	{
		return CELL_ESRCH;
	}

	// protocol is ignored in current implementation
	semaphore->waiters++;

	while (semaphore->value <= 0)
	{
		CHECK_EMU_STATUS;

		if (timeout && get_system_time() - start_time > timeout)
		{
			semaphore->waiters--;
			return CELL_ETIMEDOUT;
		}

		semaphore->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	semaphore->value--;
	semaphore->waiters--;

	return CELL_OK;
}

s32 sys_semaphore_trywait(u32 sem)
{
	sys_semaphore.Log("sys_semaphore_trywait(sem=0x%x)", sem);

	LV2_LOCK;

	const auto semaphore = Emu.GetIdManager().get<lv2_sema_t>(sem);

	if (!semaphore)
	{
		return CELL_ESRCH;
	}

	if (semaphore->value <= 0 || semaphore->waiters)
	{
		return CELL_EBUSY;
	}

	semaphore->value--;

	return CELL_OK;
}

s32 sys_semaphore_post(u32 sem, s32 count)
{
	sys_semaphore.Log("sys_semaphore_post(sem=0x%x, count=%d)", sem, count);

	LV2_LOCK;

	const auto semaphore = Emu.GetIdManager().get<lv2_sema_t>(sem);

	if (!semaphore)
	{
		return CELL_ESRCH;
	}

	if (count < 0)
	{
		return CELL_EINVAL;
	}

	const u64 new_value = semaphore->value + count;
	const u64 max_value = semaphore->max + semaphore->waiters;

	if (new_value > max_value)
	{
		return CELL_EBUSY;
	}

	semaphore->value += count;

	if (semaphore->waiters)
	{
		semaphore->cv.notify_all();
	}

	return CELL_OK;
}

s32 sys_semaphore_get_value(u32 sem, vm::ptr<s32> count)
{
	sys_semaphore.Log("sys_semaphore_get_value(sem=0x%x, count=*0x%x)", sem, count);

	if (!count)
	{
		return CELL_EFAULT;
	}

	LV2_LOCK;

	const auto semaphore = Emu.GetIdManager().get<lv2_sema_t>(sem);

	if (!semaphore)
	{
		return CELL_ESRCH;
	}

	*count = std::max<s32>(0, semaphore->value - semaphore->waiters);

	return CELL_OK;
}
