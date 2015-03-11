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
	std::shared_ptr<lwcond_t> cond(new lwcond_t(name));

	lwcond.lwcond_queue = Emu.GetIdManager().GetNewID(cond, TYPE_LWCOND);
}

s32 _sys_lwcond_create(vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name, u32 arg5)
{
	sys_lwcond.Warning("_sys_lwcond_create(lwcond_id=*0x%x, lwmutex_id=%d, control=*0x%x, name=0x%llx, arg5=0x%x)", lwcond_id, lwmutex_id, control, name, arg5);

	std::shared_ptr<lwcond_t> cond(new lwcond_t(name));

	*lwcond_id = Emu.GetIdManager().GetNewID(cond, TYPE_LWCOND);

	return CELL_OK;
}

s32 _sys_lwcond_destroy(u32 lwcond_id)
{
	sys_lwcond.Warning("_sys_lwcond_destroy(lwcond_id=%d)", lwcond_id);

	LV2_LOCK;

	std::shared_ptr<lwcond_t> cond;
	if (!Emu.GetIdManager().GetIDData(lwcond_id, cond))
	{
		return CELL_ESRCH;
	}

	if (cond->waiters)
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID<lwcond_t>(lwcond_id);

	return CELL_OK;
}

s32 _sys_lwcond_signal(u32 lwcond_id, u32 lwmutex_id, u32 ppu_thread_id, u32 mode)
{
	sys_lwcond.Log("_sys_lwcond_signal(lwcond_id=%d, lwmutex_id=%d, ppu_thread_id=%d, mode=%d)", lwcond_id, lwmutex_id, ppu_thread_id, mode);

	LV2_LOCK;

	std::shared_ptr<lwcond_t> cond;
	if (!Emu.GetIdManager().GetIDData(lwcond_id, cond))
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<lwmutex_t> mutex;
	if (lwmutex_id && !Emu.GetIdManager().GetIDData(lwmutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	// ppu_thread_id is ignored in current implementation

	if (mode != 1 && mode != 2 && mode != 3)
	{
		sys_lwcond.Error("_sys_lwcond_signal(%d): invalid mode (%d)", lwcond_id, mode);
	}

	if (~ppu_thread_id)
	{
		sys_lwcond.Todo("_sys_lwcond_signal(%d): ppu_thread_id (%d)", lwcond_id, ppu_thread_id);
	}

	if (mode == 1)
	{
		// mode 1: lightweight mutex was initially owned by the calling thread

		if (!cond->waiters)
		{
			return CELL_EPERM;
		}

		cond->signaled1++;
	}
	else if (mode == 2)
	{
		// mode 2: lightweight mutex was not owned by the calling thread and waiter hasn't been increased

		if (!cond->waiters)
		{
			return CELL_OK;
		}

		cond->signaled2++;
	}
	else
	{
		// in mode 3, lightweight mutex was forcefully owned by the calling thread

		if (!cond->waiters)
		{
			return ~ppu_thread_id ? CELL_ENOENT : CELL_EPERM;
		}

		cond->signaled1++;
	}

	cond->waiters--;
	cond->cv.notify_one();

	return CELL_OK;
}

s32 _sys_lwcond_signal_all(u32 lwcond_id, u32 lwmutex_id, u32 mode)
{
	sys_lwcond.Log("_sys_lwcond_signal_all(lwcond_id=%d, lwmutex_id=%d, mode=%d)", lwcond_id, lwmutex_id, mode);

	LV2_LOCK;

	std::shared_ptr<lwcond_t> cond;
	if (!Emu.GetIdManager().GetIDData(lwcond_id, cond))
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<lwmutex_t> mutex;
	if (lwmutex_id && !Emu.GetIdManager().GetIDData(lwmutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	if (mode != 1 && mode != 2)
	{
		sys_lwcond.Error("_sys_lwcond_signal_all(%d): invalid mode (%d)", lwcond_id, mode);
	}

	const s32 count = cond->waiters.exchange(0);
	cond->cv.notify_all();

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

s32 _sys_lwcond_queue_wait(u32 lwcond_id, u32 lwmutex_id, u64 timeout)
{
	sys_lwcond.Log("_sys_lwcond_queue_wait(lwcond_id=%d, lwmutex_id=%d, timeout=0x%llx)", lwcond_id, lwmutex_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	std::shared_ptr<lwcond_t> cond;
	if (!Emu.GetIdManager().GetIDData(lwcond_id, cond))
	{
		return CELL_ESRCH;
	}

	std::shared_ptr<lwmutex_t> mutex;
	if (!Emu.GetIdManager().GetIDData(lwmutex_id, mutex))
	{
		return CELL_ESRCH;
	}

	// finalize unlocking the mutex
	mutex->signaled++;
	mutex->cv.notify_one();

	// protocol is ignored in current implementation
	cond->waiters++; assert(cond->waiters > 0);

	while (!(cond->signaled1 && mutex->signaled) && !cond->signaled2)
	{
		const bool is_timedout = timeout && get_system_time() - start_time > timeout;

		// check timeout (TODO)
		if (is_timedout)
		{
			// cancel waiting
			cond->waiters--; assert(cond->waiters >= 0);

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
			sys_lwcond.Warning("_sys_lwcond_queue_wait(lwcond_id=%d) aborted", lwcond_id);
			return CELL_OK;
		}

		cond->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
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
