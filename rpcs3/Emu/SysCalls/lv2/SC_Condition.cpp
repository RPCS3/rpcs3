#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Mutex.h"

SysCallBase sys_cond("sys_cond");
extern SysCallBase sys_mtx;

struct condition_attr
{
	u32 pshared;
	int flags;
	u64 ipc_key;
	char name[8];
};

struct condition
{
	wxCondition cond;
	condition_attr attr;

	condition(wxMutex& mtx, const condition_attr& attr)
		: cond(mtx)
		, attr(attr)
	{
	}
};

int sys_cond_create(u32 cond_addr, u32 mutex_id, u32 attr_addr)
{
	sys_cond.Warning("sys_cond_create(cond_addr=0x%x, mutex_id=0x%x, attr_addr=%d)",
		cond_addr, mutex_id, attr_addr);

	if(!Memory.IsGoodAddr(cond_addr) || !Memory.IsGoodAddr(attr_addr)) return CELL_EFAULT;

	condition_attr attr = (condition_attr&)Memory[attr_addr];

	attr.pshared = re(attr.pshared);
	attr.ipc_key = re(attr.ipc_key);
	attr.flags = re(attr.flags);
	
	sys_cond.Log("*** pshared = %d", attr.pshared);
	sys_cond.Log("*** ipc_key = 0x%llx", attr.ipc_key);
	sys_cond.Log("*** flags = 0x%x", attr.flags);
	sys_cond.Log("*** name = %s", attr.name);

	mutex* mtx_data = nullptr;
	if(!sys_mtx.CheckId(mutex_id, mtx_data)) return CELL_ESRCH;

	Memory.Write32(cond_addr, sys_cond.GetNewId(new condition(mtx_data->mtx, attr)));

	return CELL_OK;
}

int sys_cond_destroy(u32 cond_id)
{
	sys_cond.Log("sys_cond_destroy(cond_id=0x%x)", cond_id);

	if(!sys_cond.CheckId(cond_id)) return CELL_ESRCH;

	Emu.GetIdManager().RemoveID(cond_id);
	return CELL_OK;
}

int sys_cond_wait(u32 cond_id, u64 timeout)
{
	sys_cond.Log("sys_cond_wait(cond_id=0x%x, timeout=0x%llx)", cond_id, timeout);

	condition* cond_data = nullptr;
	if(!sys_cond.CheckId(cond_id, cond_data)) return CELL_ESRCH;

	u32 counter = 0;
	const u32 max_counter = timeout ? (timeout / 1000) : 20000;
	do
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("sys_cond_wait(cond_id=%d, ...) aborted", cond_id);
			return CELL_ETIMEDOUT;
		}

		switch (cond_data->cond.WaitTimeout(1))
		{
		case wxCOND_NO_ERROR: return CELL_OK;
		case wxCOND_TIMEOUT: break;
		default: return CELL_EPERM;
		}

		if (counter++ > max_counter)
		{
			if (!timeout) 
			{
				counter = 0;
			}
			else
			{
				return CELL_ETIMEDOUT;
			}
		}		
	} while (true);
}

int sys_cond_signal(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal(cond_id=0x%x)", cond_id);

	condition* cond_data = nullptr;
	if(!sys_cond.CheckId(cond_id, cond_data)) return CELL_ESRCH;

	cond_data->cond.Signal();

	return CELL_OK;
}

int sys_cond_signal_all(u32 cond_id)
{
	sys_cond.Log("sys_cond_signal_all(cond_id=0x%x)", cond_id);

	condition* cond_data = nullptr;
	if(!sys_cond.CheckId(cond_id, cond_data)) return CELL_ESRCH;

	cond_data->cond.Broadcast();

	return CELL_OK;
}