#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/lv2/SC_Event_flag.h"

SysCallBase sys_event_flag("sys_event_flag");

int sys_event_flag_create(mem32_t eflag_id, mem_ptr_t<sys_event_flag_attr> attr, u64 init)
{
	sys_event_flag.Warning("sys_event_flag_create(eflag_id_addr=0x%x, attr_addr=0x%x, init=0x%llx)", eflag_id.GetAddr(), attr.GetAddr(), init);

	if(!eflag_id.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_PRIORITY): sys_event_flag.Warning("TODO: SYS_SYNC_PRIORITY attr"); break;
	case se32(SYS_SYNC_RETRY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_event_flag.Warning("TODO: SYS_SYNC_PRIORITY_INHERIT attr"); break;
	case se32(SYS_SYNC_FIFO): sys_event_flag.Warning("TODO: SYS_SYNC_FIFO attr"); break;
	default: return CELL_EINVAL;
	}

	if (attr->pshared.ToBE() != se32(0x200))
	{
		return CELL_EINVAL;
	}

	switch (attr->type.ToBE())
	{
	case se32(SYS_SYNC_WAITER_SINGLE): sys_event_flag.Warning("TODO: SYS_SYNC_WAITER_SINGLE type"); break;
	case se32(SYS_SYNC_WAITER_MULTIPLE): sys_event_flag.Warning("TODO: SYS_SYNC_WAITER_MULTIPLE type"); break;
	default: return CELL_EINVAL;
	}

	eflag_id = sys_event_flag.GetNewId(new event_flag(init, (u32)attr->protocol, (int)attr->type));

	sys_event_flag.Warning("*** event_flag created[%s] (protocol=%d, type=%d): id = %d", attr->name, (u32)attr->protocol, (int)attr->type, eflag_id.GetValue());

	return CELL_OK;
}

int sys_event_flag_destroy(u32 eflag_id)
{
	sys_event_flag.Warning("sys_event_flag_destroy(eflag_id=0x%x)", eflag_id);

	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	Emu.GetIdManager().RemoveID(eflag_id);

	return CELL_OK;
}

int sys_event_flag_wait(u32 eflag_id, u64 bitptn, u32 mode, mem64_t result, u32 timeout)
{
	sys_event_flag.Error("sys_event_flag_wait(eflag_id=0x%x, bitptn=0x%llx, mode=0x%x, result_addr=0x%x, timeout=0x%x)",
		eflag_id, bitptn, mode, result.GetAddr(), timeout);
	return CELL_OK;
}

int sys_event_flag_trywait(u32 eflag_id, u64 bitptn, u32 mode, mem64_t result)
{
	sys_event_flag.Error("sys_event_flag_trywait(eflag_id=0x%x, bitptn=0x%llx, mode=0x%x, result_addr=0x%x)",
		eflag_id, bitptn, mode, result.GetAddr());
	return CELL_OK;
}

int sys_event_flag_set(u32 eflag_id, u64 bitptn)
{
	sys_event_flag.Warning("sys_event_flag_set(eflag_id=0x%x, bitptn=0x%llx)", eflag_id, bitptn);

	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	ef->flags |= bitptn;

	return CELL_OK;
}

int sys_event_flag_clear(u32 eflag_id, u64 bitptn)
{
	sys_event_flag.Warning("sys_event_flag_clear(eflag_id=0x%x, bitptn=0x%llx)", eflag_id, bitptn);

	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	ef->flags &= bitptn;

	return CELL_OK;
}

int sys_event_flag_cancel(u32 eflag_id, mem32_t num)
{
	sys_event_flag.Error("sys_event_flag_cancel(eflag_id=0x%x, num_addr=0x%x)", eflag_id, num.GetAddr());
	return CELL_OK;
}

int sys_event_flag_get(u32 eflag_id, mem64_t flags)
{
	sys_event_flag.Warning("sys_event_flag_get(eflag_id=0x%x, flags_addr=0x%x)", eflag_id, flags.GetAddr());
	
	if (!flags.IsGood())
	{
		return CELL_EFAULT;
	}
	
	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	flags = ef->flags;

	return CELL_OK;
}