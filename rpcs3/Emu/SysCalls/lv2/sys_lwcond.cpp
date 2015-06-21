#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/Modules/sysPrxForUser.h"
#include "sleep_queue.h"
#include "sys_time.h"
#include "sys_lwmutex.h"
#include "sys_lwcond.h"

SysCallBase sys_lwcond("sys_lwcond");

void lwcond_create(sys_lwcond_t& lwcond, sys_lwmutex_t& lwmutex, u64 name)
{
	lwcond.lwmutex.set(vm::get_addr(&lwmutex));
	lwcond.lwcond_queue = Emu.GetIdManager().make<lv2_lwcond_t>(name);
}

s32 _sys_lwcond_create(vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name, u32 arg5)
{
	sys_lwcond.Warning("_sys_lwcond_create(lwcond_id=*0x%x, lwmutex_id=0x%x, control=*0x%x, name=0x%llx, arg5=0x%x)", lwcond_id, lwmutex_id, control, name, arg5);

	*lwcond_id = Emu.GetIdManager().make<lv2_lwcond_t>(name);

	return CELL_OK;
}

s32 _sys_lwcond_destroy(u32 lwcond_id)
{
	sys_lwcond.Warning("_sys_lwcond_destroy(lwcond_id=0x%x)", lwcond_id);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().get<lv2_lwcond_t>(lwcond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (!cond->waiters.empty() || cond->signaled1 || cond->signaled2)
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().remove<lv2_lwcond_t>(lwcond_id);

	return CELL_OK;
}

s32 _sys_lwcond_signal(u32 lwcond_id, u32 lwmutex_id, u32 ppu_thread_id, u32 mode)
{
	sys_lwcond.Log("_sys_lwcond_signal(lwcond_id=0x%x, lwmutex_id=0x%x, ppu_thread_id=0x%x, mode=%d)", lwcond_id, lwmutex_id, ppu_thread_id, mode);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().get<lv2_lwcond_t>(lwcond_id);
	const auto mutex = Emu.GetIdManager().get<lv2_lwmutex_t>(lwmutex_id);

	if (!cond || (lwmutex_id && !mutex))
	{
		return CELL_ESRCH;
	}

	if (mode != 1 && mode != 2 && mode != 3)
	{
		sys_lwcond.Error("_sys_lwcond_signal(%d): invalid mode (%d)", lwcond_id, mode);
	}

	const auto found = ~ppu_thread_id ? cond->waiters.find(ppu_thread_id) : cond->waiters.begin();

	if (mode == 1)
	{
		// mode 1: lightweight mutex was initially owned by the calling thread

		if (found == cond->waiters.end())
		{
			return CELL_EPERM;
		}

		cond->signaled1++;
	}
	else if (mode == 2)
	{
		// mode 2: lightweight mutex was not owned by the calling thread and waiter hasn't been increased

		if (found == cond->waiters.end())
		{
			return CELL_OK;
		}

		cond->signaled2++;
	}
	else
	{
		// in mode 3, lightweight mutex was forcefully owned by the calling thread

		if (found == cond->waiters.end())
		{
			return ~ppu_thread_id ? CELL_ENOENT : CELL_EPERM;
		}

		cond->signaled1++;
	}

	cond->waiters.erase(found);
	cond->cv.notify_one();

	return CELL_OK;
}

s32 _sys_lwcond_signal_all(u32 lwcond_id, u32 lwmutex_id, u32 mode)
{
	sys_lwcond.Log("_sys_lwcond_signal_all(lwcond_id=0x%x, lwmutex_id=0x%x, mode=%d)", lwcond_id, lwmutex_id, mode);

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().get<lv2_lwcond_t>(lwcond_id);
	const auto mutex = Emu.GetIdManager().get<lv2_lwmutex_t>(lwmutex_id);

	if (!cond || (lwmutex_id && !mutex))
	{
		return CELL_ESRCH;
	}

	if (mode != 1 && mode != 2)
	{
		sys_lwcond.Error("_sys_lwcond_signal_all(%d): invalid mode (%d)", lwcond_id, mode);
	}

	const u32 count = (u32)cond->waiters.size();

	if (count)
	{
		cond->waiters.clear();
		cond->cv.notify_all();
	}

	if (mode == 1)
	{
		// in mode 1, lightweight mutex was initially owned by the calling thread

		cond->signaled1 += count;

		return count;
	}
	else
	{
		// in mode 2, lightweight mutex was not owned by the calling thread and waiter hasn't been increased

		cond->signaled2 += count;

		return CELL_OK;
	}
}

s32 _sys_lwcond_queue_wait(PPUThread& CPU, u32 lwcond_id, u32 lwmutex_id, u64 timeout)
{
	sys_lwcond.Log("_sys_lwcond_queue_wait(lwcond_id=0x%x, lwmutex_id=0x%x, timeout=0x%llx)", lwcond_id, lwmutex_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto cond = Emu.GetIdManager().get<lv2_lwcond_t>(lwcond_id);
	const auto mutex = Emu.GetIdManager().get<lv2_lwmutex_t>(lwmutex_id);

	if (!cond || !mutex)
	{
		return CELL_ESRCH;
	}

	// finalize unlocking the mutex
	mutex->signaled++;

	if (mutex->waiters)
	{
		mutex->cv.notify_one();
	}

	// add waiter; protocol is ignored in current implementation
	cond->waiters.emplace(CPU.GetId());

	while ((!(cond->signaled1 && mutex->signaled) && !cond->signaled2) || cond->waiters.count(CPU.GetId()))
	{
		const bool is_timedout = timeout && get_system_time() - start_time > timeout;

		// check timeout
		if (is_timedout)
		{
			// cancel waiting
			if (!cond->waiters.erase(CPU.GetId()))
			{
				if (cond->signaled1 && !mutex->signaled)
				{
					cond->signaled1--;
				}
				else
				{
					throw __FUNCTION__;
				}
			}

			if (mutex->signaled)
			{
				mutex->signaled--;

				return CELL_EDEADLK;
			}
			else
			{
				return CELL_ETIMEDOUT;
			}
		}

		if (Emu.IsStopped())
		{
			sys_lwcond.Warning("_sys_lwcond_queue_wait(lwcond_id=0x%x) aborted", lwcond_id);
			return CELL_OK;
		}

		(cond->signaled1 ? mutex->cv : cond->cv).wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	if (cond->signaled1 && mutex->signaled)
	{
		mutex->signaled--;
		cond->signaled1--;

		return CELL_OK;
	}
	else
	{
		cond->signaled2--;

		return CELL_EBUSY;
	}
}
