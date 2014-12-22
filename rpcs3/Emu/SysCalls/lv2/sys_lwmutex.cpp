#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/Memory/atomic_type.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sleep_queue_type.h"
#include "sys_time.h"
#include "sys_lwmutex.h"

SysCallBase sys_lwmutex("sys_lwmutex");

s32 lwmutex_create(sys_lwmutex_t& lwmutex, u32 protocol, u32 recursive, u64 name_u64)
{
	LV2_LOCK(0);

	lwmutex.owner.write_relaxed(be_t<u32>::make(0));
	lwmutex.waiter.write_relaxed(be_t<u32>::make(~0));
	lwmutex.attribute = protocol | recursive;
	lwmutex.recursive_count = 0;
	u32 sq_id = sys_lwmutex.GetNewId(new sleep_queue_t(name_u64), TYPE_LWMUTEX);
	lwmutex.sleep_queue = sq_id;

	std::string name((const char*)&name_u64, 8);
	sys_lwmutex.Notice("*** lwmutex created [%s] (attribute=0x%x): sq_id = %d", name.c_str(), protocol | recursive, sq_id);

	Emu.GetSyncPrimManager().AddLwMutexData(sq_id, name, 0);
	
	return CELL_OK;
}

s32 sys_lwmutex_create(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr)
{
	sys_lwmutex.Warning("sys_lwmutex_create(lwmutex_addr=0x%x, attr_addr=0x%x)", lwmutex.addr(), attr.addr());

	switch (attr->recursive.ToBE())
	{
	case se32(SYS_SYNC_RECURSIVE): break;
	case se32(SYS_SYNC_NOT_RECURSIVE): break;
	default: sys_lwmutex.Error("Unknown recursive attribute(0x%x)", (u32)attr->recursive); return CELL_EINVAL;
	}

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_RETRY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_lwmutex.Error("Invalid SYS_SYNC_PRIORITY_INHERIT protocol attr"); return CELL_EINVAL;
	case se32(SYS_SYNC_FIFO): break;
	default: sys_lwmutex.Error("Unknown protocol attribute(0x%x)", (u32)attr->protocol); return CELL_EINVAL;
	}

	return lwmutex_create(*lwmutex, attr->protocol, attr->recursive, attr->name_u64);
}

s32 sys_lwmutex_destroy(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sys_lwmutex.Warning("sys_lwmutex_destroy(lwmutex_addr=0x%x)", lwmutex.addr());

	LV2_LOCK(0);

	u32 sq_id = lwmutex->sleep_queue;
	if (!Emu.GetIdManager().CheckID(sq_id)) return CELL_ESRCH;

	// try to make it unable to lock
	switch (int res = lwmutex->trylock(be_t<u32>::make(~0)))
	{
	case CELL_OK:
		lwmutex->all_info() = 0;
		lwmutex->attribute = 0xDEADBEEF;
		lwmutex->sleep_queue = 0;
		Emu.GetIdManager().RemoveID(sq_id);
		Emu.GetSyncPrimManager().EraseLwMutexData(sq_id);
	default: return res;
	}
}

s32 sys_lwmutex_lock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sys_lwmutex.Log("sys_lwmutex_lock(lwmutex_addr=0x%x, timeout=%lld)", lwmutex.addr(), timeout);

	return lwmutex->lock(be_t<u32>::make(CPU.GetId()), timeout);
}

s32 sys_lwmutex_trylock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sys_lwmutex.Log("sys_lwmutex_trylock(lwmutex_addr=0x%x)", lwmutex.addr());

	return lwmutex->trylock(be_t<u32>::make(CPU.GetId()));
}

s32 sys_lwmutex_unlock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sys_lwmutex.Log("sys_lwmutex_unlock(lwmutex_addr=0x%x)", lwmutex.addr());

	return lwmutex->unlock(be_t<u32>::make(CPU.GetId()));
}

s32 sys_lwmutex_t::trylock(be_t<u32> tid)
{
	if (attribute.ToBE() == se32(0xDEADBEEF)) return CELL_EINVAL;

	const be_t<u32> old_owner = owner.read_sync();

	if (old_owner == tid)
	{
		if (attribute.ToBE() & se32(SYS_SYNC_RECURSIVE))
		{
			recursive_count += 1;
			if (!recursive_count.ToBE())
			{
				return CELL_EKRESOURCE;
			}
			return CELL_OK;
		}
		else
		{
			return CELL_EDEADLK;
		}
	}

	if (!owner.compare_and_swap_test(be_t<u32>::make(0), tid))
	{
		return CELL_EBUSY;
	}
	
	recursive_count = 1;
	return CELL_OK;
}

s32 sys_lwmutex_t::unlock(be_t<u32> tid)
{
	if (owner.read_sync() != tid)
	{
		return CELL_EPERM;
	}

	if (!recursive_count || (recursive_count.ToBE() != se32(1) && (attribute.ToBE() & se32(SYS_SYNC_NOT_RECURSIVE))))
	{
		sys_lwmutex.Error("sys_lwmutex_t::unlock(%d): wrong recursive value fixed (%d)", (u32)sleep_queue, (u32)recursive_count);
		recursive_count = 1;
	}

	recursive_count -= 1;
	if (!recursive_count.ToBE())
	{
		sleep_queue_t* sq;
		if (!Emu.GetIdManager().GetIDData(sleep_queue, sq))
		{
			return CELL_ESRCH;
		}

		if (!owner.compare_and_swap_test(tid, be_t<u32>::make(sq->pop(attribute))))
		{
			assert(!"sys_lwmutex_t::unlock() failed");
		}
	}

	return CELL_OK;
}

s32 sys_lwmutex_t::lock(be_t<u32> tid, u64 timeout)
{
	switch (s32 res = trylock(tid))
	{
	case static_cast<s32>(CELL_EBUSY): break;
	default: return res;
	}

	sleep_queue_t* sq;
	if (!Emu.GetIdManager().GetIDData(sleep_queue, sq))
	{
		return CELL_ESRCH;
	}

	sq->push(tid, attribute);

	const u64 time_start = get_system_time();
	while (true)
	{
		auto old_owner = owner.compare_and_swap(be_t<u32>::make(0), tid);
		if (!old_owner.ToBE())
		{
			sq->invalidate(tid);
			break;
		}
		if (old_owner == tid)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (timeout && get_system_time() - time_start > timeout)
		{
			sq->invalidate(tid);
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_lwmutex.Warning("sys_lwmutex_t::lock(sq=%d) aborted", (u32)sleep_queue);
			return CELL_OK;
		}
	}

	recursive_count = 1;
	return CELL_OK;
}
