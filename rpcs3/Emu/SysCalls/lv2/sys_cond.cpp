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

void lv2_cond_t::notify(lv2_lock_t& lv2_lock, sleep_queue_t::iterator it)
{
	CHECK_LV2_LOCK(lv2_lock);

	auto& thread = *it;

	if (mutex->owner)
	{
		// add thread to the mutex sleep queue if cannot lock immediately
		mutex->sq.emplace_back(thread);
	}
	else
	{
		mutex->owner = thread;

		if (!thread->signal())
		{
			throw EXCEPTION("Thread already signaled");
		}
	}
}

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

	// signal one waiting thread; protocol is ignored in current implementation
	if (!cond->sq.empty())
	{
		cond->notify(lv2_lock, cond->sq.begin());
		cond->sq.pop_front();
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

	// signal all waiting threads; protocol is ignored in current implementation
	for (auto it = cond->sq.begin(); it != cond->sq.end(); it++)
	{
		cond->notify(lv2_lock, it);
	}

	cond->sq.clear();

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

	// signal specified thread (protocol is not required)
	for (auto it = cond->sq.begin(); it != cond->sq.end(); it++)
	{
		if ((*it)->get_id() == thread_id)
		{
			cond->notify(lv2_lock, it);
			cond->sq.erase(it);
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

	// add waiter; protocol is ignored in current implementation
	sleep_queue_entry_t waiter(ppu, cond->sq);

	// potential mutex waiter (not added immediately)
	sleep_queue_entry_t mutex_waiter(ppu, cond->mutex->sq, defer_sleep);

	while (!ppu.unsignal())
	{
		CHECK_EMU_STATUS;

		// timeout is ignored if waiting on the cond var is already dropped
		if (timeout && waiter)
		{
			const u64 passed = get_system_time() - start_time;

			if (passed >= timeout)
			{
				// try to reown mutex and exit if timed out
				if (!cond->mutex->owner)
				{
					cond->mutex->owner = ppu.shared_from_this();
					break;
				}

				// drop condition variable and start waiting on the mutex queue
				mutex_waiter.enter();
				waiter.leave();
				continue;
			}

			ppu.cv.wait_for(lv2_lock, std::chrono::microseconds(timeout - passed));
		}
		else
		{
			ppu.cv.wait(lv2_lock);
		}
	}

	// mutex owner is restored after notification or unlocking
	if (cond->mutex->owner.get() != &ppu)
	{
		throw EXCEPTION("Unexpected mutex owner");
	}

	// restore the recursive value
	cond->mutex->recursive_count = recursive_value;

	// check timeout (unclear)
	if (timeout && get_system_time() - start_time > timeout)
	{
		return CELL_ETIMEDOUT;
	}

	return CELL_OK;
}
