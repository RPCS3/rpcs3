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
	lwmutex& lmtx = (lwmutex&)Memory[lwmutex_addr];
	lwmutex_attr& lmtx_attr = (lwmutex_attr&)Memory[lwmutex_attr_addr];

	//sc_lwmutex.Warning("sys_lwmutex_create(lwmutex_addr = 0x%llx, lwmutex_attr_addr = 0x%llx)", lwmutex_addr, lwmutex_attr_addr);

	lmtx.lock_var.info.owner = 0;
	lmtx.lock_var.info.waiter = 0;
	
	lmtx.attribute = Emu.GetIdManager().GetNewID(wxString::Format("lwmutex[%s]", lmtx_attr.name), nullptr, lwmutex_addr);
	/*
	ConLog.Write("r3:");
	ConLog.Write("*** lock_var[owner: 0x%x, waiter: 0x%x]", re(lmtx.lock_var.info.owner), re(lmtx.lock_var.info.waiter));
	ConLog.Write("*** attribute: 0x%x", re(lmtx.attribute));
	ConLog.Write("*** recursive_count: 0x%x", re(lmtx.recursive_count));
	ConLog.Write("*** sleep_queue: 0x%x", re(lmtx.sleep_queue));
	ConLog.Write("r4:");
	ConLog.Write("*** attr_protocol: 0x%x", re(lmtx_attr.attr_protocol));
	ConLog.Write("*** attr_recursive: 0x%x", re(lmtx_attr.attr_recursive));
	ConLog.Write("*** name: %s", lmtx_attr.name);
	*/
	return CELL_OK;
}

int sys_lwmutex_destroy(u64 lwmutex_addr)
{
	//sc_lwmutex.Warning("sys_lwmutex_destroy(lwmutex_addr = 0x%llx)", lwmutex_addr);

	lwmutex& lmtx = (lwmutex&)Memory[lwmutex_addr];
	Emu.GetIdManager().RemoveID(lmtx.attribute);

	return CELL_OK;
}

int sys_lwmutex_lock(u64 lwmutex_addr, u64 timeout)
{
	//sc_lwmutex.Warning("sys_lwmutex_lock(lwmutex_addr = 0x%llx, timeout = 0x%llx)", lwmutex_addr, timeout);

	lwmutex& lmtx = (lwmutex&)Memory[lwmutex_addr];

	if(!lmtx.lock_var.info.owner)
	{
		re(lmtx.lock_var.info.owner, GetCurrentPPUThread().GetId());
	}
	else if(!lmtx.lock_var.info.waiter)
	{
		re(lmtx.lock_var.info.waiter, GetCurrentPPUThread().GetId());
		while(re(lmtx.lock_var.info.owner) != GetCurrentPPUThread().GetId()) Sleep(1);
	}
	else
	{
		return -1;
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
	lmtx.lock_var.info.owner = lmtx.lock_var.info.waiter;
	lmtx.lock_var.info.waiter = 0;

	return CELL_OK;
}

