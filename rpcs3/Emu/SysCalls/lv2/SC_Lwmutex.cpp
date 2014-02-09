#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/lv2/SC_Lwmutex.h"
#include <mutex>

SysCallBase sc_lwmutex("sys_lwmutex");

int sys_lwmutex_create(mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwmutex_attribute_t> attr)
{
	sc_lwmutex.Log("sys_lwmutex_create(lwmutex_addr=0x%x, lwmutex_attr_addr=0x%x)", 
		lwmutex.GetAddr(), attr.GetAddr());

	if (!lwmutex.IsGood() || !attr.IsGood()) return CELL_EFAULT;

	switch (attr->attr_recursive.ToBE())
	{
	case se32(SYS_SYNC_RECURSIVE): break;
	case se32(SYS_SYNC_NOT_RECURSIVE): break;
	default: sc_lwmutex.Error("Unknown 0x%x recursive attr", (u32)attr->attr_recursive); return CELL_EINVAL;
	}

	switch (attr->attr_protocol.ToBE())
	{
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_RETRY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sc_lwmutex.Error("Invalid SYS_SYNC_PRIORITY_INHERIT protocol attr"); return CELL_EINVAL;
	case se32(SYS_SYNC_FIFO): break;
	default: sc_lwmutex.Error("Unknown 0x%x protocol attr", (u32)attr->attr_protocol); return CELL_EINVAL;
	}

	lwmutex->attribute = attr->attr_protocol | attr->attr_recursive;
	lwmutex->all_info = 0;
	lwmutex->pad = 0;
	lwmutex->recursive_count = 0;

	u32 sq_id = sc_lwmutex.GetNewId(new SleepQueue(attr->name_u64));
	lwmutex->sleep_queue = sq_id;

	sc_lwmutex.Log("*** lwmutex created [%s] (attribute=0x%x): sleep_queue=0x%x", 
		attr->name, (u32)lwmutex->attribute, sq_id);

	return CELL_OK;
}

int sys_lwmutex_destroy(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Log("sys_lwmutex_destroy(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	u32 sq_id = lwmutex->sleep_queue;
	if (!Emu.GetIdManager().CheckID(sq_id)) return CELL_ESRCH;

	// try to make it unable to lock
	switch (int res = lwmutex->trylock(~0)) 
	{
	case CELL_OK:
		lwmutex->attribute = 0;
		lwmutex->sleep_queue = 0;
		Emu.GetIdManager().RemoveID(sq_id);
	default: return res;
	}
}

int sys_lwmutex_lock(mem_ptr_t<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sc_lwmutex.Log("sys_lwmutex_lock(lwmutex_addr=0x%x, timeout=%lld)", lwmutex.GetAddr(), timeout);

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	return lwmutex->lock(GetCurrentPPUThread().GetId(), timeout ? ((timeout < 1000) ? 1 : (timeout / 1000)) : 0);
}

int sys_lwmutex_trylock(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Log("sys_lwmutex_trylock(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	return lwmutex->trylock(GetCurrentPPUThread().GetId());
}

int sys_lwmutex_unlock(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Log("sys_lwmutex_unlock(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	return lwmutex->unlock(GetCurrentPPUThread().GetId());
}

