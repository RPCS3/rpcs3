#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/lv2/SC_Lwmutex.h"
#include <mutex>

SysCallBase sc_lwmutex("sys_lwmutex");

std::mutex g_lwmutex;

int sys_lwmutex_create(mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwmutex_attribute_t> attr)
{
	sc_lwmutex.Log("sys_lwmutex_create(lwmutex_addr=0x%x, lwmutex_attr_addr=0x%x)", 
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
	case SYS_SYNC_PRIORITY: break;
	case SYS_SYNC_RETRY: sc_lwmutex.Error("TODO: SYS_SYNC_RETRY attr"); break;
	case SYS_SYNC_PRIORITY_INHERIT: sc_lwmutex.Error("TODO: SYS_SYNC_PRIORITY_INHERIT attr"); break;
	case SYS_SYNC_FIFO: sc_lwmutex.Error("TODO: SYS_SYNC_FIFO attr"); break;
	default: return CELL_EINVAL;
	}

	lwmutex->attribute = (u32)attr->attr_protocol | (u32)attr->attr_recursive;
	lwmutex->all_info = 0;
	lwmutex->pad = 0;
	lwmutex->recursive_count = 0;
	lwmutex->sleep_queue = 0;

	sc_lwmutex.Log("*** lwmutex created [%s] (protocol=0x%x, recursive=0x%x)",
		attr->name, (u32)attr->attr_protocol, (u32)attr->attr_recursive);

	return CELL_OK;
}

int sys_lwmutex_destroy(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Warning("sys_lwmutex_destroy(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	return CELL_OK;
}

int sys_lwmutex_lock(mem_ptr_t<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sc_lwmutex.Log("sys_lwmutex_lock(lwmutex_addr=0x%x, timeout=%lld)", lwmutex.GetAddr(), timeout);

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	PPCThread& thr = GetCurrentPPUThread();
	const u32 id = thr.GetId();

	{ // global lock
		std::lock_guard<std::mutex> lock(g_lwmutex);

		if ((u32)lwmutex->attribute & SYS_SYNC_RECURSIVE)
		{
			if (id == (u32)lwmutex->owner)
			{
				lwmutex->recursive_count = lwmutex->recursive_count + 1;
				if (lwmutex->recursive_count == 0xffffffff) return CELL_EKRESOURCE;
				return CELL_OK;
			}
		}
		else // recursive not allowed
		{
			if (id == (u32)lwmutex->owner)
			{
				return CELL_EDEADLK;
			}
		}

		if (!lwmutex->owner) // lock
		{
			lwmutex->owner = id; 
			lwmutex->recursive_count = 1;
			return CELL_OK;
		}
		lwmutex->waiter = id; // not used yet
	}

	u32 counter = 0;
	const u32 max_counter = timeout ? (timeout / 1000) : 20000;
	do // waiting
	{
		Sleep(1);

		{ // global lock
			std::lock_guard<std::mutex> lock(g_lwmutex);

			if (!lwmutex->owner) // lock
			{
				lwmutex->owner = id; 
				lwmutex->recursive_count = 1;
				return CELL_OK;
			}
			lwmutex->waiter = id; // not used yet
		}

		if (counter++ > max_counter)
		{
			if (!timeout) 
			{ // endless waiter
				sc_lwmutex.Error("sys_lwmutex_lock(lwmutex_addr=0x%x): TIMEOUT", lwmutex.GetAddr());
				return CELL_OK;
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

	PPCThread& thr = GetCurrentPPUThread();
	const u32 id = thr.GetId();

	{ // global lock
		std::lock_guard<std::mutex> lock(g_lwmutex);

		if ((u32)lwmutex->attribute & SYS_SYNC_RECURSIVE)
		{
			if (id == (u32)lwmutex->owner)
			{
				lwmutex->recursive_count = lwmutex->recursive_count + 1;
				if (lwmutex->recursive_count == 0xffffffff) return CELL_EKRESOURCE;
				return CELL_OK;
			}
		}
		else // recursive not allowed
		{
			if (id == (u32)lwmutex->owner)
			{
				return CELL_EDEADLK;
			}
		}

		if (!lwmutex->owner) // try lock
		{
			lwmutex->owner = id; 
			lwmutex->recursive_count = 1;
			return CELL_OK;
		}
		else
		{
			return CELL_EBUSY;
		}
	}
}

int sys_lwmutex_unlock(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Log("sys_lwmutex_unlock(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	PPCThread& thr = GetCurrentPPUThread();
	const u32 id = thr.GetId();

	{ // global lock
		std::lock_guard<std::mutex> lock(g_lwmutex);

		if (id != (u32)lwmutex->owner)
		{
			return CELL_EPERM;
		}
		else
		{
			lwmutex->recursive_count = (u32)lwmutex->recursive_count - 1;
			if (!lwmutex->recursive_count)
			{
				lwmutex->waiter = 0; // not used yet
				lwmutex->owner = 0; // release
				/* CPUThread* thr = Emu.GetCPU().GetThread(lwmutex->owner);
				if(thr)
				{
					thr->Wait(false);
				} */
			}
			return CELL_OK;
		}
	}
}

