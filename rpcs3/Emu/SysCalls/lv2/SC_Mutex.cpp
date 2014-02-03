#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Mutex.h"

SysCallBase sys_mtx("sys_mutex");

int sys_mutex_create(u32 mutex_id_addr, u32 attr_addr)
{
	sys_mtx.Warning("sys_mutex_create(mutex_id_addr=0x%x, attr_addr=0x%x)",
		mutex_id_addr, attr_addr);

	if(!Memory.IsGoodAddr(mutex_id_addr) || !Memory.IsGoodAddr(attr_addr)) return CELL_EFAULT;

	mutex_attr attr = (mutex_attr&)Memory[attr_addr];
	attr.protocol = re(attr.protocol);
	attr.recursive = re(attr.recursive);
	attr.pshared = re(attr.pshared);
	attr.adaptive = re(attr.adaptive);
	attr.ipc_key = re(attr.ipc_key);
	attr.flags = re(attr.flags);
	
	sys_mtx.Log("*** protocol = %d", attr.protocol);
	sys_mtx.Log("*** recursive = %d", attr.recursive);
	sys_mtx.Log("*** pshared = %d", attr.pshared);
	sys_mtx.Log("*** ipc_key = 0x%llx", attr.ipc_key);
	sys_mtx.Log("*** flags = 0x%x", attr.flags);
	sys_mtx.Log("*** name = %s", attr.name);

	Memory.Write32(mutex_id_addr, sys_mtx.GetNewId(new mutex(attr)));

	return CELL_OK;
}

int sys_mutex_destroy(u32 mutex_id)
{
	sys_mtx.Log("sys_mutex_destroy(mutex_id=0x%x)", mutex_id);

	if(!sys_mtx.CheckId(mutex_id)) return CELL_ESRCH;

	Emu.GetIdManager().RemoveID(mutex_id);
	return CELL_OK;
}

int sys_mutex_lock(u32 mutex_id, u64 timeout)
{
	sys_mtx.Log("sys_mutex_lock(mutex_id=0x%x, timeout=0x%llx)", mutex_id, timeout);

	mutex* mtx_data = nullptr;
	if(!sys_mtx.CheckId(mutex_id, mtx_data)) return CELL_ESRCH;

	u32 counter = 0;
	const u32 max_counter = timeout ? (timeout / 1000) : 20000;
	do
	{
		if (Emu.IsStopped()) return CELL_ETIMEDOUT;

		if (mtx_data->mtx.TryLock() == wxMUTEX_NO_ERROR) return CELL_OK;
		Sleep(1);

		if (counter++ > max_counter)
		{
			if (!timeout) 
			{
				sys_mtx.Warning("sys_mutex_lock(mutex_id=0x%x, timeout=0x%llx): TIMEOUT", mutex_id, timeout);
				counter = 0;
			}
			else
			{
				return CELL_ETIMEDOUT;
			}
		}		
	} while (true);
}

int sys_mutex_trylock(u32 mutex_id)
{
	sys_mtx.Log("sys_mutex_trylock(mutex_id=0x%x)", mutex_id);

	mutex* mtx_data = nullptr;
	if(!sys_mtx.CheckId(mutex_id, mtx_data)) return CELL_ESRCH;

	if (mtx_data->mtx.TryLock() != wxMUTEX_NO_ERROR) return CELL_EBUSY;

	return CELL_OK;
}

int sys_mutex_unlock(u32 mutex_id)
{
	sys_mtx.Log("sys_mutex_unlock(mutex_id=0x%x)", mutex_id);

	mutex* mtx_data = nullptr;
	if(!sys_mtx.CheckId(mutex_id, mtx_data)) return CELL_ESRCH;

	mtx_data->mtx.Unlock();

	return CELL_OK;
}
