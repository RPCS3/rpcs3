#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

struct lwmutex_lock_info
{
	u32 owner;
	u32 waiter;
};

union lwmutex_variable
{
	lwmutex_lock_info info;
	u64 all_info;
};

struct lwmutex
{
	lwmutex_variable lock_var;
	u32 attribute;
	u32 recursive_count;
	u32 sleep_queue;
	u32 pad;
};

struct lwmutex_attr
{
	u32 attr_protocol;
	u32 attr_recursive;
	char name[8];
};

SysCallBase sc_lwmutex("sys_wmutex");

int sys_lwmutex_create(u64 lwmutex_addr, u64 lwmutex_attr_addr)
{
	if(!Memory.IsGoodAddr(lwmutex_addr, 4) || !Memory.IsGoodAddr(lwmutex_attr_addr))
	{
		return CELL_EFAULT;
	}

	lwmutex& lmtx = (lwmutex&)Memory[lwmutex_addr];
	lmtx.lock_var.all_info = 0;
	lwmutex_attr& lmtx_attr = (lwmutex_attr&)Memory[lwmutex_attr_addr];

	return CELL_OK;
}

int sys_lwmutex_destroy(u64 lwmutex_addr)
{
	//sc_lwmutex.Warning("sys_lwmutex_destroy(lwmutex_addr = 0x%llx)", lwmutex_addr);

	//lwmutex& lmtx = (lwmutex&)Memory[lwmutex_addr];
	//Emu.GetIdManager().RemoveID(lmtx.attribute);

	return CELL_OK;
}

int sys_lwmutex_lock(u64 lwmutex_addr, u64 timeout)
{
	lwmutex& lmtx = (lwmutex&)Memory[lwmutex_addr];

	PPCThread& thr = GetCurrentPPUThread();

	if(thr.GetId() == re(lmtx.lock_var.info.owner))
	{
		re(lmtx.recursive_count, re(lmtx.recursive_count) + 1);
		return CELL_OK;
	}

	if(!lmtx.lock_var.info.owner)
	{
		re(lmtx.lock_var.info.owner, GetCurrentPPUThread().GetId());
		re(lmtx.recursive_count, 1);
	}
	else if(!lmtx.lock_var.info.waiter)
	{
		thr.Wait(true);
		re(lmtx.lock_var.info.waiter, thr.GetId());
	}
	else
	{
		ConLog.Warning("lwmutex has waiter!");
		return CELL_EBUSY;
	}

	return CELL_OK;
}

int sys_lwmutex_trylock(u64 lwmutex_addr)
{
	//sc_lwmutex.Warning("sys_lwmutex_trylock(lwmutex_addr = 0x%llx)", lwmutex_addr);
	lwmutex& lmtx = (lwmutex&)Memory[lwmutex_addr];
	
	if(lmtx.lock_var.info.owner) return CELL_EBUSY;

	return CELL_OK;
}

int sys_lwmutex_unlock(u64 lwmutex_addr)
{
	//sc_lwmutex.Warning("sys_lwmutex_unlock(lwmutex_addr = 0x%llx)", lwmutex_addr);

	lwmutex& lmtx = (lwmutex&)Memory[lwmutex_addr];

	re(lmtx.recursive_count, re(lmtx.recursive_count) - 1);

	if(!lmtx.recursive_count)
	{
		if(lmtx.lock_var.info.owner = lmtx.lock_var.info.waiter)
		{
			lmtx.lock_var.info.waiter = 0;
			PPCThread* thr = Emu.GetCPU().GetThread(lmtx.lock_var.info.owner);
			if(thr)
			{
				thr->Wait(false);
			}
		}
	}

	return CELL_OK;
}

