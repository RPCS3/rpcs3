#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_lwmutex.h"
#include "sys_lwcond.h"

SysCallBase sys_lwcond("sys_lwcond");

extern u64 get_system_time();

void lv2_lwcond_t::notify(lv2_lock_t & lv2_lock, sleep_queue_t::value_type& thread, const std::shared_ptr<lv2_lwmutex_t>& mutex, bool mode2)
{
	CHECK_LV2_LOCK(lv2_lock);

	auto& ppu = static_cast<PPUThread&>(*thread);

	ppu.GPR[3] = mode2;  // set to return CELL_EBUSY

	if (!mode2)
	{
		if (!mutex->signaled)
		{
			return mutex->sq.emplace_back(thread);
		}

		mutex->signaled--;
	}

	if (!ppu.signal())
	{
		throw EXCEPTION("Thread already signaled");
	}
}

s32 _sys_lwcond_create(vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name, u32 arg5)
{
	sys_lwcond.Warning("_sys_lwcond_create(lwcond_id=*0x%x, lwmutex_id=0x%x, control=*0x%x, name=0x%llx, arg5=0x%x)", lwcond_id, lwmutex_id, control, name, arg5);

	*lwcond_id = idm::make<lv2_lwcond_t>(name);

	return CELL_OK;
}

s32 _sys_lwcond_destroy(u32 lwcond_id)
{
	sys_lwcond.Warning("_sys_lwcond_destroy(lwcond_id=0x%x)", lwcond_id);

	LV2_LOCK;

	const auto cond = idm::get<lv2_lwcond_t>(lwcond_id);

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (!cond->sq.empty())
	{
		return CELL_EBUSY;
	}

	idm::remove<lv2_lwcond_t>(lwcond_id);

	return CELL_OK;
}

s32 _sys_lwcond_signal(u32 lwcond_id, u32 lwmutex_id, u32 ppu_thread_id, u32 mode)
{
	sys_lwcond.Log("_sys_lwcond_signal(lwcond_id=0x%x, lwmutex_id=0x%x, ppu_thread_id=0x%x, mode=%d)", lwcond_id, lwmutex_id, ppu_thread_id, mode);

	LV2_LOCK;

	const auto cond = idm::get<lv2_lwcond_t>(lwcond_id);
	const auto mutex = idm::get<lv2_lwmutex_t>(lwmutex_id);

	if (!cond || (lwmutex_id && !mutex))
	{
		return CELL_ESRCH;
	}

	if (mode != 1 && mode != 2 && mode != 3)
	{
		throw EXCEPTION("Unknown mode (%d)", mode);
	}

	// mode 1: lightweight mutex was initially owned by the calling thread
	// mode 2: lightweight mutex was not owned by the calling thread and waiter hasn't been increased
	// mode 3: lightweight mutex was forcefully owned by the calling thread

	// pick waiter; protocol is ignored in current implementation
	const auto found = !~ppu_thread_id ? cond->sq.begin() : std::find_if(cond->sq.begin(), cond->sq.end(), [=](sleep_queue_t::value_type& thread)
	{
		return thread->get_id() == ppu_thread_id;
	});

	if (found == cond->sq.end())
	{
		if (mode == 1)
		{
			return CELL_EPERM;
		}
		else if (mode == 2)
		{
			return CELL_OK;
		}
		else
		{
			return ~ppu_thread_id ? CELL_ENOENT : CELL_EPERM;
		}
	}

	// signal specified waiting thread
	cond->notify(lv2_lock, *found, mutex, mode == 2);

	cond->sq.erase(found);

	return CELL_OK;
}

s32 _sys_lwcond_signal_all(u32 lwcond_id, u32 lwmutex_id, u32 mode)
{
	sys_lwcond.Log("_sys_lwcond_signal_all(lwcond_id=0x%x, lwmutex_id=0x%x, mode=%d)", lwcond_id, lwmutex_id, mode);

	LV2_LOCK;

	const auto cond = idm::get<lv2_lwcond_t>(lwcond_id);
	const auto mutex = idm::get<lv2_lwmutex_t>(lwmutex_id);

	if (!cond || (lwmutex_id && !mutex))
	{
		return CELL_ESRCH;
	}

	if (mode != 1 && mode != 2)
	{
		throw EXCEPTION("Unknown mode (%d)", mode);
	}

	// mode 1: lightweight mutex was initially owned by the calling thread
	// mode 2: lightweight mutex was not owned by the calling thread and waiter hasn't been increased

	// signal all waiting threads; protocol is ignored in current implementation
	for (auto& thread : cond->sq)
	{
		cond->notify(lv2_lock, thread, mutex, mode == 2);
	}

	// in mode 1, return the amount of threads signaled
	const s32 result = mode == 2 ? CELL_OK : static_cast<s32>(cond->sq.size());

	cond->sq.clear();

	return result;
}

s32 _sys_lwcond_queue_wait(PPUThread& ppu, u32 lwcond_id, u32 lwmutex_id, u64 timeout)
{
	sys_lwcond.Log("_sys_lwcond_queue_wait(lwcond_id=0x%x, lwmutex_id=0x%x, timeout=0x%llx)", lwcond_id, lwmutex_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto cond = idm::get<lv2_lwcond_t>(lwcond_id);
	const auto mutex = idm::get<lv2_lwmutex_t>(lwmutex_id);

	if (!cond || !mutex)
	{
		return CELL_ESRCH;
	}

	// finalize unlocking the mutex
	mutex->unlock(lv2_lock);

	// add waiter; protocol is ignored in current implementation
	sleep_queue_entry_t waiter(ppu, cond->sq);

	// potential mutex waiter (not added immediately)
	sleep_queue_entry_t mutex_waiter(ppu, cond->sq, defer_sleep);

	while (!ppu.unsignal())
	{
		CHECK_EMU_STATUS;

		if (timeout && waiter)
		{
			const u64 passed = get_system_time() - start_time;

			if (passed >= timeout)
			{
				// try to reown the mutex if timed out
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

			ppu.cv.wait_for(lv2_lock, std::chrono::microseconds(timeout - passed));
		}
		else
		{
			ppu.cv.wait(lv2_lock);
		}
	}

	// return cause
	return ppu.GPR[3] ? CELL_EBUSY : CELL_OK;
}
