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
	lwmutex->all_info() = ~0;
	lwmutex->mutex.initialize();
	//lwmutex->waiter = lwmutex->owner.GetOwner();
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
	switch (int res = lwmutex->trylock(lwmutex->mutex.GetDeadValue()))
	{
	case CELL_OK:
		lwmutex->all_info() = 0;
		lwmutex->attribute = 0xDEADBEEF;
		lwmutex->sleep_queue = 0;
		Emu.GetIdManager().RemoveID(sq_id);
	default: return res;
	}
}

int sys_lwmutex_lock(mem_ptr_t<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sc_lwmutex.Log("sys_lwmutex_lock(lwmutex_addr=0x%x, timeout=%lld)", lwmutex.GetAddr(), timeout);

	if (!lwmutex.IsGood()) return CELL_EFAULT;

	//ConLog.Write("*** lock mutex (addr=0x%x, attr=0x%x, Nrec=%d, owner=%d, waiter=%d)",
		//lwmutex.GetAddr(), (u32)lwmutex->attribute, (u32)lwmutex->recursive_count, lwmutex->vars.parts.owner.GetOwner(), (u32)lwmutex->waiter);

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

	//ConLog.Write("*** unlocking mutex (addr=0x%x, attr=0x%x, Nrec=%d, owner=%d, waiter=%d)",
		//lwmutex.GetAddr(), (u32)lwmutex->attribute, (u32)lwmutex->recursive_count, (u32)lwmutex->vars.parts.owner.GetOwner(), (u32)lwmutex->waiter);

	return lwmutex->unlock(GetCurrentPPUThread().GetId());
}

void SleepQueue::push(u32 tid)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	list.AddCpy(tid);
}

u32 SleepQueue::pop() // SYS_SYNC_FIFO
{
	std::lock_guard<std::mutex> lock(m_mutex);
		
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
	std::lock_guard<std::mutex> lock(m_mutex);

	while (true)
	{
		if (list.GetCount())
		{
			u32 highest_prio = ~0;
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
				if (prio < highest_prio)
				{
					highest_prio = prio;
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
	std::lock_guard<std::mutex> lock(m_mutex);

	if (tid) for (u32 i = 0; i < list.GetCount(); i++)
	{
		if (list[i] == tid)
		{
			list.RemoveAt(i);
			return true;
		}
	}

	return false;
}

bool SleepQueue::finalize()
{
	if (!m_mutex.try_lock()) return false;

	for (u32 i = 0; i < list.GetCount(); i++)
	{
		if (list[i])
		{
			m_mutex.unlock();
			return false;
		}
	}

	m_mutex.unlock();
	return true;
}

int sys_lwmutex_t::trylock(be_t<u32> tid)
{
	if (attribute.ToBE() == se32(0xDEADBEEF)) return CELL_EINVAL;

	be_t<u32> owner_tid = mutex.GetFreeValue();

	if (mutex.unlock(owner_tid, owner_tid) != SMR_OK) // check free value
	{
		owner_tid = mutex.GetOwner();
		if (CPUThread* tt = Emu.GetCPU().GetThread(owner_tid))
		{
			if (!tt->IsAlive())
			{
				sc_lwmutex.Error("sys_lwmutex_t::(try)lock(%d): deadlock on invalid thread(%d)", (u32)sleep_queue, (u32)owner_tid);
				mutex.unlock(owner_tid, tid);
				recursive_count = 1;
				return CELL_OK;
			}
		}
		else
		{
			sc_lwmutex.Error("sys_lwmutex_t::(try)lock(%d): deadlock on invalid thread(%d)", (u32)sleep_queue, (u32)owner_tid);
			mutex.unlock(owner_tid, tid);
			recursive_count = 1;
			return CELL_OK;
		}
	}

	/*while ((attribute.ToBE() & se32(SYS_SYNC_ATTR_RECURSIVE_MASK)) == 0)
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("(hack) sys_lwmutex_t::(try)lock aborted (waiting for recursive attribute, attr=0x%x)", (u32)attribute);
			return CELL_ESRCH;
		}
		Sleep(1);
	}*/

	if (tid == owner_tid)
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

	switch (mutex.trylock(tid))
	{
	case SMR_OK: recursive_count = 1; return CELL_OK;
	case SMR_FAILED: return CELL_EBUSY;
	default: return CELL_EINVAL;
	}
}

int sys_lwmutex_t::unlock(be_t<u32> tid)
{
	if (mutex.unlock(tid, tid) != SMR_OK)
	{
		return CELL_EPERM;
	}
	else
	{
		if (!recursive_count || (recursive_count.ToBE() != se32(1) && (attribute.ToBE() & se32(SYS_SYNC_NOT_RECURSIVE))))
		{
			sc_lwmutex.Error("sys_lwmutex_t::unlock(%d): wrong recursive value (%d)", (u32)sleep_queue, (u32)recursive_count);
			recursive_count = 1;
		}
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
			case se32(SYS_SYNC_RETRY): break;
			}
			if (target) mutex.unlock(tid, target);
			else mutex.unlock(tid);
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

	switch (mutex.lock(tid, timeout))
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
