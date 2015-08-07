#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_sync.h"
#include "sys_semaphore.h"

SysCallBase sys_semaphore("sys_semaphore");

extern u64 get_system_time();

s32 sys_semaphore_create(vm::ptr<u32> sem_id, vm::ptr<sys_semaphore_attribute_t> attr, s32 initial_val, s32 max_val)
{
	sys_semaphore.Warning("sys_semaphore_create(sem_id=*0x%x, attr=*0x%x, initial_val=%d, max_val=%d)", sem_id, attr, initial_val, max_val);

	if (!sem_id || !attr)
	{
		return CELL_EFAULT;
	}

	if (max_val <= 0 || initial_val > max_val || initial_val < 0)
	{
		sys_semaphore.Error("sys_semaphore_create(): invalid parameters (initial_val=%d, max_val=%d)", initial_val, max_val);
		return CELL_EINVAL;
	}

	const u32 protocol = attr->protocol;

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_PRIORITY && protocol != SYS_SYNC_PRIORITY_INHERIT)
	{
		sys_semaphore.Error("sys_semaphore_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	if (attr->pshared != SYS_SYNC_NOT_PROCESS_SHARED || attr->ipc_key.data() || attr->flags.data())
	{
		sys_semaphore.Error("sys_semaphore_create(): unknown attributes (pshared=0x%x, ipc_key=0x%x, flags=0x%x)", attr->pshared, attr->ipc_key, attr->flags);
		return CELL_EINVAL;
	}

	*sem_id = idm::make<lv2_sema_t>(protocol, max_val, attr->name_u64, initial_val);

	return CELL_OK;
}

s32 sys_semaphore_destroy(u32 sem_id)
{
	sys_semaphore.Warning("sys_semaphore_destroy(sem_id=0x%x)", sem_id);

	LV2_LOCK;

	const auto sem = idm::get<lv2_sema_t>(sem_id);

	if (!sem)
	{
		return CELL_ESRCH;
	}
	
	if (sem->sq.size())
	{
		return CELL_EBUSY;
	}

	idm::remove<lv2_sema_t>(sem_id);

	return CELL_OK;
}

s32 sys_semaphore_wait(PPUThread& ppu, u32 sem_id, u64 timeout)
{
	sys_semaphore.Log("sys_semaphore_wait(sem_id=0x%x, timeout=0x%llx)", sem_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto sem = idm::get<lv2_sema_t>(sem_id);

	if (!sem)
	{
		return CELL_ESRCH;
	}

	if (sem->value > 0)
	{
		sem->value--;
		
		return CELL_OK;
	}

	// add waiter; protocol is ignored in current implementation
	sleep_queue_entry_t waiter(ppu, sem->sq);

	while (!ppu.unsignal())
	{
		CHECK_EMU_STATUS;

		if (timeout)
		{
			const u64 passed = get_system_time() - start_time;

			if (passed >= timeout)
			{
				return CELL_ETIMEDOUT;
			}

			ppu.cv.wait_for(lv2_lock, std::chrono::microseconds(timeout - passed));
		}
		else
		{
			ppu.cv.wait(lv2_lock);
		}
	}

	return CELL_OK;
}

s32 sys_semaphore_trywait(u32 sem_id)
{
	sys_semaphore.Log("sys_semaphore_trywait(sem_id=0x%x)", sem_id);

	LV2_LOCK;

	const auto sem = idm::get<lv2_sema_t>(sem_id);

	if (!sem)
	{
		return CELL_ESRCH;
	}

	if (sem->value <= 0 || sem->sq.size())
	{
		return CELL_EBUSY;
	}

	sem->value--;

	return CELL_OK;
}

s32 sys_semaphore_post(u32 sem_id, s32 count)
{
	sys_semaphore.Log("sys_semaphore_post(sem_id=0x%x, count=%d)", sem_id, count);

	LV2_LOCK;

	const auto sem = idm::get<lv2_sema_t>(sem_id);

	if (!sem)
	{
		return CELL_ESRCH;
	}

	if (count < 0)
	{
		return CELL_EINVAL;
	}

	// get comparable values considering waiting threads
	const u64 new_value = sem->value + count;
	const u64 max_value = sem->max + sem->sq.size();

	if (new_value > max_value)
	{
		return CELL_EBUSY;
	}

	// wakeup as much threads as possible
	while (count && !sem->sq.empty())
	{
		count--;

		if (!sem->sq.front()->signal())
		{
			throw EXCEPTION("Thread already signaled");
		}

		sem->sq.pop_front();
	}

	// add the rest to the value
	sem->value += count;

	return CELL_OK;
}

s32 sys_semaphore_get_value(u32 sem_id, vm::ptr<s32> count)
{
	sys_semaphore.Log("sys_semaphore_get_value(sem_id=0x%x, count=*0x%x)", sem_id, count);

	LV2_LOCK;

	if (!count)
	{
		return CELL_EFAULT;
	}

	const auto sem = idm::get<lv2_sema_t>(sem_id);

	if (!sem)
	{
		return CELL_ESRCH;
	}

	*count = sem->value;

	return CELL_OK;
}
