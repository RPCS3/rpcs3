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

SysCallBase sc_lwmutex("Lwmutex");

//TODO
int SysCalls::Lv2LwmutexCreate(PPUThread& CPU)
{
	/*
	//int sys_lwmutex_create(sys_lwmutex_t *lwmutex, sys_lwmutex_attribute_t *attr)
	const u64 lwmutex_addr = CPU.GPR[3];
	const u64 lwmutex_attr_addr = CPU.GPR[4];

	//ConLog.Write("Lv2LwmutexCreate[r3: 0x%llx, r4: 0x%llx]", lwmutex_addr, lwmutex_attr_addr);

	lwmutex& lmtx = *(lwmutex*)Memory.GetMemFromAddr(lwmutex_addr);
	lwmutex_attr& lmtx_attr = *(lwmutex_attr*)Memory.GetMemFromAddr(lwmutex_attr_addr);

	lmtx.lock_var.info.owner = CPU.GetId();
	lmtx.attribute = Emu.GetIdManager().GetNewID(wxString::Format("Lwmutex[%s]", lmtx_attr.name), NULL, lwmutex_addr);
	*/
	/*
	ConLog.Write("r3:");
	ConLog.Write("*** lock_var[owner: 0x%x, waiter: 0x%x]", lmtx.lock_var.info.owner, lmtx.lock_var.info.waiter);
	ConLog.Write("*** attribute: 0x%x", lmtx.attribute);
	ConLog.Write("*** recursive_count: 0x%x", lmtx.recursive_count);
	ConLog.Write("*** sleep_queue: 0x%x", lmtx.sleep_queue);
	ConLog.Write("r4:");
	ConLog.Write("*** attr_protocol: 0x%x", lmtx_attr.attr_protocol);
	ConLog.Write("*** attr_recursive: 0x%x", lmtx_attr.attr_recursive);
	ConLog.Write("*** name: %s", lmtx_attr.name);
	*/

	return 0;
}

int SysCalls::Lv2LwmutexDestroy(PPUThread& CPU)
{
	/*
	const u64 lwmutex_addr = CPU.GPR[3];
	//ConLog.Write("Lv2LwmutexDestroy[r3: 0x%llx]", lwmutex_addr);

	lwmutex& lmtx = *(lwmutex*)Memory.GetMemFromAddr(lwmutex_addr);
	Emu.GetIdManager().RemoveID(lmtx.attribute);
	//memset(Memory.GetMemFromAddr(lwmutex_addr), 0, sizeof(lwmutex));
	*/
	return 0;
}

int SysCalls::Lv2LwmutexLock(PPUThread& CPU)
{
	//int sys_lwmutex_lock(sys_lwmutex_t *lwmutex, usecond_t timeout)
	const u64 lwmutex_addr = CPU.GPR[3];
	const u64 timeout = CPU.GPR[4];
	//ConLog.Write("Lv2LwmutexLock[r3: 0x%llx, r4: 0x%llx]", lwmutex_addr, timeout);

	//lwmutex& lmtx = *(lwmutex*)Memory.GetMemFromAddr(lwmutex_addr);
	//lmtx.lock_var.info.waiter = CPU.GetId();

	return 0;//CELL_ESRCH;
}

int SysCalls::Lv2LwmutexTrylock(PPUThread& CPU)
{
	//int sys_lwmutex_trylock(sys_lwmutex_t *lwmutex)
	const u64 lwmutex_addr = CPU.GPR[3];
	//ConLog.Write("Lv2LwmutexTrylock[r3: 0x%llx]", lwmutex_addr);
	return 0;
}

int SysCalls::Lv2LwmutexUnlock(PPUThread& CPU)
{
	//int sys_lwmutex_unlock(sys_lwmutex_t *lwmutex)
	const u64 lwmutex_addr = CPU.GPR[3];
	//ConLog.Write("Lv2LwmutexUnlock[r3: 0x%llx]", lwmutex_addr);
	return 0;
}
