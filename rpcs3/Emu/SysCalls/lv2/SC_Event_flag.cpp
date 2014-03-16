#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/lv2/SC_Event_flag.h"

SysCallBase sys_event_flag("sys_event_flag");

int sys_event_flag_create(mem32_t eflag_id, mem_ptr_t<sys_event_flag_attr> attr, u64 init)
{
	sys_event_flag.Log("sys_event_flag_create(eflag_id_addr=0x%x, attr_addr=0x%x, init=0x%llx)",
		eflag_id.GetAddr(), attr.GetAddr(), init);

	if(!eflag_id.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_PRIORITY): sys_event_flag.Warning("TODO: SYS_SYNC_PRIORITY attr"); break;
	case se32(SYS_SYNC_RETRY): sys_event_flag.Warning("TODO: SYS_SYNC_RETRY attr"); break;
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
	case se32(SYS_SYNC_WAITER_SINGLE): break;
	case se32(SYS_SYNC_WAITER_MULTIPLE): sys_event_flag.Error("TODO: SYS_SYNC_WAITER_MULTIPLE type"); break;
	default: return CELL_EINVAL;
	}

	eflag_id = sys_event_flag.GetNewId(new event_flag(init, (u32)attr->protocol, (int)attr->type));

	sys_event_flag.Warning("*** event_flag created [%s] (protocol=0x%x, type=0x%x): id = %d",
		wxString(attr->name, 8).wx_str(), (u32)attr->protocol, (int)attr->type, eflag_id.GetValue());

	return CELL_OK;
}

int sys_event_flag_destroy(u32 eflag_id)
{
	sys_event_flag.Warning("sys_event_flag_destroy(eflag_id=%d)", eflag_id);

	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	Emu.GetIdManager().RemoveID(eflag_id);

	return CELL_OK;
}

int sys_event_flag_wait(u32 eflag_id, u64 bitptn, u32 mode, mem64_t result, u64 timeout)
{
	sys_event_flag.Warning("sys_event_flag_wait(eflag_id=%d, bitptn=0x%llx, mode=0x%x, result_addr=0x%x, timeout=%lld)",
		eflag_id, bitptn, mode, result.GetAddr(), timeout);

	if (result.IsGood()) result = 0;

	switch (mode & 0xf)
	{
	case SYS_EVENT_FLAG_WAIT_AND: break;
	case SYS_EVENT_FLAG_WAIT_OR: break;
	default: return CELL_EINVAL;
	}

	switch (mode & ~0xf)
	{
	case 0: break; // ???
	case SYS_EVENT_FLAG_WAIT_CLEAR: break;
	case SYS_EVENT_FLAG_WAIT_CLEAR_ALL: break;
	default: return CELL_EINVAL;
	}

	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	u32 tid = GetCurrentPPUThread().GetId();

	ef->waiters.push(tid);
	if (ef->m_type == SYS_SYNC_WAITER_SINGLE && ef->waiters.list.GetCount() > 1)
	{
		ef->waiters.invalidate(tid);
		return CELL_EPERM;
	}

	u32 counter = 0;
	const u32 max_counter = timeout ? (timeout / 1000) : ~0;

	while (true)
	{
		{
			SMutexLocker lock(ef->m_mutex);

			u64 flags = ef->flags;

			if (((mode & SYS_EVENT_FLAG_WAIT_AND) && (flags & bitptn) == bitptn) ||
				((mode & SYS_EVENT_FLAG_WAIT_OR) && (flags & bitptn)))
			{
				ef->waiters.invalidate(tid);

				if (mode & SYS_EVENT_FLAG_WAIT_CLEAR)
				{
					ef->flags &= ~bitptn;
				}
				else if (mode & SYS_EVENT_FLAG_WAIT_CLEAR_ALL)
				{
					ef->flags = 0;
				}

				if (result.IsGood())
				{
					result = flags;
					return CELL_OK;
				}
				return CELL_EFAULT;
			}
		}

		Sleep(1);

		if (counter++ > max_counter)
		{
			ef->waiters.invalidate(tid);
			return CELL_ETIMEDOUT;
		}
		if (Emu.IsStopped())
		{
			ConLog.Warning("sys_event_flag_wait(id=%d) aborted", eflag_id);
			return CELL_OK;
		}
	}
}

int sys_event_flag_trywait(u32 eflag_id, u64 bitptn, u32 mode, mem64_t result)
{
	sys_event_flag.Warning("sys_event_flag_trywait(eflag_id=%d, bitptn=0x%llx, mode=0x%x, result_addr=0x%x)",
		eflag_id, bitptn, mode, result.GetAddr());

	if (result.IsGood()) result = 0;

	switch (mode & 0xf)
	{
	case SYS_EVENT_FLAG_WAIT_AND: break;
	case SYS_EVENT_FLAG_WAIT_OR: break;
	default: return CELL_EINVAL;
	}

	switch (mode & ~0xf)
	{
	case 0: break; // ???
	case SYS_EVENT_FLAG_WAIT_CLEAR: break;
	case SYS_EVENT_FLAG_WAIT_CLEAR_ALL: break;
	default: return CELL_EINVAL;
	}

	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	SMutexLocker lock(ef->m_mutex);

	u64 flags = ef->flags;

	if (((mode & SYS_EVENT_FLAG_WAIT_AND) && (flags & bitptn) == bitptn) ||
		((mode & SYS_EVENT_FLAG_WAIT_OR) && (flags & bitptn)))
	{
		if (mode & SYS_EVENT_FLAG_WAIT_CLEAR)
		{
			ef->flags &= ~bitptn;
		}
		else if (mode & SYS_EVENT_FLAG_WAIT_CLEAR_ALL)
		{
			ef->flags = 0;
		}

		if (result.IsGood())
		{
			result = flags;
			return CELL_OK;
		}
		return CELL_EFAULT;
	}

	return CELL_EBUSY;
}

int sys_event_flag_set(u32 eflag_id, u64 bitptn)
{
	sys_event_flag.Warning("sys_event_flag_set(eflag_id=%d, bitptn=0x%llx)", eflag_id, bitptn);

	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	SMutexLocker lock(ef->m_mutex);
	ef->flags |= bitptn;

	return CELL_OK;
}

int sys_event_flag_clear(u32 eflag_id, u64 bitptn)
{
	sys_event_flag.Warning("sys_event_flag_clear(eflag_id=%d, bitptn=0x%llx)", eflag_id, bitptn);

	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	SMutexLocker lock(ef->m_mutex);
	ef->flags &= bitptn;

	return CELL_OK;
}

int sys_event_flag_cancel(u32 eflag_id, mem32_t num)
{
	sys_event_flag.Error("sys_event_flag_cancel(eflag_id=%d, num_addr=0x%x)", eflag_id, num.GetAddr());

	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	return CELL_OK;
}

int sys_event_flag_get(u32 eflag_id, mem64_t flags)
{
	sys_event_flag.Warning("sys_event_flag_get(eflag_id=%d, flags_addr=0x%x)", eflag_id, flags.GetAddr());
	
	if (!flags.IsGood())
	{
		return CELL_EFAULT;
	}
	
	event_flag* ef;
	if(!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	flags = ef->flags;

	return CELL_OK;
}