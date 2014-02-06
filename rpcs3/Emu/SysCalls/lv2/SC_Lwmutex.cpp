#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/lv2/SC_Lwmutex.h"
#include <mutex>

SysCallBase sc_lwmutex("sys_lwmutex");

int sys_lwmutex_create(mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwmutex_attribute_t> attr)
{
	sc_lwmutex.Warning("sys_lwmutex_create(lwmutex_addr=0x%x, lwmutex_attr_addr=0x%x)", 
		lwmutex.GetAddr(), attr.GetAddr());

	if (!lwmutex.IsGood() || !attr.IsGood()) return CELL_EFAULT;

	switch (attr->attr_recursive.ToBE())
	{
	case se32(SYS_SYNC_RECURSIVE): break;
	case se32(SYS_SYNC_NOT_RECURSIVE): break;
	default: return CELL_EINVAL;
	}

	switch (attr->attr_protocol.ToBE())
	{
	case se32(SYS_SYNC_PRIORITY): sc_lwmutex.Warning("TODO: SYS_SYNC_PRIORITY attr"); break;
	case se32(SYS_SYNC_RETRY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sc_lwmutex.Error("Invalid SYS_SYNC_PRIORITY_INHERIT attr"); break;
	case se32(SYS_SYNC_FIFO): sc_lwmutex.Warning("TODO: SYS_SYNC_FIFO attr"); break;
	default: return CELL_EINVAL;
	}

	lwmutex->attribute = (u32)attr->attr_protocol | (u32)attr->attr_recursive;
	lwmutex->all_info = 0;
	lwmutex->pad = 0;
	lwmutex->recursive_count = 0;
	lwmutex->sleep_queue = 0;

	sc_lwmutex.Warning("*** lwmutex created [%s] (attribute=0x%x): id=???", attr->name, (u32)lwmutex->attribute);

	return CELL_OK;
}

int sys_lwmutex_destroy(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Warning("sys_lwmutex_destroy(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	// try to make it unable to lock
	switch (int res = lwmutex->trylock(~0)) 
	{
		case CELL_OK: lwmutex->attribute = 0;
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

