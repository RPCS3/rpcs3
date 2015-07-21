#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_lwmutex.h"

SysCallBase sys_lwmutex("sys_lwmutex");

extern u64 get_system_time();

void lv2_lwmutex_t::unlock(lv2_lock_t& lv2_lock)
{
	CHECK_LV2_LOCK(lv2_lock);

	if (signaled)
	{
		throw EXCEPTION("Unexpected");
	}

	if (sq.size())
	{
		if (!sq.front()->signal())
		{
			throw EXCEPTION("Thread already signaled");
		}

		sq.pop_front();
	}
	else
	{
		signaled++;
	}
}

s32 _sys_lwmutex_create(vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, u32 arg4, u64 name, u32 arg6)
{
	sys_lwmutex.Warning("_sys_lwmutex_create(lwmutex_id=*0x%x, protocol=0x%x, control=*0x%x, arg4=0x%x, name=0x%llx, arg6=0x%x)", lwmutex_id, protocol, control, arg4, name, arg6);

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_RETRY && protocol != SYS_SYNC_PRIORITY)
	{
		sys_lwmutex.Error("_sys_lwmutex_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	if (arg4 != 0x80000001 || arg6)
	{
		throw EXCEPTION("Unknown arguments (arg4=0x%x, arg6=0x%x)", arg4, arg6);
	}

	*lwmutex_id = Emu.GetIdManager().make<lv2_lwmutex_t>(protocol, name);

	return CELL_OK;
}

s32 _sys_lwmutex_destroy(u32 lwmutex_id)
{
	sys_lwmutex.Warning("_sys_lwmutex_destroy(lwmutex_id=0x%x)", lwmutex_id);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_lwmutex_t>(lwmutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (!mutex->sq.empty())
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().remove<lv2_lwmutex_t>(lwmutex_id);

	return CELL_OK;
}

s32 _sys_lwmutex_lock(PPUThread& ppu, u32 lwmutex_id, u64 timeout)
{
	sys_lwmutex.Log("_sys_lwmutex_lock(lwmutex_id=0x%x, timeout=0x%llx)", lwmutex_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_lwmutex_t>(lwmutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex->signaled)
	{
		mutex->signaled--;

		return CELL_OK;
	}

	// add waiter; protocol is ignored in current implementation
	sleep_queue_entry_t waiter(ppu, mutex->sq);

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

s32 _sys_lwmutex_trylock(u32 lwmutex_id)
{
	sys_lwmutex.Log("_sys_lwmutex_trylock(lwmutex_id=0x%x)", lwmutex_id);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_lwmutex_t>(lwmutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (!mutex->sq.empty() || !mutex->signaled)
	{
		return CELL_EBUSY;
	}

	mutex->signaled--;

	return CELL_OK;
}

s32 _sys_lwmutex_unlock(u32 lwmutex_id)
{
	sys_lwmutex.Log("_sys_lwmutex_unlock(lwmutex_id=0x%x)", lwmutex_id);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_lwmutex_t>(lwmutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	mutex->unlock(lv2_lock);

	return CELL_OK;
}
