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
#include "sys_cond.h"

SysCallBase sys_cond("sys_cond");

s32 sys_cond_create(vm::ptr<u32> cond_id, u32 mutex_id, vm::ptr<sys_cond_attribute> attr)
{
	sys_cond.Log("sys_cond_create(cond_id_addr=0x%x, mutex_id=%d, attr_addr=0x%x)",
		cond_id.addr(), mutex_id, attr.addr());

	LV2_LOCK(0);

	if (attr->pshared.ToBE() != se32(0x200))
	{
		sys_cond.Error("Invalid pshared attribute(0x%x)", (u32)attr->pshared);
		return CELL_EINVAL;
	}

	std::shared_ptr<Mutex> mutex;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<Cond> cond(new Cond(mutex, attr->name_u64));
	const u32 id = sys_cond.GetNewId(cond, TYPE_COND);
	*cond_id = id;
	mutex->cond_count++;
	sys_cond.Warning("*** condition created [%s] (mutex_id=%d): id = %d", std::string(attr->name, 8).c_str(), mutex_id, id);

	return CELL_OK;
}

s32 sys_cond_destroy(u32 cond_id)
{
	sys_cond.Warning("sys_cond_destroy(cond_id=%d)", cond_id);

	LV2_LOCK(0);

	std::shared_ptr<Cond> cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	if (cond->queue.count()) // TODO: safely make object unusable
	{
		return CELL_EBUSY;
	}

	cond->mutex->cond_count--;
	Emu.GetIdManager().RemoveID(cond_id);
	return CELL_OK;
}

s32 sys_cond_signal(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal(cond_id=%d)", cond_id);

	std::shared_ptr<Cond> cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<Mutex> mutex = cond->mutex;

	if (u32 target = cond->queue.pop(mutex->protocol))
	{
		cond->signal.push(target);

		if (Emu.IsStopped())
		{
			sys_cond.Warning("sys_cond_signal(id=%d) aborted", cond_id);
		}
	}

	return CELL_OK;
}

s32 sys_cond_signal_all(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal_all(cond_id=%d)", cond_id);

	std::shared_ptr<Cond> cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<Mutex> mutex = cond->mutex;

	while (u32 target = cond->queue.pop(mutex->protocol))
	{
		cond->signal.push(target);

		if (Emu.IsStopped())
		{
			sys_cond.Warning("sys_cond_signal_all(id=%d) aborted", cond_id);
			break;
		}
	}

	return CELL_OK;
}

s32 sys_cond_signal_to(u32 cond_id, u32 thread_id)
{
	sys_cond.Log("sys_cond_signal_to(cond_id=%d, thread_id=%d)", cond_id, thread_id);

	std::shared_ptr<Cond> cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	if (!Emu.GetIdManager().CheckID(thread_id))
	{
		return CELL_ESRCH;
	}

	if (!cond->queue.invalidate(thread_id))
	{
		return CELL_EPERM;
	}

	std::shared_ptr<Mutex> mutex = cond->mutex;

	u32 target = thread_id;
	{
		cond->signal.push(target);
	}

	if (Emu.IsStopped())
	{
		sys_cond.Warning("sys_cond_signal_to(id=%d, to=%d) aborted", cond_id, thread_id);
	}

	return CELL_OK;
}

s32 sys_cond_wait(PPUThread& CPU, u32 cond_id, u64 timeout)
{
	sys_cond.Log("sys_cond_wait(cond_id=%d, timeout=%lld)", cond_id, timeout);

	std::shared_ptr<Cond> cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<Mutex> mutex = cond->mutex;

	const u32 tid = CPU.GetId();
	if (mutex->owner.read_sync() != tid)
	{
		return CELL_EPERM;
	}

	cond->queue.push(tid, mutex->protocol);

	auto old_recursive = mutex->recursive_count.load();
	mutex->recursive_count = 0;
	if (!mutex->owner.compare_and_swap_test(tid, mutex->queue.pop(mutex->protocol)))
	{
		assert(!"sys_cond_wait() failed");
	}

	bool pushed_in_sleep_queue = false;
	const u64 time_start = get_system_time();
	while (true)
	{
		u32 signaled;
		if (cond->signal.try_peek(signaled) && signaled == tid) // check signaled threads
		{
			if (mutex->owner.compare_and_swap_test(0, tid)) // try to lock
			{
				break;
			}

			if (!pushed_in_sleep_queue)
			{
				mutex->queue.push(tid, mutex->protocol);
				pushed_in_sleep_queue = true;
			}
				
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
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (timeout && get_system_time() - time_start > timeout)
		{
			cond->queue.invalidate(tid);
			CPU.owned_mutexes--; // ???
			return CELL_ETIMEDOUT; // mutex not locked
		}

		if (Emu.IsStopped())
		{
			sys_cond.Warning("sys_cond_wait(id=%d) aborted", cond_id);
			return CELL_OK;
		}
	}

	mutex->recursive_count = old_recursive;
	cond->signal.pop(cond_id /* unused result */);
	return CELL_OK;
}
