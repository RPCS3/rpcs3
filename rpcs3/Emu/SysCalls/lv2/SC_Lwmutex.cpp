#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/lv2/SC_Lwmutex.h"
#include <mutex>

SysCallBase sc_lwmutex("sys_lwmutex");

std::mutex g_lwmutex;

int sys_lwmutex_create(mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwmutex_attribute_t> attr)
{
	sc_lwmutex.Warning("sys_lwmutex_create(lwmutex_addr=0x%x, lwmutex_attr_addr=0x%x)", 
		lwmutex.GetAddr(), attr.GetAddr());

	if (!lwmutex.IsGood() || !attr.IsGood()) return CELL_EFAULT;

	switch ((u32)attr->attr_recursive)
	{
	case SYS_SYNC_RECURSIVE: break;
	case SYS_SYNC_NOT_RECURSIVE: break;
	default: return CELL_EINVAL;
	}

	switch ((u32)attr->attr_protocol)
	{
	case SYS_SYNC_PRIORITY: sc_lwmutex.Warning("TODO: SYS_SYNC_PRIORITY attr"); break;
	case SYS_SYNC_RETRY: sc_lwmutex.Warning("TODO: SYS_SYNC_RETRY attr"); break;
	case SYS_SYNC_PRIORITY_INHERIT: sc_lwmutex.Warning("TODO: SYS_SYNC_PRIORITY_INHERIT attr"); break;
	case SYS_SYNC_FIFO: sc_lwmutex.Warning("TODO: SYS_SYNC_FIFO attr"); break;
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

	if (!lwmutex->attribute) return CELL_EINVAL;

	{ // global lock
		std::lock_guard<std::mutex> lock(g_lwmutex);

		if (!lwmutex->owner)
		{
			lwmutex->owner = ~0; // make it unable to lock
			lwmutex->attribute = 0;
		}
		else
		{
			return CELL_EBUSY;
		}
	}

	return CELL_OK;
}

int sys_lwmutex_lock(mem_ptr_t<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sc_lwmutex.Log("sys_lwmutex_lock(lwmutex_addr=0x%x, timeout=%lld)", lwmutex.GetAddr(), timeout);

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	if (!lwmutex->attribute) return CELL_EINVAL;

	const u32 tid = GetCurrentPPUThread().GetId();

	int res = lwmutex->trylock(tid);
	if (res != CELL_EBUSY) return res;

	u32 counter = 0;
	const u32 max_counter = timeout ? (timeout / 1000) : 20000;
	do // waiting
	{
		if (Emu.IsStopped()) return CELL_ETIMEDOUT;
		Sleep(1);

		res = lwmutex->trylock(tid);
		if (res != CELL_EBUSY) return res;
		if (!lwmutex->attribute) return CELL_EINVAL;

		if (counter++ > max_counter)
		{
			if (!timeout) 
			{
				sc_lwmutex.Warning("sys_lwmutex_lock(lwmutex_addr=0x%x): TIMEOUT", lwmutex.GetAddr());
				counter = 0;
			}
			else
			{
				return CELL_ETIMEDOUT;
			}
		}
	} while (true);
}

int sys_lwmutex_trylock(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Log("sys_lwmutex_trylock(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	if (!lwmutex->attribute) return CELL_EINVAL;

	return lwmutex->trylock(GetCurrentPPUThread().GetId());
}

int sys_lwmutex_unlock(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Log("sys_lwmutex_unlock(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	if (!lwmutex->unlock(GetCurrentPPUThread().GetId())) return CELL_EPERM;

	return CELL_OK;
}

