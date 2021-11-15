#include "stdafx.h"
#include "sys_event.h"

#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "sys_process.h"

LOG_CHANNEL(sys_event);

lv2_event_queue::lv2_event_queue(u32 protocol, s32 type, s32 size, u64 name, u64 ipc_key) noexcept
	: id(idm::last_id())
	, protocol{static_cast<u8>(protocol)}
	, type(static_cast<u8>(type))
	, size(static_cast<u8>(size))
	, name(name)
	, key(ipc_key)
{
}

std::shared_ptr<lv2_event_queue> lv2_event_queue::find(u64 ipc_key)
{
	if (ipc_key == SYS_EVENT_QUEUE_LOCAL)
	{
		// Invalid IPC key
		return {};
	}

	return g_fxo->get<ipc_manager<lv2_event_queue, u64>>().get(ipc_key);
}

extern void resume_spu_thread_group_from_waiting(spu_thread& spu);

CellError lv2_event_queue::send(lv2_event event)
{
	std::lock_guard lock(mutex);

	if (!exists)
	{
		return CELL_ENOTCONN;
	}

	if (sq.empty())
	{
		if (events.size() < this->size + 0u)
		{
			// Save event
			events.emplace_back(event);
			return {};
		}

		return CELL_EBUSY;
	}

	if (type == SYS_PPU_QUEUE)
	{
		// Store event in registers
		auto& ppu = static_cast<ppu_thread&>(*schedule<ppu_thread>(sq, protocol));

		std::tie(ppu.gpr[4], ppu.gpr[5], ppu.gpr[6], ppu.gpr[7]) = event;

		awake(&ppu);
	}
	else
	{
		// Store event in In_MBox
		auto& spu = static_cast<spu_thread&>(*schedule<spu_thread>(sq, protocol));

		const u32 data1 = static_cast<u32>(std::get<1>(event));
		const u32 data2 = static_cast<u32>(std::get<2>(event));
		const u32 data3 = static_cast<u32>(std::get<3>(event));
		spu.ch_in_mbox.set_values(4, CELL_OK, data1, data2, data3);
		resume_spu_thread_group_from_waiting(spu);
	}

	return {};
}

error_code sys_event_queue_create(cpu_thread& cpu, vm::ptr<u32> equeue_id, vm::ptr<sys_event_queue_attribute_t> attr, u64 ipc_key, s32 size)
{
	cpu.state += cpu_flag::wait;

	sys_event.warning("sys_event_queue_create(equeue_id=*0x%x, attr=*0x%x, ipc_key=0x%llx, size=%d)", equeue_id, attr, ipc_key, size);

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

	const u32 pshared = ipc_key == SYS_EVENT_QUEUE_LOCAL ? SYS_SYNC_NOT_PROCESS_SHARED : SYS_SYNC_PROCESS_SHARED;
	constexpr u32 flags = SYS_SYNC_NEWLY_CREATED;
	const u64 name = attr->name_u64;

	if (const auto error = lv2_obj::create<lv2_event_queue>(pshared, ipc_key, flags, [&]()
	{
		return std::make_shared<lv2_event_queue>(protocol, type, size, name, ipc_key);
	}))
	{
		return error;
	}

	*equeue_id = idm::last_id();
	return CELL_OK;
}

error_code sys_event_queue_destroy(ppu_thread& ppu, u32 equeue_id, s32 mode)
{
	ppu.state += cpu_flag::wait;

	sys_event.warning("sys_event_queue_destroy(equeue_id=0x%x, mode=%d)", equeue_id, mode);

	if (mode && mode != SYS_EVENT_QUEUE_DESTROY_FORCE)
	{
		return CELL_EINVAL;
	}

	std::vector<lv2_event> events;

	const auto queue = idm::withdraw<lv2_obj, lv2_event_queue>(equeue_id, [&](lv2_event_queue& queue) -> CellError
	{
		std::lock_guard lock(queue.mutex);

		if (!mode && !queue.sq.empty())
		{
			return CELL_EBUSY;
		}

		if (queue.sq.empty())
		{
			// Optimization
			mode = 0;
		}

		if (!queue.events.empty())
		{
			// Copy events for logging, does not empty
			events.insert(events.begin(), queue.events.begin(), queue.events.end());
		}

		lv2_obj::on_id_destroy(queue, queue.key);
		return {};
	});

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (queue.ret)
	{
		return queue.ret;
	}

	std::string lost_data;

	if (mode == SYS_EVENT_QUEUE_DESTROY_FORCE)
	{
		std::deque<cpu_thread*> sq;

		std::lock_guard lock(queue->mutex);

		sq = std::move(queue->sq);

		if (sys_event.warning)
		{
			fmt::append(lost_data, "Forcefully awaken waiters (%u):\n", sq.size());

			for (auto cpu : sq)
			{
				lost_data += cpu->get_name();
				lost_data += '\n';
			}
		}

		if (queue->type == SYS_PPU_QUEUE)
		{
			for (auto cpu : sq)
			{
				static_cast<ppu_thread&>(*cpu).gpr[3] = CELL_ECANCELED;
				queue->append(cpu);
			}

			lv2_obj::awake_all();
		}
		else
		{
			for (auto cpu : sq)
			{
				static_cast<spu_thread&>(*cpu).ch_in_mbox.set_values(1, CELL_ECANCELED);
				resume_spu_thread_group_from_waiting(static_cast<spu_thread&>(*cpu));
			}
		}
	}

	if (sys_event.warning)
	{
		if (!events.empty())
		{
			fmt::append(lost_data, "Unread queue events (%u):\n", events.size());
		}

		for (const lv2_event& evt : events)
		{
			fmt::append(lost_data, "data0=0x%x, data1=0x%x, data2=0x%x, data3=0x%x\n"
				, std::get<0>(evt), std::get<1>(evt), std::get<2>(evt), std::get<3>(evt));
		}

		if (!lost_data.empty())
		{
			sys_event.warning("sys_event_queue_destroy(): %s", lost_data);
		}
	}

	return CELL_OK;
}

error_code sys_event_queue_tryreceive(ppu_thread& ppu, u32 equeue_id, vm::ptr<sys_event_t> event_array, s32 size, vm::ptr<u32> number)
{
	ppu.state += cpu_flag::wait;

	sys_event.trace("sys_event_queue_tryreceive(equeue_id=0x%x, event_array=*0x%x, size=%d, number=*0x%x)", equeue_id, event_array, size, number);

	const auto queue = idm::get<lv2_obj, lv2_event_queue>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (queue->type != SYS_PPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	std::lock_guard lock(queue->mutex);

	if (!queue->exists)
	{
		return CELL_ESRCH;
	}

	s32 count = 0;

	while (queue->sq.empty() && count < size && !queue->events.empty())
	{
		auto& dest = event_array[count++];
		const auto event = queue->events.front();
		queue->events.pop_front();

		std::tie(dest.source, dest.data1, dest.data2, dest.data3) = event;
	}

	*number = count;

	return CELL_OK;
}

error_code sys_event_queue_receive(ppu_thread& ppu, u32 equeue_id, vm::ptr<sys_event_t> dummy_event, u64 timeout)
{
	ppu.state += cpu_flag::wait;

	sys_event.trace("sys_event_queue_receive(equeue_id=0x%x, *0x%x, timeout=0x%llx)", equeue_id, dummy_event, timeout);

	ppu.gpr[3] = CELL_OK;

	const auto queue = idm::get<lv2_obj, lv2_event_queue>(equeue_id, [&](lv2_event_queue& queue) -> CellError
	{
		if (queue.type != SYS_PPU_QUEUE)
		{
			return CELL_EINVAL;
		}

		std::lock_guard lock(queue.mutex);

		// "/dev_flash/vsh/module/msmw2.sprx" seems to rely on some cryptic shared memory behaviour that we don't emulate correctly
		// This is a hack to avoid waiting for 1m40s every time we boot vsh
		if (queue.key == 0x8005911000000012 && g_ps3_process_info.get_cellos_appname() == "vsh.self"sv)
		{
			sys_event.todo("sys_event_queue_receive(equeue_id=0x%x, *0x%x, timeout=0x%llx) Bypassing timeout for msmw2.sprx", equeue_id, dummy_event, timeout);
			timeout = 1;
		}

		if (queue.events.empty())
		{
			queue.sq.emplace_back(&ppu);
			queue.sleep(ppu, timeout);
			return CELL_EBUSY;
		}

		std::tie(ppu.gpr[4], ppu.gpr[5], ppu.gpr[6], ppu.gpr[7]) = queue.events.front();
		queue.events.pop_front();
		return {};
	});

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (queue.ret)
	{
		if (queue.ret != CELL_EBUSY)
		{
			return queue.ret;
		}
	}
	else
	{
		return CELL_OK;
	}

	// If cancelled, gpr[3] will be non-zero. Other registers must contain event data.
	while (auto state = ppu.state.fetch_sub(cpu_flag::signal))
	{
		if (is_stopped(state) || state & cpu_flag::signal)
		{
			break;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				if (ppu.check_state())
				{
					return {};
				}

				std::lock_guard lock(queue->mutex);

				if (!queue->unqueue(queue->sq, &ppu))
				{
					break;
				}

				ppu.gpr[3] = CELL_ETIMEDOUT;
				break;
			}
		}
		else
		{
			thread_ctrl::wait_on(ppu.state, state);
		}
	}

	return not_an_error(ppu.gpr[3]);
}

error_code sys_event_queue_drain(ppu_thread& ppu, u32 equeue_id)
{
	ppu.state += cpu_flag::wait;

	sys_event.trace("sys_event_queue_drain(equeue_id=0x%x)", equeue_id);

	const auto queue = idm::check<lv2_obj, lv2_event_queue>(equeue_id, [&](lv2_event_queue& queue)
	{
		std::lock_guard lock(queue.mutex);

		queue.events.clear();
	});

	if (!queue)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_event_port_create(cpu_thread& cpu, vm::ptr<u32> eport_id, s32 port_type, u64 name)
{
	cpu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_create(eport_id=*0x%x, port_type=%d, name=0x%llx)", eport_id, port_type, name);

	if (port_type != SYS_EVENT_PORT_LOCAL && port_type != 3)
	{
		sys_event.error("sys_event_port_create(): unknown port type (%d)", port_type);
		return CELL_EINVAL;
	}

	if (const u32 id = idm::make<lv2_obj, lv2_event_port>(port_type, name))
	{
		*eport_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code sys_event_port_destroy(ppu_thread& ppu, u32 eport_id)
{
	ppu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_destroy(eport_id=0x%x)", eport_id);

	const auto port = idm::withdraw<lv2_obj, lv2_event_port>(eport_id, [](lv2_event_port& port) -> CellError
	{
		if (lv2_obj::check(port.queue))
		{
			return CELL_EISCONN;
		}

		return {};
	});

	if (!port)
	{
		return CELL_ESRCH;
	}

	if (port.ret)
	{
		return port.ret;
	}

	return CELL_OK;
}

error_code sys_event_port_connect_local(cpu_thread& cpu, u32 eport_id, u32 equeue_id)
{
	cpu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_connect_local(eport_id=0x%x, equeue_id=0x%x)", eport_id, equeue_id);

	std::lock_guard lock(id_manager::g_mutex);

	const auto port = idm::check_unlocked<lv2_obj, lv2_event_port>(eport_id);

	if (!port || !idm::check_unlocked<lv2_obj, lv2_event_queue>(equeue_id))
	{
		return CELL_ESRCH;
	}

	if (port->type != SYS_EVENT_PORT_LOCAL)
	{
		return CELL_EINVAL;
	}

	if (lv2_obj::check(port->queue))
	{
		return CELL_EISCONN;
	}

	port->queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(equeue_id);

	return CELL_OK;
}

error_code sys_event_port_connect_ipc(ppu_thread& ppu, u32 eport_id, u64 ipc_key)
{
	ppu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_connect_ipc(eport_id=0x%x, ipc_key=0x%x)", eport_id, ipc_key);

	if (ipc_key == 0)
	{
		return CELL_EINVAL;
	}

	auto queue = lv2_event_queue::find(ipc_key);

	std::lock_guard lock(id_manager::g_mutex);

	const auto port = idm::check_unlocked<lv2_obj, lv2_event_port>(eport_id);

	if (!port || !queue)
	{
		return CELL_ESRCH;
	}

	if (port->type != SYS_EVENT_PORT_IPC)
	{
		return CELL_EINVAL;
	}

	if (lv2_obj::check(port->queue))
	{
		return CELL_EISCONN;
	}

	port->queue = std::move(queue);

	return CELL_OK;
}

error_code sys_event_port_disconnect(ppu_thread& ppu, u32 eport_id)
{
	ppu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_disconnect(eport_id=0x%x)", eport_id);

	std::lock_guard lock(id_manager::g_mutex);

	const auto port = idm::check_unlocked<lv2_obj, lv2_event_port>(eport_id);

	if (!port)
	{
		return CELL_ESRCH;
	}

	if (!lv2_obj::check(port->queue))
	{
		return CELL_ENOTCONN;
	}

	// TODO: return CELL_EBUSY if necessary (can't detect the condition)

	port->queue.reset();

	return CELL_OK;
}

error_code sys_event_port_send(u32 eport_id, u64 data1, u64 data2, u64 data3)
{
	if (auto cpu = get_current_cpu_thread())
	{
		cpu->state += cpu_flag::wait;
	}

	sys_event.trace("sys_event_port_send(eport_id=0x%x, data1=0x%llx, data2=0x%llx, data3=0x%llx)", eport_id, data1, data2, data3);

	const auto port = idm::get<lv2_obj, lv2_event_port>(eport_id, [&](lv2_event_port& port) -> CellError
	{
		if (lv2_obj::check(port.queue))
		{
			const u64 source = port.name ? port.name : (s64{process_getpid()} << 32) | u64{eport_id};

			return port.queue->send(source, data1, data2, data3);
		}

		return CELL_ENOTCONN;
	});

	if (!port)
	{
		return CELL_ESRCH;
	}

	if (port.ret)
	{
		if (port.ret == CELL_EBUSY)
		{
			return not_an_error(CELL_EBUSY);
		}

		return port.ret;
	}

	return CELL_OK;
}
