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
	std::shared_ptr<lwcond_t> lwc(new lwcond_t(name));

	lwcond.lwcond_queue = Emu.GetIdManager().GetNewID(lwc, TYPE_LWCOND);
}

s32 _sys_lwcond_create(vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name, u32 arg5)
{
	sys_lwcond.Warning("_sys_lwcond_create(lwcond_id=*0x%x, lwmutex_id=%d, control=*0x%x, name=0x%llx, arg5=0x%x)", lwcond_id, lwmutex_id, control, name, arg5);

	std::shared_ptr<lwcond_t> lwc(new lwcond_t(name));

	*lwcond_id = Emu.GetIdManager().GetNewID(lwc, TYPE_LWCOND);

	return CELL_OK;
}

s32 _sys_lwcond_destroy(u32 lwcond_id)
{
	sys_lwcond.Warning("_sys_lwcond_destroy(lwcond_id=%d)", lwcond_id);

	LV2_LOCK;

	std::shared_ptr<lwcond_t> lwc;
	if (!Emu.GetIdManager().GetIDData(lwcond_id, lwc))
	{
		return CELL_ESRCH;
	}

	if (lwc->waiters)
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(lwcond_id);

	return CELL_OK;
}

s32 _sys_lwcond_signal(u32 lwcond_id, u32 lwmutex_id, u32 ppu_thread_id, u32 mode)
{
	sys_lwcond.Fatal("_sys_lwcond_signal(lwcond_id=%d, lwmutex_id=%d, ppu_thread_id=%d, mode=%d)", lwcond_id, lwmutex_id, ppu_thread_id, mode);

	return CELL_OK;
}

s32 _sys_lwcond_signal_all(u32 lwcond_id, u32 lwmutex_id, u32 mode)
{
	sys_lwcond.Fatal("_sys_lwcond_signal_all(lwcond_id=%d, lwmutex_id=%d, mode=%d)", lwcond_id, lwmutex_id, mode);

	return CELL_OK;
}

s32 _sys_lwcond_queue_wait(u32 lwcond_id, u32 lwmutex_id, u64 timeout)
{
	sys_lwcond.Fatal("_sys_lwcond_queue_wait(lwcond_id=%d, lwmutex_id=%d, timeout=0x%llx)", lwcond_id, lwmutex_id, timeout);

	return CELL_OK;
}
