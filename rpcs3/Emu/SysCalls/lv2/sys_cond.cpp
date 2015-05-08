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
#include "sys_cond.h"

SysCallBase sys_cond("sys_cond");

s32 sys_cond_create(vm::ptr<u32> cond_id, u32 mutex_id, vm::ptr<sys_cond_attribute_t> attr)
{
	sys_cond.Warning("sys_cond_create(cond_id=*0x%x, mutex_id=0x%x, attr=*0x%x)", cond_id, mutex_id, attr);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().GetIDData<mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (attr->pshared.data() != se32(0x200) || attr->ipc_key.data() || attr->flags.data())
	{
		sys_cond.Error("sys_cond_create(): unknown attributes (pshared=0x%x, ipc_key=0x%llx, flags=0x%x)", attr->pshared, attr->ipc_key, attr->flags);
		return CELL_EINVAL;
	}

	if (!++mutex->cond_count)
	{
		throw __FUNCTION__;
	}

	std::shared_ptr<cond_t> cond(new cond_t(mutex, attr->name_u64));

	*cond_id = Emu.GetIdManager().GetNewID(cond, TYPE_COND);

	return CELL_OK;
}

s32 sys_cond_destroy(u32 cond_id)
{
	sys_cond.Warning("sys_cond_destroy(cond_id=0x%x)", cond_id);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().GetIDData<cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (!cond->waiters.empty() || cond->signaled)
	{
		return CELL_EBUSY;
	}

	if (!cond->mutex->cond_count--)
	{
		throw __FUNCTION__;
	}

	Emu.GetIdManager().RemoveID<cond_t>(cond_id);

	return CELL_OK;
}

s32 sys_cond_signal(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal(cond_id=0x%x)", cond_id);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().GetIDData<cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (!cond->waiters.empty())
	{
		cond->signaled++;
		cond->waiters.erase(cond->waiters.begin());
		cond->cv.notify_one();
	}

	return CELL_OK;
}

s32 sys_cond_signal_all(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal_all(cond_id=0x%x)", cond_id);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().GetIDData<cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (const u32 count = cond->waiters.size())
	{
		cond->signaled += count;
		cond->waiters.clear();
		cond->cv.notify_all();
	}

	return CELL_OK;
}

s32 sys_cond_signal_to(u32 cond_id, u32 thread_id)
{
	sys_cond.Log("sys_cond_signal_to(cond_id=0x%x, thread_id=0x%x)", cond_id, thread_id);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().GetIDData<cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (!Emu.GetIdManager().CheckID<CPUThread>(thread_id))
	{
		return CELL_ESRCH;
	}

	const auto found = cond->waiters.find(thread_id);

	if (found == cond->waiters.end())
	{
		return CELL_EPERM;
	}

	cond->signaled++;
	cond->waiters.erase(found);
	cond->cv.notify_one();

	return CELL_OK;
}

s32 sys_cond_wait(PPUThread& CPU, u32 cond_id, u64 timeout)
{
	sys_cond.Log("sys_cond_wait(cond_id=0x%x, timeout=%lld)", cond_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().GetIDData<cond_t>(cond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	const auto thread = Emu.GetCPU().GetThread(CPU.GetId());

	if (cond->mutex->owner.owner_before(thread) || thread.owner_before(cond->mutex->owner)) // check equality
	{
		return CELL_EPERM;
	}

	// add waiter; protocol is ignored in current implementation
	cond->waiters.emplace(CPU.GetId());

	// unlock mutex
	cond->mutex->owner.reset();

	if (cond->mutex->waiters)
	{
		cond->mutex->cv.notify_one();
	}

	// save recursive value
	const u32 recursive_value = cond->mutex->recursive_count.exchange(0);

	while (!cond->mutex->owner.expired() || !cond->signaled || cond->waiters.count(CPU.GetId()))
	{
		const bool is_timedout = timeout && get_system_time() - start_time > timeout;

		// check timeout
		if (is_timedout && cond->mutex->owner.expired())
		{
			// cancel waiting if the mutex is free, restore its owner and recursive value
			cond->mutex->owner = thread;
			cond->mutex->recursive_count = recursive_value;

			if (!cond->waiters.erase(CPU.GetId()))
			{
				throw __FUNCTION__;
			}

			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_cond.Warning("sys_cond_wait(id=0x%x) aborted", cond_id);
			return CELL_OK;
		}

		// wait on appropriate condition variable
		(cond->signaled || is_timedout ? cond->mutex->cv : cond->cv).wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	// reown the mutex and restore its recursive value
	cond->mutex->owner = thread;
	cond->mutex->recursive_count = recursive_value;
	cond->signaled--;

	return CELL_OK;
}
