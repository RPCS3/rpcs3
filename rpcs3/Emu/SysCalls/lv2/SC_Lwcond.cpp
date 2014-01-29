#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_Lwmutex.h"
#include "SC_Lwcond.h"

SysCallBase sys_lwcond("sys_lwcond");

int sys_lwcond_create(mem_ptr_t<sys_lwcond_t> lwcond, mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwcond_attribute_t> attr)
{
	sys_lwcond.Warning("sys_lwcond_create(lwcond_addr=0x%x, lwmutex_addr=0x%x, attr_addr=0x%x)",
		lwcond.GetAddr(), lwmutex.GetAddr(), attr.GetAddr());

	if (!lwcond.IsGood() || !lwmutex.IsGood() || !attr.IsGood()) return CELL_EFAULT;

	lwcond->lwmutex_addr = lwmutex.GetAddr();
	lwcond->lwcond_queue = sys_lwcond.GetNewId(new LWCond(*(u64*)&attr->name));

	sys_lwcond.Warning("*** lwcond created [%s]", attr->name);
	return CELL_OK;
}

int sys_lwcond_destroy(mem_ptr_t<sys_lwcond_t> lwcond)
{
	sys_lwcond.Warning("sys_lwcond_destroy(lwcond_addr=0x%x)", lwcond.GetAddr());

	if (!lwcond.IsGood()) return CELL_EFAULT;
	LWCond* lwc;
	u32 id = (u32)lwcond->lwcond_queue;
	if (!sys_lwcond.CheckId(id, lwc)) return CELL_ESRCH;

	Emu.GetIdManager().RemoveID(id);
	return CELL_OK;
}

int sys_lwcond_signal(mem_ptr_t<sys_lwcond_t> lwcond)
{
	sys_lwcond.Warning("sys_lwcond_signal(lwcond_addr=0x%x)", lwcond.GetAddr());

	if (!lwcond.IsGood()) return CELL_EFAULT;
	LWCond* lwc;
	u32 id = (u32)lwcond->lwcond_queue;
	if (!sys_lwcond.CheckId(id, lwc)) return CELL_ESRCH;

	return CELL_OK;
}

int sys_lwcond_signal_all(mem_ptr_t<sys_lwcond_t> lwcond)
{
	sys_lwcond.Warning("sys_lwcond_signal_all(lwcond_addr=0x%x)", lwcond.GetAddr());

	if (!lwcond.IsGood()) return CELL_EFAULT;
	LWCond* lwc;
	u32 id = (u32)lwcond->lwcond_queue;
	if (!sys_lwcond.CheckId(id, lwc)) return CELL_ESRCH;

	return CELL_OK;
}

int sys_lwcond_signal_to(mem_ptr_t<sys_lwcond_t> lwcond, u32 ppu_thread_id)
{
	sys_lwcond.Warning("sys_lwcond_signal_to(lwcond_addr=0x%x, ppu_thread_id=%d)", lwcond.GetAddr(), ppu_thread_id);

	if (!lwcond.IsGood()) return CELL_EFAULT;
	LWCond* lwc;
	u32 id = (u32)lwcond->lwcond_queue;
	if (!sys_lwcond.CheckId(id, lwc)) return CELL_ESRCH;

	return CELL_OK;
}

int sys_lwcond_wait(mem_ptr_t<sys_lwcond_t> lwcond, u64 timeout)
{
	sys_lwcond.Warning("sys_lwcond_wait(lwcond_addr=0x%x, timeout=%llu)", lwcond.GetAddr(), timeout);

	if (!lwcond.IsGood()) return CELL_EFAULT;
	LWCond* lwc;
	u32 id = (u32)lwcond->lwcond_queue;
	if (!sys_lwcond.CheckId(id, lwc)) return CELL_ESRCH;

	return CELL_OK;
}
