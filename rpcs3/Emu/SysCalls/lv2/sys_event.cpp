#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Event.h"
#include "sys_process.h"
#include "sys_event.h"

SysCallBase sys_event("sys_event");

extern u64 get_system_time();

lv2_event_queue_t::lv2_event_queue_t(u32 protocol, s32 type, u64 name, u64 key, s32 size)
	: id(Emu.GetIdManager().get_current_id())
	, protocol(protocol)
	, type(type)
	, name(name)
	, key(key)
	, size(size)
{
}

void lv2_event_queue_t::push(lv2_lock_t& lv2_lock, u64 source, u64 data1, u64 data2, u64 data3)
{
	CHECK_LV2_LOCK(lv2_lock);

	events.emplace_back(source, data1, data2, data3);

	// notify waiter; protocol is ignored in current implementation
	for (auto& waiter : sq)
	{
		if (waiter->signal())
		{
			return;
		}
	}
}

s32 sys_event_queue_create(vm::ptr<u32> equeue_id, vm::ptr<sys_event_queue_attribute_t> attr, u64 event_queue_key, s32 size)
{
	sys_event.Warning("sys_event_queue_create(equeue_id=*0x%x, attr=*0x%x, event_queue_key=0x%llx, size=%d)", equeue_id, attr, event_queue_key, size);

	if (size <= 0 || size > 127)
	{
		return CELL_EINVAL;
	}

	const u32 protocol = attr->protocol;

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_PRIORITY)
	{
		sys_event.Error("sys_event_queue_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	const u32 type = attr->type;

	if (type != SYS_PPU_QUEUE && type != SYS_SPU_QUEUE)
	{
		sys_event.Error("sys_event_queue_create(): unknown type (0x%x)", type);
		return CELL_EINVAL;
	}

	const auto queue = Emu.GetEventManager().MakeEventQueue(event_queue_key, protocol, type, attr->name_u64, event_queue_key, size);

	if (!queue)
	{
		return CELL_EEXIST;
	}

	*equeue_id = queue->id;
	
	return CELL_OK;
}

s32 sys_event_queue_destroy(u32 equeue_id, s32 mode)
{
	sys_event.Warning("sys_event_queue_destroy(equeue_id=0x%x, mode=%d)", equeue_id, mode);

	LV2_LOCK;

	const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (mode && mode != SYS_EVENT_QUEUE_DESTROY_FORCE)
	{
		return CELL_EINVAL;
	}

	if (!mode && queue->sq.size())
	{
		return CELL_EBUSY;
	}

	// cleanup
	Emu.GetEventManager().UnregisterKey(queue->key);
	Emu.GetIdManager().remove<lv2_event_queue_t>(equeue_id);

	// signal all threads to return CELL_ECANCELED
	for (auto& thread : queue->sq)
	{
		thread->signal();
	}

	return CELL_OK;
}

s32 sys_event_queue_tryreceive(u32 equeue_id, vm::ptr<sys_event_t> event_array, s32 size, vm::ptr<u32> number)
{
	sys_event.Log("sys_event_queue_tryreceive(equeue_id=0x%x, event_array=*0x%x, size=%d, number=*0x%x)", equeue_id, event_array, size, number);

	LV2_LOCK;

	const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (size < 0)
	{
		throw EXCEPTION("Negative size");
	}

	if (queue->type != SYS_PPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	s32 count = 0;

	while (queue->sq.empty() && count < size && queue->events.size())
	{
		auto& dest = event_array[count++];

		std::tie(dest.source, dest.data1, dest.data2, dest.data3) = queue->events.front();

		queue->events.pop_front();
	}

	*number = count;

	return CELL_OK;
}

s32 sys_event_queue_receive(PPUThread& ppu, u32 equeue_id, vm::ptr<sys_event_t> dummy_event, u64 timeout)
{
	sys_event.Log("sys_event_queue_receive(equeue_id=0x%x, *0x%x, timeout=0x%llx)", equeue_id, dummy_event, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (queue->type != SYS_PPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	if (queue->events.empty() || !queue->sq.empty())
	{
		// add waiter; protocol is ignored in current implementation
		sleep_queue_entry_t waiter(ppu, queue->sq);

		while (!ppu.unsignal())
		{
			CHECK_EMU_STATUS;

			if (timeout)
			{
				const u64 passed = get_system_time() - start_time;

				if (passed >= timeout)
				{
					return CELL_ETIMEDOUT;
				}

				ppu.cv.wait_for(lv2_lock, std::chrono::microseconds(timeout - passed));
			}
			else
			{
				ppu.cv.wait(lv2_lock);
			}
		}
	}

	if (queue->events.empty())
	{
		if (Emu.GetIdManager().check_id<lv2_event_queue_t>(equeue_id))
		{
			throw EXCEPTION("Unexpected");
		}

		return CELL_ECANCELED;
	}

	// event data is returned in registers (dummy_event is not used)
	std::tie(ppu.GPR[4], ppu.GPR[5], ppu.GPR[6], ppu.GPR[7]) = queue->events.front();

	queue->events.pop_front();

	return CELL_OK;
}

s32 sys_event_queue_drain(u32 equeue_id)
{
	sys_event.Log("sys_event_queue_drain(equeue_id=0x%x)", equeue_id);

	LV2_LOCK;

	const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	queue->events.clear();

	return CELL_OK;
}

s32 sys_event_port_create(vm::ptr<u32> eport_id, s32 port_type, u64 name)
{
	sys_event.Warning("sys_event_port_create(eport_id=*0x%x, port_type=%d, name=0x%llx)", eport_id, port_type, name);

	if (port_type != SYS_EVENT_PORT_LOCAL)
	{
		sys_event.Error("sys_event_port_create(): unknown port type (%d)", port_type);
		return CELL_EINVAL;
	}

	*eport_id = Emu.GetIdManager().make<lv2_event_port_t>(port_type, name);

	return CELL_OK;
}

s32 sys_event_port_destroy(u32 eport_id)
{
	sys_event.Warning("sys_event_port_destroy(eport_id=0x%x)", eport_id);

	LV2_LOCK;

	const auto port = Emu.GetIdManager().get<lv2_event_port_t>(eport_id);

	if (!port)
	{
		return CELL_ESRCH;
	}

	if (!port->queue.expired())
	{
		return CELL_EISCONN;
	}

	Emu.GetIdManager().remove<lv2_event_port_t>(eport_id);

	return CELL_OK;
}

s32 sys_event_port_connect_local(u32 eport_id, u32 equeue_id)
{
	sys_event.Warning("sys_event_port_connect_local(eport_id=0x%x, equeue_id=0x%x)", eport_id, equeue_id);

	LV2_LOCK;

	const auto port = Emu.GetIdManager().get<lv2_event_port_t>(eport_id);
	const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(equeue_id);

	if (!port || !queue)
	{
		return CELL_ESRCH;
	}

	if (port->type != SYS_EVENT_PORT_LOCAL)
	{
		return CELL_EINVAL;
	}

	if (!port->queue.expired())
	{
		return CELL_EISCONN;
	}

	port->queue = queue;

	return CELL_OK;
}

s32 sys_event_port_disconnect(u32 eport_id)
{
	sys_event.Warning("sys_event_port_disconnect(eport_id=0x%x)", eport_id);

	LV2_LOCK;

	const auto port = Emu.GetIdManager().get<lv2_event_port_t>(eport_id);

	if (!port)
	{
		return CELL_ESRCH;
	}

	const auto queue = port->queue.lock();

	if (!queue)
	{
		return CELL_ENOTCONN;
	}

	// CELL_EBUSY is not returned

	port->queue.reset();

	return CELL_OK;
}

s32 sys_event_port_send(u32 eport_id, u64 data1, u64 data2, u64 data3)
{
	sys_event.Log("sys_event_port_send(eport_id=0x%x, data1=0x%llx, data2=0x%llx, data3=0x%llx)", eport_id, data1, data2, data3);

	LV2_LOCK;

	const auto port = Emu.GetIdManager().get<lv2_event_port_t>(eport_id);

	if (!port)
	{
		return CELL_ESRCH;
	}

	const auto queue = port->queue.lock();

	if (!queue)
	{
		return CELL_ENOTCONN;
	}

	if (queue->events.size() >= queue->size)
	{
		return CELL_EBUSY;
	}

	const u64 source = port->name ? port->name : ((u64)process_getpid() << 32) | (u64)eport_id;

	queue->push(lv2_lock, source, data1, data2, data3);

	return CELL_OK;
}
