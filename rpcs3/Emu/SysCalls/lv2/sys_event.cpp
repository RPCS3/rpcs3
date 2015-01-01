#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/Memory/atomic_type.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Event.h"
#include "sleep_queue_type.h"
#include "sys_time.h"
#include "sys_process.h"
#include "sys_event.h"

SysCallBase sys_event("sys_event");

u32 event_queue_create(u32 protocol, s32 type, u64 name_u64, u64 event_queue_key, s32 size)
{
	std::shared_ptr<EventQueue> eq(new EventQueue(protocol, type, name_u64, event_queue_key, size));

	if (event_queue_key && !Emu.GetEventManager().RegisterKey(eq, event_queue_key))
	{
		return 0;
	}

	const u32 id = sys_event.GetNewId(eq, TYPE_EVENT_QUEUE);
	eq->sq.set_full_name(fmt::Format("EventQueue(%d)", id));
	sys_event.Warning("*** event_queue created [%s] (protocol=0x%x, type=0x%x, key=0x%llx, size=0x%x): id = %d",
		std::string((const char*)&name_u64, 8).c_str(), protocol, type, event_queue_key, size, id);
	return id;
}

s32 sys_event_queue_create(vm::ptr<u32> equeue_id, vm::ptr<sys_event_queue_attr> attr, u64 event_queue_key, s32 size)
{
	sys_event.Warning("sys_event_queue_create(equeue_id_addr=0x%x, attr_addr=0x%x, event_queue_key=0x%llx, size=%d)",
		equeue_id.addr(), attr.addr(), event_queue_key, size);

	if(size <= 0 || size > 127)
	{
		return CELL_EINVAL;
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
	default: sys_event.Error("Unknown 0x%x type attr", (s32)attr->type); return CELL_EINVAL;
	}

	if (event_queue_key && Emu.GetEventManager().CheckKey(event_queue_key))
	{
		return CELL_EEXIST;
	}

	if (u32 id = event_queue_create(attr->protocol, attr->type, attr->name_u64, event_queue_key, size))
	{
		*equeue_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;	
}

s32 sys_event_queue_destroy(u32 equeue_id, int mode)
{
	sys_event.Todo("sys_event_queue_destroy(equeue_id=%d, mode=0x%x)", equeue_id, mode);

	std::shared_ptr<EventQueue> eq;
	if (!Emu.GetIdManager().GetIDData(equeue_id, eq))
	{
		return CELL_ESRCH;
	}

	if (mode && mode != SYS_EVENT_QUEUE_DESTROY_FORCE)
	{
		return CELL_EINVAL;
	}

	//u32 tid = GetCurrentPPUThread().GetId();
	//eq->sq.m_mutex.lock();
	//eq->owner.lock(tid);
	// check if some threads are waiting for an event
	//if (!mode && eq->sq.list.size())
	//{
	//	eq->owner.unlock(tid);
	//	eq->sq.m_mutex.unlock();
	//	return CELL_EBUSY;
	//}
	//eq->owner.unlock(tid, ~0);
	//eq->sq.m_mutex.unlock();
	//while (eq->sq.list.size())
	//{
	//	std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	//	if (Emu.IsStopped())
	//	{
	//		sys_event.Warning("sys_event_queue_destroy(equeue=%d) aborted", equeue_id);
	//		break;
	//	}
	//}

	Emu.GetEventManager().UnregisterKey(eq->key);
	eq->ports.clear();
	Emu.GetIdManager().RemoveID(equeue_id);

	return CELL_OK;
}

s32 sys_event_queue_tryreceive(u32 equeue_id, vm::ptr<sys_event_data> event_array, s32 size, vm::ptr<u32> number)
{
	sys_event.Todo("sys_event_queue_tryreceive(equeue_id=%d, event_array_addr=0x%x, size=%d, number_addr=0x%x)",
		equeue_id, event_array.addr(), size, number.addr());

	std::shared_ptr<EventQueue> eq;
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
		*number = 0;
		return CELL_OK;
	}

	//u32 tid = GetCurrentPPUThread().GetId();
	//eq->sq.m_mutex.lock();
	//eq->owner.lock(tid);
	//if (eq->sq.list.size())
	//{
	//	*number = 0;
	//	eq->owner.unlock(tid);
	//	eq->sq.m_mutex.unlock();
	//	return CELL_OK;
	//}
	*number = eq->events.pop_all(event_array.get_ptr(), size);
	//eq->owner.unlock(tid);
	//eq->sq.m_mutex.unlock();
	return CELL_OK;
}

s32 sys_event_queue_receive(u32 equeue_id, vm::ptr<sys_event_data> dummy_event, u64 timeout)
{
	// dummy_event argument is ignored, data returned in registers
	sys_event.Log("sys_event_queue_receive(equeue_id=%d, dummy_event_addr=0x%x, timeout=%lld)",
		equeue_id, dummy_event.addr(), timeout);

	const u64 start_time = get_system_time();

	std::shared_ptr<EventQueue> eq;
	if (!Emu.GetIdManager().GetIDData(equeue_id, eq))
	{
		return CELL_ESRCH;
	}

	if (eq->type != SYS_PPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	const u32 tid = GetCurrentPPUThread().GetId();

	eq->sq.push(tid, eq->protocol); // add thread to sleep queue

	while (true)
	{
		const u32 old_owner = eq->owner.compare_and_swap(0, tid);
		const s32 res = old_owner ? (old_owner == tid ? 1 : 2) : 0;

		switch (res)
		{
		case 0:
		{
			const u32 next = eq->events.count() ? eq->sq.signal(eq->protocol) : 0;
			if (next != tid)
			{
				if (!eq->owner.compare_and_swap_test(tid, next))
				{
					assert(!"sys_event_queue_receive() failed (I)");
				}
				break;
			}
			// fallthrough
		}
		case 1:
		{
			sys_event_data event;
			eq->events.pop(event);
			if (!eq->owner.compare_and_swap_test(tid, 0))
			{
				assert(!"sys_event_queue_receive() failed (II)");
			}
			sys_event.Log(" *** event received: source=0x%llx, d1=0x%llx, d2=0x%llx, d3=0x%llx", 
				(u64)event.source, (u64)event.data1, (u64)event.data2, (u64)event.data3);
			/* passing event data in registers */
			PPUThread& t = GetCurrentPPUThread();
			t.GPR[4] = event.source;
			t.GPR[5] = event.data1;
			t.GPR[6] = event.data2;
			t.GPR[7] = event.data3;
			if (!eq->sq.invalidate(tid, eq->protocol))
			{
				assert(!"sys_event_queue_receive() failed (receiving)");
			}
			return CELL_OK;
		}
		}

		if (!~old_owner)
		{
			if (!eq->sq.invalidate(tid, eq->protocol))
			{
				assert(!"sys_event_queue_receive() failed (cancelling)");
			}
			return CELL_ECANCELED;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (timeout && get_system_time() - start_time > timeout)
		{
			if (!eq->sq.invalidate(tid, eq->protocol))
			{
				assert(!"sys_event_queue_receive() failed (timeout)");
			}
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_event.Warning("sys_event_queue_receive(equeue=%d) aborted", equeue_id);
			return CELL_OK;
		}
	}
}

s32 sys_event_queue_drain(u32 equeue_id)
{
	sys_event.Log("sys_event_queue_drain(equeue_id=%d)", equeue_id);

	std::shared_ptr<EventQueue> eq;
	if (!Emu.GetIdManager().GetIDData(equeue_id, eq))
	{
		return CELL_ESRCH;
	}

	eq->events.clear();

	return CELL_OK;
}

u32 event_port_create(u64 name)
{
	std::shared_ptr<EventPort> eport(new EventPort());
	u32 id = sys_event.GetNewId(eport, TYPE_EVENT_PORT);
	eport->name = name ? name : ((u64)process_getpid() << 32) | (u64)id;
	sys_event.Warning("*** sys_event_port created: id = %d, name=0x%llx", id, eport->name);
	return id;
}

s32 sys_event_port_create(vm::ptr<u32> eport_id, s32 port_type, u64 name)
{
	sys_event.Warning("sys_event_port_create(eport_id_addr=0x%x, port_type=0x%x, name=0x%llx)",
		eport_id.addr(), port_type, name);

	if (port_type != SYS_EVENT_PORT_LOCAL)
	{
		sys_event.Error("sys_event_port_create: invalid port_type(0x%x)", port_type);
		return CELL_EINVAL;
	}

	*eport_id = event_port_create(name);
	return CELL_OK;
}

s32 sys_event_port_destroy(u32 eport_id)
{
	sys_event.Warning("sys_event_port_destroy(eport_id=%d)", eport_id);

	std::shared_ptr<EventPort> eport;
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

	std::shared_ptr<EventPort> eport;
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

	std::shared_ptr<EventQueue> equeue;
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

	std::shared_ptr<EventPort> eport;
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

	std::shared_ptr<EventPort> eport;
	if (!Emu.GetIdManager().GetIDData(eport_id, eport))
	{
		return CELL_ESRCH;
	}

	std::lock_guard<std::mutex> lock(eport->m_mutex);

	std::shared_ptr<EventQueue> eq = eport->eq;
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
