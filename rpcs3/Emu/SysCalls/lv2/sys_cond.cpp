#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_mutex.h"
#include "sys_cond.h"

SysCallBase sys_cond("sys_cond");

extern u64 get_system_time();

s32 sys_cond_create(vm::ptr<u32> cond_id, u32 mutex_id, vm::ptr<sys_cond_attribute_t> attr)
{
	sys_cond.Warning("sys_cond_create(cond_id=*0x%x, mutex_id=0x%x, attr=*0x%x)", cond_id, mutex_id, attr);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (attr->pshared != SYS_SYNC_NOT_PROCESS_SHARED || attr->ipc_key.data() || attr->flags.data())
	{
		sys_cond.Error("sys_cond_create(): unknown attributes (pshared=0x%x, ipc_key=0x%llx, flags=0x%x)", attr->pshared, attr->ipc_key, attr->flags);
		return CELL_EINVAL;
	}

	if (!++mutex->cond_count)
	{
		throw EXCEPTION("Unexpected cond_count");
	}

	*cond_id = Emu.GetIdManager().make<lv2_cond_t>(mutex, attr->name_u64);

	return CELL_OK;
}

s32 sys_cond_destroy(u32 cond_id)
{
	sys_cond.Warning("sys_cond_destroy(cond_id=0x%x)", cond_id);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().get<lv2_cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (!cond->sq.empty())
	{
		return CELL_EBUSY;
	}

	if (!cond->mutex->cond_count--)
	{
		throw EXCEPTION("Unexpected cond_count");
	}

	Emu.GetIdManager().remove<lv2_cond_t>(cond_id);

	return CELL_OK;
}

s32 sys_cond_signal(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal(cond_id=0x%x)", cond_id);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().get<lv2_cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	for (auto& thread : cond->sq)
	{
		// signal one waiting thread; protocol is ignored in current implementation
		if (thread->Signal())
		{
			return CELL_OK;
		}
	}

	return CELL_OK;
}

s32 sys_cond_signal_all(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal_all(cond_id=0x%x)", cond_id);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().get<lv2_cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	for (auto& thread : cond->sq)
	{
		// signal all waiting threads; protocol is ignored in current implementation
		if (thread->Signal())
		{
			;
		}
	}

	return CELL_OK;
}

s32 sys_cond_signal_to(u32 cond_id, u32 thread_id)
{
	sys_cond.Log("sys_cond_signal_to(cond_id=0x%x, thread_id=0x%x)", cond_id, thread_id);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().get<lv2_cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	// TODO: check if CELL_ESRCH is returned if thread_id is invalid

	for (auto& thread : cond->sq)
	{
		// signal specified thread
		if (thread->GetId() == thread_id && thread->Signal())
		{
			return CELL_OK;
		}
	}

	return CELL_EPERM;
}

s32 sys_cond_wait(PPUThread& ppu, u32 cond_id, u64 timeout)
{
	sys_cond.Log("sys_cond_wait(cond_id=0x%x, timeout=%lld)", cond_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().get<lv2_cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	// check current ownership
	if (cond->mutex->owner.get() != &ppu)
	{
		return CELL_EPERM;
	}

	// save the recursive value
	const u32 recursive_value = cond->mutex->recursive_count.exchange(0);

	// unlock the mutex
	cond->mutex->unlock(lv2_lock);

	{
		// add waiter; protocol is ignored in current implementation
		sleep_queue_entry_t waiter(ppu, cond->sq);

		while (!ppu.Signaled())
		{
			if (timeout)
			{
				const u64 passed = get_system_time() - start_time;

				if (passed >= timeout || ppu.cv.wait_for(lv2_lock, std::chrono::microseconds(timeout - passed)) == std::cv_status::timeout)
				{
					break;
				}
			}
			else
			{
				ppu.cv.wait(lv2_lock);
			}

			CHECK_EMU_STATUS;
		}
	}

	// reown the mutex (could be set when notified)
	if (!cond->mutex->owner)
	{
		cond->mutex->owner = ppu.shared_from_this();
	}

	if (cond->mutex->owner.get() != &ppu)
	{
		// add waiter; protocol is ignored in current implementation
		sleep_queue_entry_t waiter(ppu, cond->mutex->sq);

		while (!ppu.Signaled())
		{
			ppu.cv.wait(lv2_lock);

			CHECK_EMU_STATUS;
		}

		if (cond->mutex->owner.get() != &ppu)
		{
			throw EXCEPTION("Unexpected mutex owner");
		}
	}

	// restore the recursive value
	cond->mutex->recursive_count = recursive_value;

	// check timeout
	if (timeout && get_system_time() - start_time > timeout)
	{
		return CELL_ETIMEDOUT;
	}

	return CELL_OK;
}
