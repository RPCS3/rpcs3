#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "sys_process.h"
#include "sys_event.h"

logs::channel sys_event("sys_event", logs::level::notice);

template<> DECLARE(ipc_manager<lv2_event_queue_t, u64>::g_ipc) {};

extern u64 get_system_time();

std::shared_ptr<lv2_event_queue_t> lv2_event_queue_t::make(u32 protocol, s32 type, u64 name, u64 ipc_key, s32 size)
{
	auto queue = std::make_shared<lv2_event_queue_t>(protocol, type, name, ipc_key, size);

	auto make_expr = WRAP_EXPR(idm::import<lv2_event_queue_t>(WRAP_EXPR(queue)));

	if (ipc_key == SYS_EVENT_QUEUE_LOCAL)
	{
		// Not an IPC queue
		return make_expr();
	}

	// IPC queue
	if (ipc_manager<lv2_event_queue_t, u64>::add(ipc_key, make_expr))
	{
		return queue;
	}

	return nullptr;
}

std::shared_ptr<lv2_event_queue_t> lv2_event_queue_t::find(u64 ipc_key)
{
	if (ipc_key == SYS_EVENT_QUEUE_LOCAL)
	{
		// Invalid IPC key
		return{};
	}

	return ipc_manager<lv2_event_queue_t, u64>::get(ipc_key);
}

void lv2_event_queue_t::push(lv2_lock_t, u64 source, u64 data1, u64 data2, u64 data3)
{
	EXPECTS(m_sq.empty() || m_events.empty());

	// save event if no waiters
	if (m_sq.empty())
	{
		return m_events.emplace_back(source, data1, data2, data3);
	}

	// notify waiter; protocol is ignored in current implementation
	auto& thread = m_sq.front();

	if (type == SYS_PPU_QUEUE && thread->type == cpu_type::ppu)
	{
		// store event data in registers
		auto& ppu = static_cast<PPUThread&>(*thread);

		ppu.GPR[4] = source;
		ppu.GPR[5] = data1;
		ppu.GPR[6] = data2;
		ppu.GPR[7] = data3;
	}
	else if (type == SYS_SPU_QUEUE && thread->type == cpu_type::spu)
	{
		// store event data in In_MBox
		auto& spu = static_cast<SPUThread&>(*thread);

		spu.ch_in_mbox.set_values(4, CELL_OK, static_cast<u32>(data1), static_cast<u32>(data2), static_cast<u32>(data3));
	}
	else
	{
		throw fmt::exception("Unexpected (queue.type=%d, thread.type=%d)" HERE, type, thread->type);
	}

	VERIFY(!thread->state.test_and_set(cpu_state::signal));
	(*thread)->notify();

	return m_sq.pop_front();
}

lv2_event_queue_t::event_type lv2_event_queue_t::pop(lv2_lock_t)
{
	EXPECTS(m_events.size());
	auto result = m_events.front();
	m_events.pop_front();
	return result;
}

s32 sys_event_queue_create(vm::ptr<u32> equeue_id, vm::ptr<sys_event_queue_attribute_t> attr, u64 event_queue_key, s32 size)
{
	sys_event.warning("sys_event_queue_create(equeue_id=*0x%x, attr=*0x%x, event_queue_key=0x%llx, size=%d)", equeue_id, attr, event_queue_key, size);

	if (size <= 0 || size > 127)
	{
		return CELL_EINVAL;
	}

	const u32 protocol = attr->protocol;

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_PRIORITY)
	{
		sys_event.error("sys_event_queue_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	const u32 type = attr->type;

	if (type != SYS_PPU_QUEUE && type != SYS_SPU_QUEUE)
	{
		sys_event.error("sys_event_queue_create(): unknown type (0x%x)", type);
		return CELL_EINVAL;
	}

	const auto queue = lv2_event_queue_t::make(protocol, type, reinterpret_cast<u64&>(attr->name), event_queue_key, size);

	if (!queue)
	{
		return CELL_EEXIST;
	}

	*equeue_id = queue->id;
	
	return CELL_OK;
}

s32 sys_event_queue_destroy(u32 equeue_id, s32 mode)
{
	sys_event.warning("sys_event_queue_destroy(equeue_id=0x%x, mode=%d)", equeue_id, mode);

	LV2_LOCK;

	const auto queue = idm::get<lv2_event_queue_t>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (mode && mode != SYS_EVENT_QUEUE_DESTROY_FORCE)
	{
		return CELL_EINVAL;
	}

	if (!mode && queue->waiters())
	{
		return CELL_EBUSY;
	}

	// cleanup
	idm::remove<lv2_event_queue_t>(equeue_id);

	// signal all threads to return CELL_ECANCELED
	for (auto& thread : queue->thread_queue(lv2_lock))
	{
		if (queue->type == SYS_PPU_QUEUE && thread->type == cpu_type::ppu)
		{
			static_cast<PPUThread&>(*thread).GPR[3] = 1;
		}
		else if (queue->type == SYS_SPU_QUEUE && thread->type == cpu_type::spu)
		{
			static_cast<SPUThread&>(*thread).ch_in_mbox.set_values(1, CELL_ECANCELED);
		}
		else
		{
			throw fmt::exception("Unexpected (queue.type=%d, thread.type=%d)" HERE, queue->type, thread->type);
		}

		thread->state += cpu_state::signal;
		(*thread)->notify();
	}

	return CELL_OK;
}

s32 sys_event_queue_tryreceive(u32 equeue_id, vm::ptr<sys_event_t> event_array, s32 size, vm::ptr<u32> number)
{
	sys_event.trace("sys_event_queue_tryreceive(equeue_id=0x%x, event_array=*0x%x, size=%d, number=*0x%x)", equeue_id, event_array, size, number);

	LV2_LOCK;

	const auto queue = idm::get<lv2_event_queue_t>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (size < 0)
	{
		throw fmt::exception("Negative size (%d)" HERE, size);
	}

	if (queue->type != SYS_PPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	s32 count = 0;

	while (queue->waiters() == 0 && count < size && queue->events())
	{
		auto& dest = event_array[count++];

		std::tie(dest.source, dest.data1, dest.data2, dest.data3) = queue->pop(lv2_lock);
	}

	*number = count;

	return CELL_OK;
}

s32 sys_event_queue_receive(PPUThread& ppu, u32 equeue_id, vm::ptr<sys_event_t> dummy_event, u64 timeout)
{
	sys_event.trace("sys_event_queue_receive(equeue_id=0x%x, *0x%x, timeout=0x%llx)", equeue_id, dummy_event, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto queue = idm::get<lv2_event_queue_t>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (queue->type != SYS_PPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	if (queue->events())
	{
		// event data is returned in registers (dummy_event is not used)
		std::tie(ppu.GPR[4], ppu.GPR[5], ppu.GPR[6], ppu.GPR[7]) = queue->pop(lv2_lock);
		return CELL_OK;
	}

	// cause (if cancelled) will be returned in r3
	ppu.GPR[3] = 0;

	// add waiter; protocol is ignored in current implementation
	sleep_entry<cpu_thread> waiter(queue->thread_queue(lv2_lock), ppu);

	while (!ppu.state.test_and_reset(cpu_state::signal))
	{
		CHECK_EMU_STATUS;

		if (timeout)
		{
			const u64 passed = get_system_time() - start_time;

			if (passed >= timeout)
			{
				return CELL_ETIMEDOUT;
			}

			get_current_thread_cv().wait_for(lv2_lock, std::chrono::microseconds(timeout - passed));
		}
		else
		{
			get_current_thread_cv().wait(lv2_lock);
		}
	}

	if (ppu.GPR[3])
	{
		ENSURES(!idm::check<lv2_event_queue_t>(equeue_id));
		return CELL_ECANCELED;
	}

	// r4-r7 registers must be set by push()
	return CELL_OK;
}

s32 sys_event_queue_drain(u32 equeue_id)
{
	sys_event.trace("sys_event_queue_drain(equeue_id=0x%x)", equeue_id);

	LV2_LOCK;

	const auto queue = idm::get<lv2_event_queue_t>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	queue->clear(lv2_lock);

	return CELL_OK;
}

s32 sys_event_port_create(vm::ptr<u32> eport_id, s32 port_type, u64 name)
{
	sys_event.warning("sys_event_port_create(eport_id=*0x%x, port_type=%d, name=0x%llx)", eport_id, port_type, name);

	if (port_type != SYS_EVENT_PORT_LOCAL)
	{
		sys_event.error("sys_event_port_create(): unknown port type (%d)", port_type);
		return CELL_EINVAL;
	}

	*eport_id = idm::make<lv2_event_port_t>(port_type, name);

	return CELL_OK;
}

s32 sys_event_port_destroy(u32 eport_id)
{
	sys_event.warning("sys_event_port_destroy(eport_id=0x%x)", eport_id);

	LV2_LOCK;

	const auto port = idm::get<lv2_event_port_t>(eport_id);

	if (!port)
	{
		return CELL_ESRCH;
	}

	if (!port->queue.expired())
	{
		return CELL_EISCONN;
	}

	idm::remove<lv2_event_port_t>(eport_id);

	return CELL_OK;
}

s32 sys_event_port_connect_local(u32 eport_id, u32 equeue_id)
{
	sys_event.warning("sys_event_port_connect_local(eport_id=0x%x, equeue_id=0x%x)", eport_id, equeue_id);

	LV2_LOCK;

	const auto port = idm::get<lv2_event_port_t>(eport_id);
	const auto queue = idm::get<lv2_event_queue_t>(equeue_id);

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
	sys_event.warning("sys_event_port_disconnect(eport_id=0x%x)", eport_id);

	LV2_LOCK;

	const auto port = idm::get<lv2_event_port_t>(eport_id);

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
	sys_event.trace("sys_event_port_send(eport_id=0x%x, data1=0x%llx, data2=0x%llx, data3=0x%llx)", eport_id, data1, data2, data3);

	LV2_LOCK;

	const auto port = idm::get<lv2_event_port_t>(eport_id);

	if (!port)
	{
		return CELL_ESRCH;
	}

	const auto queue = port->queue.lock();

	if (!queue)
	{
		return CELL_ENOTCONN;
	}

	if (queue->events() >= queue->size)
	{
		return CELL_EBUSY;
	}

	const u64 source = port->name ? port->name : ((u64)process_getpid() << 32) | (u64)eport_id;

	queue->push(lv2_lock, source, data1, data2, data3);

	return CELL_OK;
}
