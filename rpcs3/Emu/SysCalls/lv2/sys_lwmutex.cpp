#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sleep_queue.h"
#include "sys_time.h"
#include "sys_lwmutex.h"

SysCallBase sys_lwmutex("sys_lwmutex");

void lwmutex_create(sys_lwmutex_t& lwmutex, bool recursive, u32 protocol, u64 name)
{
	std::shared_ptr<lwmutex_t> lw(new lwmutex_t(protocol, name));

	lwmutex.lock_var.write_relaxed({ lwmutex::free, lwmutex::zero });
	lwmutex.attribute = protocol | (recursive ? SYS_SYNC_RECURSIVE : SYS_SYNC_NOT_RECURSIVE);
	lwmutex.recursive_count = 0;
	lwmutex.sleep_queue = Emu.GetIdManager().GetNewID(lw, TYPE_LWMUTEX);
}

s32 _sys_lwmutex_create(vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, u32 arg4, u64 name, u32 arg6)
{
	throw __FUNCTION__;
}

s32 _sys_lwmutex_destroy(PPUThread& CPU, u32 lwmutex_id)
{
	throw __FUNCTION__;
}

s32 _sys_lwmutex_lock(PPUThread& CPU, u32 lwmutex_id, u64 timeout)
{
	throw __FUNCTION__;
}

s32 _sys_lwmutex_trylock(PPUThread& CPU, u32 lwmutex_id)
{
	throw __FUNCTION__;
}

s32 _sys_lwmutex_unlock(PPUThread& CPU, u32 lwmutex_id)
{
	throw __FUNCTION__;
}
