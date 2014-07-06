#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"

#include "Emu/SysCalls/SysCalls.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Event.h"

#include "sys_lwmutex.h"
#include "sys_event.h"

SysCallBase sys_event("sys_event");

s32 sys_event_queue_create(mem32_t equeue_id, mem_ptr_t<sys_event_queue_attr> attr, u64 event_queue_key, int size)
{
	sys_event.Warning("sys_event_queue_create(equeue_id_addr=0x%x, attr_addr=0x%x, event_queue_key=0x%llx, size=%d)",
		equeue_id.GetAddr(), attr.GetAddr(), event_queue_key, size);

	if(size <= 0 || size > 127)
	{
		return CELL_EINVAL;
	}

	if(!equeue_id.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_RETRY): sys_event.Error("Invalid SYS_SYNC_RETRY protocol attr"); return CELL_EINVAL;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_event.Error("Invalid SYS_SYNC_PRIORITY_INHERIT protocol attr"); return CELL_EINVAL;
	case se32(SYS_SYNC_FIFO): break;
	default: sys_event.Error("Unknown 0x%x protocol attr", (u32)attr->protocol); return CELL_EINVAL;
	}

	switch (attr->type.ToBE())
	{
	case se32(SYS_PPU_QUEUE): break;
	case se32(SYS_SPU_QUEUE): break;
	default: sys_event.Error("Unknown 0x%x type attr", (u32)attr->type); return CELL_EINVAL;
	}

	if (event_queue_key && Emu.GetEventManager().CheckKey(event_queue_key))
	{
		return CELL_EEXIST;
	}

	EventQueue* eq = new EventQueue((u32)attr->protocol, (int)attr->type, attr->name_u64, event_queue_key, size);

	if (event_queue_key && !Emu.GetEventManager().RegisterKey(eq, event_queue_key))
	{
		delete eq;
		return CELL_EAGAIN;
	}

	equeue_id = sys_event.GetNewId(eq);
	sys_event.Warning("*** event_queue created [%s] (protocol=0x%x, type=0x%x): id = %d",
		std::string(attr->name, 8).c_str(), (u32)attr->protocol, (int)attr->type, equeue_id.GetValue());

	return CELL_OK;
}

s32 sys_event_queue_destroy(u32 equeue_id, int mode)
{
	sys_event.Error("sys_event_queue_destroy(equeue_id=%d, mode=0x%x)", equeue_id, mode);

	EventQueue* eq;
	if (!Emu.GetIdManager().GetIDData(equeue_id, eq))
	{
		return CELL_ESRCH;
	}

	if (mode && mode != SYS_EVENT_QUEUE_DESTROY_FORCE)
	{
		return CELL_EINVAL;
	}

	u32 tid = GetCurrentPPUThread().GetId();

	eq->sq.m_mutex.lock();
	eq->owner.lock(tid);
	// check if some threads are waiting for an event
	if (!mode && eq->sq.list.size())
	{
		eq->owner.unlock(tid);
		eq->sq.m_mutex.unlock();
		return CELL_EBUSY;
	}
	eq->owner.unlock(tid, ~0);
	eq->sq.m_mutex.unlock();
	while (eq->sq.list.size())
	{
		Sleep(1);
		if (Emu.IsStopped())
		{
			LOG_WARNING(HLE, "sys_event_queue_destroy(equeue=%d) aborted", equeue_id);
			break;
		}
	}

	Emu.GetEventManager().UnregisterKey(eq->key);
	eq->ports.clear();
	Emu.GetIdManager().RemoveID(equeue_id);

	return CELL_OK;
}

s32 sys_event_queue_tryreceive(u32 equeue_id, mem_ptr_t<sys_event_data> event_array, int size, mem32_t number)
{
	sys_event.Error("sys_event_queue_tryreceive(equeue_id=%d, event_array_addr=0x%x, size=%d, number_addr=0x%x)",
		equeue_id, event_array.GetAddr(), size, number.GetAddr());

	if (size < 0 || !number.IsGood())
	{
		return CELL_EFAULT;
	}

	if (size && !Memory.IsGoodAddr(event_array.GetAddr(), sizeof(sys_event_data) * size))
	{
		return CELL_EFAULT;
	}

	EventQueue* eq;
	if (!Emu.GetIdManager().GetIDData(equeue_id, eq))
	{
		return CELL_ESRCH;
	}

	if (eq->type != SYS_PPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	if (size == 0)
	{
		number = 0;
		return CELL_OK;
	}

	u32 tid = GetCurrentPPUThread().GetId();

	eq->sq.m_mutex.lock();
	eq->owner.lock(tid);
	if (eq->sq.list.size())
	{
		number = 0;
		eq->owner.unlock(tid);
		eq->sq.m_mutex.unlock();
		return CELL_OK;
	}
	number = eq->events.pop_all((sys_event_data*)(Memory + event_array.GetAddr()), size);
	eq->owner.unlock(tid);
	eq->sq.m_mutex.unlock();
	return CELL_OK;
}

s32 sys_event_queue_receive(u32 equeue_id, mem_ptr_t<sys_event_data> event, u64 timeout)
{
	sys_event.Log("sys_event_queue_receive(equeue_id=%d, event_addr=0x%x, timeout=%lld)",
		equeue_id, event.GetAddr(), timeout);

	if (!event.IsGood())
	{
		return CELL_EFAULT;
	}

	EventQueue* eq;
	if (!Emu.GetIdManager().GetIDData(equeue_id, eq))
	{
		return CELL_ESRCH;
	}

	if (eq->type != SYS_PPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	u32 tid = GetCurrentPPUThread().GetId();

	eq->sq.push(tid); // add thread to sleep queue

	timeout = timeout ? (timeout / 1000) : ~0;
	u64 counter = 0;
	while (true)
	{
		switch (eq->owner.trylock(tid))
		{
		case SMR_OK:
			if (!eq->events.count())
			{
				eq->owner.unlock(tid);
				break;
			}
			else
			{
				u32 next = (eq->protocol == SYS_SYNC_FIFO) ? eq->sq.pop() : eq->sq.pop_prio();
				if (next != tid)
				{
					eq->owner.unlock(tid, next);
					break;
				}
			}
		case SMR_SIGNAL:
			{
				eq->events.pop(*event);
				eq->owner.unlock(tid);
				sys_event.Log(" *** event received: source=0x%llx, d1=0x%llx, d2=0x%llx, d3=0x%llx", 
					(u64)event->source, (u64)event->data1, (u64)event->data2, (u64)event->data3);
				/* passing event data in registers */
				PPUThread& t = GetCurrentPPUThread();
				t.GPR[4] = event->source;
				t.GPR[5] = event->data1;
				t.GPR[6] = event->data2;
				t.GPR[7] = event->data3;
				return CELL_OK;
			}
		case SMR_FAILED: break;
		default: eq->sq.invalidate(tid); return CELL_ECANCELED;
		}

		Sleep(1);
		if (counter++ > timeout || Emu.IsStopped())
		{
			if (Emu.IsStopped()) LOG_WARNING(HLE, "sys_event_queue_receive(equeue=%d) aborted", equeue_id);
			eq->sq.invalidate(tid);
			return CELL_ETIMEDOUT;
		}
	}
}

s32 sys_event_queue_drain(u32 equeue_id)
{
	sys_event.Log("sys_event_queue_drain(equeue_id=%d)", equeue_id);

	EventQueue* eq;
	if (!Emu.GetIdManager().GetIDData(equeue_id, eq))
	{
		return CELL_ESRCH;
	}

	eq->events.clear();

	return CELL_OK;
}

s32 sys_event_port_create(mem32_t eport_id, int port_type, u64 name)
{
	sys_event.Warning("sys_event_port_create(eport_id_addr=0x%x, port_type=0x%x, name=0x%llx)",
		eport_id.GetAddr(), port_type, name);

	if (!eport_id.IsGood())
	{
		return CELL_EFAULT;
	}

	if (port_type != SYS_EVENT_PORT_LOCAL)
	{
		sys_event.Error("sys_event_port_create: invalid port_type(0x%x)", port_type);
		return CELL_EINVAL;
	}

	EventPort* eport = new EventPort();
	u32 id = sys_event.GetNewId(eport);
	eport->name = name ? name : ((u64)sys_process_getpid() << 32) | (u64)id;
	eport_id = id;
	sys_event.Warning("*** sys_event_port created: id = %d", id);

	return CELL_OK;
}

s32 sys_event_port_destroy(u32 eport_id)
{
	sys_event.Warning("sys_event_port_destroy(eport_id=%d)", eport_id);

	EventPort* eport;
	if (!Emu.GetIdManager().GetIDData(eport_id, eport))
	{
		return CELL_ESRCH;
	}

	if (!eport->m_mutex.try_lock())
	{
		return CELL_EISCONN;
	}

	if (eport->eq)
	{
		eport->m_mutex.unlock();
		return CELL_EISCONN;
	}

	eport->m_mutex.unlock();
	Emu.GetIdManager().RemoveID(eport_id);
	return CELL_OK;
}

s32 sys_event_port_connect_local(u32 eport_id, u32 equeue_id)
{
	sys_event.Warning("sys_event_port_connect_local(eport_id=%d, equeue_id=%d)", eport_id, equeue_id);

	EventPort* eport;
	if (!Emu.GetIdManager().GetIDData(eport_id, eport))
	{
		return CELL_ESRCH;
	}

	if (!eport->m_mutex.try_lock())
	{
		return CELL_EISCONN;
	}

	if (eport->eq)
	{
		eport->m_mutex.unlock();
		return CELL_EISCONN;
	}

	EventQueue* equeue;
	if (!Emu.GetIdManager().GetIDData(equeue_id, equeue))
	{
		sys_event.Error("sys_event_port_connect_local: event_queue(%d) not found!", equeue_id);
		eport->m_mutex.unlock();
		return CELL_ESRCH;
	}
	else
	{
		equeue->ports.add(eport);
	}

	eport->eq = equeue;
	eport->m_mutex.unlock();
	return CELL_OK;
}

s32 sys_event_port_disconnect(u32 eport_id)
{
	sys_event.Warning("sys_event_port_disconnect(eport_id=%d)", eport_id);

	EventPort* eport;
	if (!Emu.GetIdManager().GetIDData(eport_id, eport))
	{
		return CELL_ESRCH;
	}

	if (!eport->eq)
	{
		return CELL_ENOTCONN;
	}

	if (!eport->m_mutex.try_lock())
	{
		return CELL_EBUSY;
	}

	eport->eq->ports.remove(eport);
	eport->eq = nullptr;
	eport->m_mutex.unlock();
	return CELL_OK;
}

s32 sys_event_port_send(u32 eport_id, u64 data1, u64 data2, u64 data3)
{
	sys_event.Log("sys_event_port_send(eport_id=%d, data1=0x%llx, data2=0x%llx, data3=0x%llx)",
		eport_id, data1, data2, data3);

	EventPort* eport;
	if (!Emu.GetIdManager().GetIDData(eport_id, eport))
	{
		return CELL_ESRCH;
	}

	std::lock_guard<std::mutex> lock(eport->m_mutex);

	EventQueue* eq = eport->eq;
	if (!eq)
	{
		return CELL_ENOTCONN;
	}

	if (!eq->events.push(eport->name, data1, data2, data3))
	{
		return CELL_EBUSY;
	}

	return CELL_OK;
}

// sys_event_flag
s32 sys_event_flag_create(mem32_t eflag_id, mem_ptr_t<sys_event_flag_attr> attr, u64 init)
{
	sys_event.Warning("sys_event_flag_create(eflag_id_addr=0x%x, attr_addr=0x%x, init=0x%llx)",
		eflag_id.GetAddr(), attr.GetAddr(), init);

	if(!eflag_id.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	switch (attr->protocol.ToBE())
	{
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_RETRY): sys_event.Warning("TODO: SYS_SYNC_RETRY attr"); break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_event.Warning("TODO: SYS_SYNC_PRIORITY_INHERIT attr"); break;
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

	eflag_id = sys_event.GetNewId(new EventFlag(init, (u32)attr->protocol, (int)attr->type));

	sys_event.Warning("*** event_flag created [%s] (protocol=0x%x, type=0x%x): id = %d",
		std::string(attr->name, 8).c_str(), (u32)attr->protocol, (int)attr->type, eflag_id.GetValue());

	return CELL_OK;
}

s32 sys_event_flag_destroy(u32 eflag_id)
{
	sys_event.Warning("sys_event_flag_destroy(eflag_id=%d)", eflag_id);

	EventFlag* ef;
	if(!sys_event.CheckId(eflag_id, ef)) return CELL_ESRCH;

	if (ef->waiters.size()) // ???
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(eflag_id);

	return CELL_OK;
}

s32 sys_event_flag_wait(u32 eflag_id, u64 bitptn, u32 mode, mem64_t result, u64 timeout)
{
	sys_event.Log("sys_event_flag_wait(eflag_id=%d, bitptn=0x%llx, mode=0x%x, result_addr=0x%x, timeout=%lld)",
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

	EventFlag* ef;
	if(!sys_event.CheckId(eflag_id, ef)) return CELL_ESRCH;

	u32 tid = GetCurrentPPUThread().GetId();

	{
		SMutexLocker lock(ef->m_mutex);
		if (ef->m_type == SYS_SYNC_WAITER_SINGLE && ef->waiters.size() > 0)
		{
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

			if (result.IsGood())
			{
				result = flags;
				return CELL_OK;
			}

			if (!result.GetAddr())
			{
				return CELL_OK;
			}
			return CELL_EFAULT;
		}
	}

	u32 counter = 0;
	const u32 max_counter = timeout ? (timeout / 1000) : ~0;

	while (true)
	{
		if (ef->signal.unlock(tid, tid) == SMR_OK)
		{
			SMutexLocker lock(ef->m_mutex);

			u64 flags = ef->flags;

			for (u32 i = 0; i < ef->waiters.size(); i++)
			{
				if (ef->waiters[i].tid == tid)
				{
					ef->waiters.erase(ef->waiters.begin() +i);

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

					if (result.IsGood())
					{
						result = flags;
						return CELL_OK;
					}

					if (!result.GetAddr())
					{
						return CELL_OK;
					}
					return CELL_EFAULT;
				}
			}

			ef->signal.unlock(tid);
			return CELL_ECANCELED;
		}

		Sleep(1);

		if (counter++ > max_counter)
		{
			SMutexLocker lock(ef->m_mutex);

			for (u32 i = 0; i < ef->waiters.size(); i++)
			{
				if (ef->waiters[i].tid == tid)
				{
					ef->waiters.erase(ef->waiters.begin() + i);
					break;
				}
			}
			
			return CELL_ETIMEDOUT;
		}
		if (Emu.IsStopped())
		{
			LOG_WARNING(HLE, "sys_event_flag_wait(id=%d) aborted", eflag_id);
			return CELL_OK;
		}
	}
}

s32 sys_event_flag_trywait(u32 eflag_id, u64 bitptn, u32 mode, mem64_t result)
{
	sys_event.Log("sys_event_flag_trywait(eflag_id=%d, bitptn=0x%llx, mode=0x%x, result_addr=0x%x)",
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

	EventFlag* ef;
	if(!sys_event.CheckId(eflag_id, ef)) return CELL_ESRCH;

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

		if (!result.GetAddr())
		{
			return CELL_OK;
		}
		return CELL_EFAULT;
	}

	return CELL_EBUSY;
}

s32 sys_event_flag_set(u32 eflag_id, u64 bitptn)
{
	sys_event.Log("sys_event_flag_set(eflag_id=%d, bitptn=0x%llx)", eflag_id, bitptn);

	EventFlag* ef;
	if(!sys_event.CheckId(eflag_id, ef)) return CELL_ESRCH;

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
	sys_event.Log("sys_event_flag_clear(eflag_id=%d, bitptn=0x%llx)", eflag_id, bitptn);

	EventFlag* ef;
	if(!sys_event.CheckId(eflag_id, ef)) return CELL_ESRCH;

	SMutexLocker lock(ef->m_mutex);
	ef->flags &= bitptn;

	return CELL_OK;
}

s32 sys_event_flag_cancel(u32 eflag_id, mem32_t num)
{
	sys_event.Log("sys_event_flag_cancel(eflag_id=%d, num_addr=0x%x)", eflag_id, num.GetAddr());

	EventFlag* ef;
	if(!sys_event.CheckId(eflag_id, ef)) return CELL_ESRCH;

	std::vector<u32> tids;

	{
		SMutexLocker lock(ef->m_mutex);
		tids.resize(ef->waiters.size());
		for (u32 i = 0; i < ef->waiters.size(); i++)
		{
			tids[i] = ef->waiters[i].tid;
		}
		ef->waiters.clear();
	}

	for (u32 i = 0; i < tids.size(); i++)
	{
		ef->signal.lock(tids[i]);
	}

	if (Emu.IsStopped())
	{
		LOG_WARNING(HLE, "sys_event_flag_cancel(id=%d) aborted", eflag_id);
		return CELL_OK;
	}

	if (num.IsGood())
	{
		num = tids.size();
		return CELL_OK;
	}

	if (!num.GetAddr())
	{
		return CELL_OK;
	}
	return CELL_EFAULT;
}

s32 sys_event_flag_get(u32 eflag_id, mem64_t flags)
{
	sys_event.Log("sys_event_flag_get(eflag_id=%d, flags_addr=0x%x)", eflag_id, flags.GetAddr());
	
	EventFlag* ef;
	if(!sys_event.CheckId(eflag_id, ef)) return CELL_ESRCH;

	if (!flags.IsGood())
	{
		return CELL_EFAULT;
	}

	SMutexLocker lock(ef->m_mutex);
	flags = ef->flags;

	return CELL_OK;
}
