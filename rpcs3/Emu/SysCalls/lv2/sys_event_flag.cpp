#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_lwmutex.h"
#include "sys_event_flag.h"

SysCallBase sys_event_flag("sys_event_flag");

u32 EventFlag::check()
{
	SleepQueue sq; // TODO: implement without SleepQueue

	u32 target = 0;

	for (u32 i = 0; i < waiters.size(); i++)
	{
		if (((waiters[i].mode & SYS_EVENT_FLAG_WAIT_AND) && (flags & waiters[i].bitptn) == waiters[i].bitptn) ||
			((waiters[i].mode & SYS_EVENT_FLAG_WAIT_OR) && (flags & waiters[i].bitptn)))
		{
			if (m_protocol == SYS_SYNC_FIFO)
			{
				target = waiters[i].tid;
				break;
			}
			sq.list.push_back(waiters[i].tid);
		}
	}

	if (m_protocol == SYS_SYNC_PRIORITY)
	{
		target = sq.pop_prio();
	}

	return target;
}

s32 sys_event_flag_create(vm::ptr<be_t<u32>> eflag_id, vm::ptr<sys_event_flag_attr> attr, u64 init)
{
	sys_event_flag.Warning("sys_event_flag_create(eflag_id_addr=0x%x, attr_addr=0x%x, init=0x%llx)",
		eflag_id.addr(), attr.addr(), init);

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_RETRY): sys_event_flag.Todo("sys_event_flag_create(): SYS_SYNC_RETRY"); break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_event_flag.Todo("sys_event_flag_create(): SYS_SYNC_PRIORITY_INHERIT"); break;
	case se32(SYS_SYNC_FIFO): break;
	default: return CELL_EINVAL;
	}

	if (attr->pshared.ToBE() != se32(0x200))
	{
		return CELL_EINVAL;
	}

	switch (attr->type.ToBE())
	{
	case se32(SYS_SYNC_WAITER_SINGLE): break;
	case se32(SYS_SYNC_WAITER_MULTIPLE): break;
	default: return CELL_EINVAL;
	}

	u32 id = sys_event_flag.GetNewId(new EventFlag(init, (u32)attr->protocol, (int)attr->type), TYPE_EVENT_FLAG);
	*eflag_id = id;
	sys_event_flag.Warning("*** event_flag created [%s] (protocol=0x%x, type=0x%x): id = %d",
		std::string(attr->name, 8).c_str(), (u32)attr->protocol, (int)attr->type, id);

	return CELL_OK;
}

s32 sys_event_flag_destroy(u32 eflag_id)
{
	sys_event_flag.Warning("sys_event_flag_destroy(eflag_id=%d)", eflag_id);

	EventFlag* ef;
	if (!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	if (ef->waiters.size()) // ???
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(eflag_id);

	return CELL_OK;
}

s32 sys_event_flag_wait(u32 eflag_id, u64 bitptn, u32 mode, vm::ptr<be_t<u64>> result, u64 timeout)
{
	sys_event_flag.Log("sys_event_flag_wait(eflag_id=%d, bitptn=0x%llx, mode=0x%x, result_addr=0x%x, timeout=%lld)",
		eflag_id, bitptn, mode, result.addr(), timeout);

	if (result) *result = 0;

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

	EventFlag* ef;
	if (!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	const u32 tid = GetCurrentPPUThread().GetId();

	{
		ef->m_mutex.lock(tid);
		if (ef->m_type == SYS_SYNC_WAITER_SINGLE && ef->waiters.size() > 0)
		{
			ef->m_mutex.unlock(tid);
			return CELL_EPERM;
		}
		EventFlagWaiter rec;
		rec.bitptn = bitptn;
		rec.mode = mode;
		rec.tid = tid;
		ef->waiters.push_back(rec);

		if (ef->check() == tid)
		{
			u64 flags = ef->flags;

			ef->waiters.erase(ef->waiters.end() - 1);

			if (mode & SYS_EVENT_FLAG_WAIT_CLEAR)
			{
				ef->flags &= ~bitptn;
			}
			else if (mode & SYS_EVENT_FLAG_WAIT_CLEAR_ALL)
			{
				ef->flags = 0;
			}

			if (result) *result = flags;

			ef->m_mutex.unlock(tid);
			return CELL_OK;
		}
		ef->m_mutex.unlock(tid);
	}

	u64 counter = 0;
	const u64 max_counter = timeout ? (timeout / 1000) : ~0;

	while (true)
	{
		if (ef->signal.unlock(tid, tid) == SMR_OK)
		{
			ef->m_mutex.lock(tid);
			u64 flags = ef->flags;

			for (u32 i = 0; i < ef->waiters.size(); i++)
			{
				if (ef->waiters[i].tid == tid)
				{
					ef->waiters.erase(ef->waiters.begin() + i);

					if (mode & SYS_EVENT_FLAG_WAIT_CLEAR)
					{
						ef->flags &= ~bitptn;
					}
					else if (mode & SYS_EVENT_FLAG_WAIT_CLEAR_ALL)
					{
						ef->flags = 0;
					}

					if (u32 target = ef->check())
					{
						// if signal, leave both mutexes locked...
						ef->signal.unlock(tid, target);
						ef->m_mutex.unlock(tid, target);
					}
					else
					{
						ef->signal.unlock(tid);
					}

					if (result) *result = flags;

					ef->m_mutex.unlock(tid);
					return CELL_OK;
				}
			}

			ef->signal.unlock(tid);
			ef->m_mutex.unlock(tid);
			return CELL_ECANCELED;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (counter++ > max_counter)
		{
			ef->m_mutex.lock(tid);

			for (u32 i = 0; i < ef->waiters.size(); i++)
			{
				if (ef->waiters[i].tid == tid)
				{
					ef->waiters.erase(ef->waiters.begin() + i);
					break;
				}
			}

			ef->m_mutex.unlock(tid);
			return CELL_ETIMEDOUT;
		}
		if (Emu.IsStopped())
		{
			sys_event_flag.Warning("sys_event_flag_wait(id=%d) aborted", eflag_id);
			return CELL_OK;
		}
	}
}

s32 sys_event_flag_trywait(u32 eflag_id, u64 bitptn, u32 mode, vm::ptr<be_t<u64>> result)
{
	sys_event_flag.Log("sys_event_flag_trywait(eflag_id=%d, bitptn=0x%llx, mode=0x%x, result_addr=0x%x)",
		eflag_id, bitptn, mode, result.addr());

	if (result) *result = 0;

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

	EventFlag* ef;
	if (!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	const u32 tid = GetCurrentPPUThread().GetId();
	ef->m_mutex.lock(tid);

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

		if (result) *result = flags;

		ef->m_mutex.unlock(tid);
		return CELL_OK;
	}

	ef->m_mutex.unlock(tid);
	return CELL_EBUSY;
}

s32 sys_event_flag_set(u32 eflag_id, u64 bitptn)
{
	sys_event_flag.Log("sys_event_flag_set(eflag_id=%d, bitptn=0x%llx)", eflag_id, bitptn);

	EventFlag* ef;
	if (!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	u32 tid = GetCurrentPPUThread().GetId();

	ef->m_mutex.lock(tid);
	ef->flags |= bitptn;
	if (u32 target = ef->check())
	{
		// if signal, leave both mutexes locked...
		ef->signal.lock(target);
		ef->m_mutex.unlock(tid, target);
	}
	else
	{
		ef->m_mutex.unlock(tid);
	}

	return CELL_OK;
}

s32 sys_event_flag_clear(u32 eflag_id, u64 bitptn)
{
	sys_event_flag.Log("sys_event_flag_clear(eflag_id=%d, bitptn=0x%llx)", eflag_id, bitptn);

	EventFlag* ef;
	if (!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	const u32 tid = GetCurrentPPUThread().GetId();
	ef->m_mutex.lock(tid);
	ef->flags &= bitptn;
	ef->m_mutex.unlock(tid);
	return CELL_OK;
}

s32 sys_event_flag_cancel(u32 eflag_id, vm::ptr<be_t<u32>> num)
{
	sys_event_flag.Log("sys_event_flag_cancel(eflag_id=%d, num_addr=0x%x)", eflag_id, num.addr());

	EventFlag* ef;
	if (!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	std::vector<u32> tids;

	const u32 tid = GetCurrentPPUThread().GetId();

	{
		ef->m_mutex.lock(tid);
		tids.resize(ef->waiters.size());
		for (u32 i = 0; i < ef->waiters.size(); i++)
		{
			tids[i] = ef->waiters[i].tid;
		}
		ef->waiters.clear();
		ef->m_mutex.unlock(tid);
	}

	for (u32 i = 0; i < tids.size(); i++)
	{
		ef->signal.lock(tids[i]);
	}

	if (Emu.IsStopped())
	{
		sys_event_flag.Warning("sys_event_flag_cancel(id=%d) aborted", eflag_id);
		return CELL_OK;
	}

	if (num) *num = (u32)tids.size();

	return CELL_OK;
}

s32 sys_event_flag_get(u32 eflag_id, vm::ptr<be_t<u64>> flags)
{
	sys_event_flag.Log("sys_event_flag_get(eflag_id=%d, flags_addr=0x%x)", eflag_id, flags.addr());

	EventFlag* ef;
	if (!sys_event_flag.CheckId(eflag_id, ef)) return CELL_ESRCH;

	const u32 tid = GetCurrentPPUThread().GetId();
	ef->m_mutex.lock(tid);
	*flags = ef->flags;
	ef->m_mutex.unlock(tid);
	return CELL_OK;
}