#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Mutex.h"
#include "Emu/SysCalls/lv2/SC_Condition.h"

SysCallBase sys_cond("sys_cond");

int sys_cond_create(mem32_t cond_id, u32 mutex_id, mem_ptr_t<sys_cond_attribute> attr)
{
	sys_cond.Warning("sys_cond_create(cond_id_addr=0x%x, mutex_id=%d, attr_addr=%d)",
		cond_id.GetAddr(), mutex_id, attr.GetAddr());

	if (!cond_id.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	if (attr->pshared.ToBE() != se32(0x200))
	{
		sys_cond.Error("Invalid pshared attribute(0x%x)", (u32)attr->pshared);
		return CELL_EINVAL;
	}

	mutex* mtx_data;
	if (!Emu.GetIdManager().GetIDData(mutex_id, mtx_data))
	{
		return CELL_ESRCH;
	}

	cond_id = sys_cond.GetNewId(new condition(mtx_data->mtx, attr->name_u64));
	sys_cond.Warning("*** condition created [%s]: id = %d", wxString(attr->name, 8).wx_str(), cond_id.GetValue());

	return CELL_OK;
}

int sys_cond_destroy(u32 cond_id)
{
	sys_cond.Error("sys_cond_destroy(cond_id=%d)", cond_id);

	condition* cond;
	if (!Emu.GetIdManager().GetIDData(cond_id, cond))
	{
		return CELL_ESRCH;
	}

	if (true) // TODO
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(cond_id);
	return CELL_OK;
}

int sys_cond_wait(u32 cond_id, u64 timeout)
{
	sys_cond.Warning("sys_cond_wait(cond_id=%d, timeout=%lld)", cond_id, timeout);

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
	sys_cond.Warning("sys_cond_signal(cond_id=%d)", cond_id);

	condition* cond_data = nullptr;
	if(!sys_cond.CheckId(cond_id, cond_data)) return CELL_ESRCH;

	cond_data->cond.Signal();

	return CELL_OK;
}

int sys_cond_signal_all(u32 cond_id)
{
	sys_cond.Warning("sys_cond_signal_all(cond_id=%d)", cond_id);

	condition* cond_data = nullptr;
	if(!sys_cond.CheckId(cond_id, cond_data)) return CELL_ESRCH;

	cond_data->cond.Broadcast();

	return CELL_OK;
}

int sys_cond_signal_to(u32 cond_id, u32 thread_id)
{
	sys_cond.Error("sys_cond_signal_to(cond_id=%d, thread_id=%d)", cond_id, thread_id);

	return CELL_OK;
}