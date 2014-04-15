#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/event.h"

SysCallBase sys_event("sys_event");

//128
int sys_event_queue_create(mem32_t equeue_id, mem_ptr_t<sys_event_queue_attr> attr, u64 event_queue_key, int size)
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

int sys_event_queue_destroy(u32 equeue_id, int mode)
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
			ConLog.Warning("sys_event_queue_destroy(equeue=%d) aborted", equeue_id);
			break;
		}
	}

	Emu.GetEventManager().UnregisterKey(eq->key);
	eq->ports.clear();
	Emu.GetIdManager().RemoveID(equeue_id);

	return CELL_OK;
}

int sys_event_queue_tryreceive(u32 equeue_id, mem_ptr_t<sys_event_data> event_array, int size, mem32_t number)
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

int sys_event_queue_receive(u32 equeue_id, mem_ptr_t<sys_event_data> event, u64 timeout)
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
			if (Emu.IsStopped()) ConLog.Warning("sys_event_queue_receive(equeue=%d) aborted", equeue_id);
			eq->sq.invalidate(tid);
			return CELL_ETIMEDOUT;
		}
	}
}

int sys_event_queue_drain(u32 equeue_id)
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

int sys_event_port_create(mem32_t eport_id, int port_type, u64 name)
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

int sys_event_port_destroy(u32 eport_id)
{
	sys_event.Warning("sys_event_port_destroy(eport_id=%d)", eport_id);

	EventPort* eport;
	if (!Emu.GetIdManager().GetIDData(eport_id, eport))
	{
		return CELL_ESRCH;
	}

	u32 tid = GetCurrentPPUThread().GetId();

	if (eport->mutex.trylock(tid) != SMR_OK)
	{
		return CELL_EISCONN;
	}

	if (eport->eq)
	{
		eport->mutex.unlock(tid);
		return CELL_EISCONN;
	}

	eport->mutex.unlock(tid, ~0);
	Emu.GetIdManager().RemoveID(eport_id);
	return CELL_OK;
}

int sys_event_port_connect_local(u32 eport_id, u32 equeue_id)
{
	sys_event.Warning("sys_event_port_connect_local(eport_id=%d, equeue_id=%d)", eport_id, equeue_id);

	EventPort* eport;
	if (!Emu.GetIdManager().GetIDData(eport_id, eport))
	{
		return CELL_ESRCH;
	}

	u32 tid = GetCurrentPPUThread().GetId();

	if (eport->mutex.trylock(tid) != SMR_OK)
	{
		return CELL_EISCONN;
	}

	if (eport->eq)
	{
		eport->mutex.unlock(tid);
		return CELL_EISCONN;
	}

	EventQueue* equeue;
	if (!Emu.GetIdManager().GetIDData(equeue_id, equeue))
	{
		sys_event.Error("sys_event_port_connect_local: event_queue(%d) not found!", equeue_id);
		eport->mutex.unlock(tid);
		return CELL_ESRCH;
	}
	else
	{
		equeue->ports.add(eport);
	}

	eport->eq = equeue;
	eport->mutex.unlock(tid);
	return CELL_OK;
}

int sys_event_port_disconnect(u32 eport_id)
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

	u32 tid = GetCurrentPPUThread().GetId();

	if (eport->mutex.trylock(tid) != SMR_OK)
	{
		return CELL_EBUSY;
	}

	eport->eq->ports.remove(eport);
	eport->eq = nullptr;
	eport->mutex.unlock(tid);
	return CELL_OK;
}

int sys_event_port_send(u32 eport_id, u64 data1, u64 data2, u64 data3)
{
	sys_event.Log("sys_event_port_send(eport_id=%d, data1=0x%llx, data2=0x%llx, data3=0x%llx)",
		eport_id, data1, data2, data3);

	EventPort* eport;
	if (!Emu.GetIdManager().GetIDData(eport_id, eport))
	{
		return CELL_ESRCH;
	}

	SMutexLocker lock(eport->mutex);

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
