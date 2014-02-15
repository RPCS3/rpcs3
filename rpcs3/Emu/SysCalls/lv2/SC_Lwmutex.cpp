#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/lv2/SC_Lwmutex.h"

SysCallBase sc_lwmutex("sys_lwmutex");

int sys_lwmutex_create(mem_ptr_t<sys_lwmutex_t> lwmutex, mem_ptr_t<sys_lwmutex_attribute_t> attr)
{
	sc_lwmutex.Log("sys_lwmutex_create(lwmutex_addr=0x%x, lwmutex_attr_addr=0x%x)", 
		lwmutex.GetAddr(), attr.GetAddr());

	if (!lwmutex.IsGood() || !attr.IsGood()) return CELL_EFAULT;

	switch (attr->attr_recursive.ToBE())
	{
	case se32(SYS_SYNC_RECURSIVE): break;
	case se32(SYS_SYNC_NOT_RECURSIVE): break;
	default: sc_lwmutex.Error("Unknown recursive attribute(0x%x)", (u32)attr->attr_recursive); return CELL_EINVAL;
	}

	switch (attr->attr_protocol.ToBE())
	{
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_RETRY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sc_lwmutex.Error("Invalid SYS_SYNC_PRIORITY_INHERIT protocol attr"); return CELL_EINVAL;
	case se32(SYS_SYNC_FIFO): break;
	default: sc_lwmutex.Error("Unknown protocol attribute(0x%x)", (u32)attr->attr_protocol); return CELL_EINVAL;
	}

	lwmutex->attribute = attr->attr_protocol | attr->attr_recursive;
	lwmutex->all_info = 0;
	lwmutex->pad = 0;
	lwmutex->recursive_count = 0;

	u32 sq_id = sc_lwmutex.GetNewId(new SleepQueue(attr->name_u64));
	lwmutex->sleep_queue = sq_id;

	sc_lwmutex.Warning("*** lwmutex created [%s] (attribute=0x%x): sq_id = %d", 
		wxString(attr->name, 8).wx_str(), (u32)lwmutex->attribute, sq_id);

	return CELL_OK;
}

int sys_lwmutex_destroy(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Warning("sys_lwmutex_destroy(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	u32 sq_id = lwmutex->sleep_queue;
	if (!Emu.GetIdManager().CheckID(sq_id)) return CELL_ESRCH;

	// try to make it unable to lock
	switch (int res = lwmutex->trylock(~0)) 
	{
	case CELL_OK:
		lwmutex->attribute = 0;
		lwmutex->sleep_queue = 0;
		Emu.GetIdManager().RemoveID(sq_id);
	default: return res;
	}
}

int sys_lwmutex_lock(mem_ptr_t<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sc_lwmutex.Log("sys_lwmutex_lock(lwmutex_addr=0x%x, timeout=%lld)", lwmutex.GetAddr(), timeout);

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	return lwmutex->lock(GetCurrentPPUThread().GetId(), timeout ? ((timeout < 1000) ? 1 : (timeout / 1000)) : 0);
}

int sys_lwmutex_trylock(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Log("sys_lwmutex_trylock(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	return lwmutex->trylock(GetCurrentPPUThread().GetId());
}

int sys_lwmutex_unlock(mem_ptr_t<sys_lwmutex_t> lwmutex)
{
	sc_lwmutex.Log("sys_lwmutex_unlock(lwmutex_addr=0x%x)", lwmutex.GetAddr());

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	return lwmutex->unlock(GetCurrentPPUThread().GetId());
}

void SleepQueue::push(u32 tid)
{
	SMutexLocker lock(m_mutex);
	list.AddCpy(tid);
}

u32 SleepQueue::pop() // SYS_SYNC_FIFO
{
	SMutexLocker lock(m_mutex);
		
	while (true)
	{
		if (list.GetCount())
		{
			u32 res = list[0];
			list.RemoveAt(0);
			if (res && Emu.GetIdManager().CheckID(res))
			// check thread
			{
				return res;
			}
		}
		return 0;
	};
}

u32 SleepQueue::pop_prio() // SYS_SYNC_PRIORITY
{
	SMutexLocker lock(m_mutex);

	while (true)
	{
		if (list.GetCount())
		{
			u64 max_prio = 0;
			u32 sel = 0;
			for (u32 i = 0; i < list.GetCount(); i++)
			{
				CPUThread* t = Emu.GetCPU().GetThread(list[i]);
				if (!t)
				{
					list[i] = 0;
					sel = i;
					break;
				}
				u64 prio = t->GetPrio();
				if (prio > max_prio)
				{
					max_prio = prio;
					sel = i;
				}
			}
			u32 res = list[sel];
			list.RemoveAt(sel);
			/* if (Emu.GetIdManager().CheckID(res)) */
			if (res)
			// check thread
			{
				return res;
			}
		}
		return 0;
	}
}

u32 SleepQueue::pop_prio_inherit() // (TODO)
{
	ConLog.Error("TODO: SleepQueue::pop_prio_inherit()");
	Emu.Pause();
	return 0;
}

bool SleepQueue::invalidate(u32 tid)
{
	SMutexLocker lock(m_mutex);

	if (tid) for (u32 i = 0; i < list.GetCount(); i++)
	{
		if (list[i] = tid)
		{
			list[i] = 0;
			return true;
		}
	}

	return false;
}

bool SleepQueue::finalize()
{
	u32 tid = GetCurrentPPUThread().GetId();

	m_mutex.lock(tid);

	for (u32 i = 0; i < list.GetCount(); i++)
	{
		if (list[i])
		{
			m_mutex.unlock(tid);
			return false;
		}
	}

	return true;
}

int sys_lwmutex_t::trylock(be_t<u32> tid)
{
	if (!attribute.ToBE()) return CELL_EINVAL;

	if (tid == owner.GetOwner()) 
	{
		if (attribute.ToBE() & se32(SYS_SYNC_RECURSIVE))
		{
			recursive_count += 1;
			if (!recursive_count.ToBE()) return CELL_EKRESOURCE;
			return CELL_OK;
		}
		else
		{
			return CELL_EDEADLK;
		}
	}

	switch (owner.trylock(tid))
	{
	case SMR_OK: recursive_count = 1; return CELL_OK;
	case SMR_FAILED: return CELL_EBUSY;
	default: return CELL_EINVAL;
	}
}

int sys_lwmutex_t::unlock(be_t<u32> tid)
{
	if (tid != owner.GetOwner())
	{
		return CELL_EPERM;
	}
	else
	{
		recursive_count -= 1;
		if (!recursive_count.ToBE())
		{
			be_t<u32> target = 0;
			switch (attribute.ToBE() & se32(SYS_SYNC_ATTR_PROTOCOL_MASK))
			{
			case se32(SYS_SYNC_FIFO):
			case se32(SYS_SYNC_PRIORITY):
				SleepQueue* sq;
				if (!Emu.GetIdManager().GetIDData(sleep_queue, sq)) return CELL_ESRCH;
				target = attribute.ToBE() & se32(SYS_SYNC_FIFO) ? sq->pop() : sq->pop_prio();
			case se32(SYS_SYNC_RETRY): default: owner.unlock(tid, target); break;
			}
		}
		return CELL_OK;
	}
}

int sys_lwmutex_t::lock(be_t<u32> tid, u64 timeout)
{
	switch (int res = trylock(tid))
	{
	case CELL_EBUSY: break;
	default: return res;
	}

	SleepQueue* sq;
	if (!Emu.GetIdManager().GetIDData(sleep_queue, sq)) return CELL_ESRCH;

	switch (attribute.ToBE() & se32(SYS_SYNC_ATTR_PROTOCOL_MASK))
	{
	case se32(SYS_SYNC_PRIORITY):
	case se32(SYS_SYNC_FIFO):
		sq->push(tid);
	default: break;
	}

	switch (owner.lock(tid, timeout))
	{
	case SMR_OK:
		sq->invalidate(tid);
	case SMR_SIGNAL:
		recursive_count = 1; return CELL_OK;
	case SMR_TIMEOUT:
		sq->invalidate(tid); return CELL_ETIMEDOUT;
	case SMR_ABORT:
		if (Emu.IsStopped()) ConLog.Warning("sys_lwmutex_t::lock(sq=%d) aborted", (u32)sleep_queue);
	default:
		sq->invalidate(tid); return CELL_EINVAL;
	}
}