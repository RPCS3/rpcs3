#include "stdafx.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/ErrorCodes.h"

#include "sys_config.h"

LOG_CHANNEL(sys_config);


// Enums
template<>
void fmt_class_string<sys_config_service_id>::format(std::string& out, u64 id)
{
	const s64 s_id = static_cast<s64>(id);

	switch (s_id)
	{
	case SYS_CONFIG_SERVICE_PADMANAGER   : out += "SYS_CONFIG_SERVICE_PADMANAGER"; return;
	case SYS_CONFIG_SERVICE_PADMANAGER2  : out += "SYS_CONFIG_SERVICE_PADMANAGER2"; return;
	case SYS_CONFIG_SERVICE_USER_LIBPAD  : out += "SYS_CONFIG_SERVICE_USER_LIBPAD"; return;
	case SYS_CONFIG_SERVICE_USER_LIBKB   : out += "SYS_CONFIG_SERVICE_USER_LIBKB"; return;
	case SYS_CONFIG_SERVICE_USER_LIBMOUSE: out += "SYS_CONFIG_SERVICE_USER_LIBMOUSE"; return;
	}

	if (s_id < 0)
	{
		fmt::append(out, "SYS_CONFIG_SERVICE_USER_%llx", id & ~(1ull << 63));
	}
	else
	{
		fmt::append(out, "SYS_CONFIG_SERVICE_%llx", id);
	}
}

template<>
void fmt_class_string<sys_config_service_listener_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
			STR_CASE(SYS_CONFIG_SERVICE_LISTENER_ONCE);
			STR_CASE(SYS_CONFIG_SERVICE_LISTENER_REPEATING);
		}

		return unknown;
	});
}

// Utilities
void dump_buffer(std::string& out, const std::vector<u8>& buffer)
{
	if (!buffer.empty())
	{
		out.reserve(out.size() + buffer.size() * 2 + 1);
		fmt::append(out, "0x");

		for (u8 x : buffer)
		{
			fmt::append(out, "%02x", x);
		}
	}
	else
	{
		fmt::append(out, "EMPTY");
	}
}


// LV2 Config
void lv2_config::initialize()
{
	if (m_state || !m_state.compare_and_swap_test(0, 1))
	{
		return;
	}

	// Register padmanager service, notifying vsh that a controller is connected
	static const u8 hid_info[0x1a] = {
		0x01, 0x01, //  2 unk
		0x02, 0x02, //  4
		0x00, 0x00, //  6
		0x00, 0x00, //  8
		0x00, 0x00, // 10
		0x05, 0x4c, // 12 vid
		0x02, 0x68, // 14 pid
		0x00, 0x10, // 16 unk2
		0x91, 0x88, // 18
		0x04, 0x00, // 20
		0x00, 0x07, // 22
		0x00, 0x00, // 24
		0x00, 0x00  // 26
	};

	// user_id for the padmanager seems to signify the controller port number, and the buffer contains some sort of HID descriptor
	lv2_config_service::create(SYS_CONFIG_SERVICE_PADMANAGER , 0, 1, 0, hid_info, 0x1a)->notify();
	lv2_config_service::create(SYS_CONFIG_SERVICE_PADMANAGER2, 0, 1, 0, hid_info, 0x1a)->notify();
}

void lv2_config::add_service_event(shared_ptr<lv2_config_service_event> event)
{
	std::lock_guard lock(m_mutex);
	events.emplace(event->id, std::move(event));
}

void lv2_config::remove_service_event(u32 id)
{
	shared_ptr<lv2_config_service_event> ptr;

	std::lock_guard lock(m_mutex);

	if (auto it = events.find(id); it != events.end())
	{
		ptr = std::move(it->second);
		events.erase(it);
	}
}

lv2_config_service_event& lv2_config_service_event::operator=(thread_state s) noexcept
{
	if (s == thread_state::destroying_context && !m_destroyed.exchange(true))
	{
		if (auto global = g_fxo->try_get<lv2_config>())
		{
			global->remove_service_event(id);
		}
	}

	return *this;
}

lv2_config_service_event::~lv2_config_service_event() noexcept
{
	operator=(thread_state::destroying_context);
}

lv2_config::~lv2_config() noexcept
{
	for (auto& [key, event] : events)
	{
		if (event)
		{
			// Avoid collision with lv2_config_service_event destructor
			event->m_destroyed = true;
		}
	}
}

// LV2 Config Service Listener
bool lv2_config_service_listener::check_service(const lv2_config_service& service) const
{
	// Filter by type
	if (type == SYS_CONFIG_SERVICE_LISTENER_ONCE && !service_events.empty())
	{
		return false;
	}

	// Filter by service ID or verbosity
	if (service_id != service.id || min_verbosity > service.verbosity)
	{
		return false;
	}

	// realhw only seems to send the pad connected events to the listeners that provided 0x01 as the first byte of their data buffer
	// TODO: Figure out how this filter works more properly
	if (service_id == SYS_CONFIG_SERVICE_PADMANAGER && (data.empty() || data[0] != 0x01))
	{
		return false;
	}

	// Event applies to this listener!
	return true;
}

bool lv2_config_service_listener::notify(const shared_ptr<lv2_config_service_event>& event)
{
	service_events.emplace_back(event);
	return event->notify();
}

bool lv2_config_service_listener::notify(const shared_ptr<lv2_config_service>& service)
{
	if (!check_service(*service))
		return false;

	// Create service event and notify queue!
	const auto event = lv2_config_service_event::create(handle, service, *this);
	return notify(event);
}

void lv2_config_service_listener::notify_all()
{
	std::vector<shared_ptr<lv2_config_service>> services;

	// Grab all events
	idm::select<lv2_config_service>([&](u32 /*id*/, lv2_config_service& service)
	{
		if (check_service(service))
		{
			services.push_back(service.get_shared_ptr());
		}
	});

	// Sort services by timestamp
	sort(services.begin(), services.end(), [](const shared_ptr<lv2_config_service>& s1, const shared_ptr<lv2_config_service>& s2)
	{
		return s1->timestamp < s2->timestamp;
	});

	// Notify listener (now with services in sorted order)
	for (auto& service : services)
	{
		this->notify(service);
	}
}


// LV2 Config Service
void lv2_config_service::unregister()
{
	registered = false;

	// Notify listeners
	notify();

	// Allow this object to be destroyed by withdrawing it from the IDM
	// Note that it won't be destroyed while there are service events that hold a reference to it
	idm::remove<lv2_config_service>(idm_id);
}

void lv2_config_service::notify() const
{
	std::vector<shared_ptr<lv2_config_service_listener>> listeners;

	const shared_ptr<lv2_config_service> sptr = get_shared_ptr();

	idm::select<lv2_config_service_listener>([&](u32 /*id*/, lv2_config_service_listener& listener)
	{
		if (listener.check_service(*sptr))
			listeners.push_back(listener.get_shared_ptr());
	});

	for (auto& listener : listeners)
	{
		listener->notify(sptr);
	}
}

bool lv2_config_service_event::notify() const
{
	const auto _handle = handle;

	if (!_handle)
	{
		return false;
	}

	// Send event
	return _handle->notify(SYS_CONFIG_EVENT_SOURCE_SERVICE, (static_cast<u64>(service->is_registered()) << 32) | id, service->get_size());
}


// LV2 Config Service Event
void lv2_config_service_event::write(sys_config_service_event_t *dst) const
{
	const auto registered = service->is_registered();

	dst->service_listener_handle = listener.get_id();
	dst->registered = registered;
	dst->service_id = service->id;
	dst->user_id = service->user_id;

	if (registered)
	{
		dst->verbosity = service->verbosity;
		dst->padding = service->padding;

		const auto size = service->data.size();
		dst->data_size = static_cast<u32>(size);
		memcpy(dst->data, service->data.data(), size);
	}
}




/*
 * Syscalls
 */
error_code sys_config_open(u32 equeue_hdl, vm::ptr<u32> out_config_hdl)
{
	sys_config.trace("sys_config_open(equeue_hdl=0x%x, out_config_hdl=*0x%x)", equeue_hdl, out_config_hdl);

	// Find queue with the given ID
	const auto queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(equeue_hdl);
	if (!queue)
	{
		return CELL_ESRCH;
	}

	// Initialize lv2_config global state
	auto& global = g_fxo->get<lv2_config>();
	if (true)
	{
		global.initialize();
	}

	// Create a lv2_config_handle object
	const auto config = lv2_config_handle::create(std::move(queue));
	if (config)
	{
		*out_config_hdl = idm::last_id();
		return CELL_OK;
	}

	// Failed to allocate sys_config object
	return CELL_EAGAIN;
}

error_code sys_config_close(u32 config_hdl)
{
	sys_config.trace("sys_config_close(config_hdl=0x%x)", config_hdl);

	if (!idm::remove<lv2_config_handle>(config_hdl))
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}



error_code sys_config_get_service_event(u32 config_hdl, u32 event_id, vm::ptr<sys_config_service_event_t> dst, u64 size)
{
	sys_config.trace("sys_config_get_service_event(config_hdl=0x%x, event_id=0x%llx, dst=*0x%llx, size=0x%llx)", config_hdl, event_id, dst, size);

	// Find sys_config handle object with the given ID
	const auto cfg = idm::get_unlocked<lv2_config_handle>(config_hdl);
	if (!cfg)
	{
		return CELL_ESRCH;
	}

	// Find service_event object
	const auto event = g_fxo->get<lv2_config>().find_event(event_id);
	if (!event)
	{
		return CELL_ESRCH;
	}

	// Check buffer fits
	if (!event->check_buffer_size(size))
	{
		return CELL_EAGAIN;
	}

	// Write event to buffer
	event->write(dst.get_ptr());

	return CELL_OK;
}



error_code sys_config_add_service_listener(u32 config_hdl, sys_config_service_id service_id, u64 min_verbosity, vm::ptr<void> in, u64 size, sys_config_service_listener_type type, vm::ptr<u32> out_listener_hdl)
{
	sys_config.trace("sys_config_add_service_listener(config_hdl=0x%x, service_id=0x%llx, min_verbosity=0x%llx, in=*0x%x, size=%lld, type=0x%llx, out_listener_hdl=*0x%x)", config_hdl, service_id, min_verbosity, in, size, type, out_listener_hdl);

	// Find sys_config handle object with the given ID
	auto cfg = idm::get_unlocked<lv2_config_handle>(config_hdl);
	if (!cfg)
	{
		return CELL_ESRCH;
	}

	// Create service listener
	const auto listener = lv2_config_service_listener::create(cfg, service_id, min_verbosity, type, static_cast<u8*>(in.get_ptr()), size);
	if (!listener)
	{
		return CELL_EAGAIN;
	}

	if (size > 0)
	{
		std::string buf_str;
		dump_buffer(buf_str, listener->data);
		sys_config.todo("Registered service listener for service %llx with non-zero buffer: %s", service_id, buf_str.c_str());
	}

	// Notify listener with all past events
	listener->notify_all();

	// Done!
	*out_listener_hdl = listener->get_id();
	return CELL_OK;
}

error_code sys_config_remove_service_listener(u32 config_hdl, u32 listener_hdl)
{
	sys_config.trace("sys_config_remove_service_listener(config_hdl=0x%x, listener_hdl=0x%x)", config_hdl, listener_hdl);

	// Remove listener from IDM
	if (!idm::remove<lv2_config_service_listener>(listener_hdl))
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}



error_code sys_config_register_service(u32 config_hdl, sys_config_service_id service_id, u64 user_id, u64 verbosity, vm::ptr<u8> data_buf, u64 size, vm::ptr<u32> out_service_hdl)
{
	sys_config.trace("sys_config_register_service(config_hdl=0x%x, service_id=0x%llx, user_id=0x%llx, verbosity=0x%llx, data_but=*0x%llx, size=%lld, out_service_hdl=*0x%llx)", config_hdl, service_id, user_id, verbosity, data_buf, size, out_service_hdl);

	// Find sys_config handle object with the given ID
	const auto cfg = idm::get_unlocked<lv2_config_handle>(config_hdl);
	if (!cfg)
	{
		return CELL_ESRCH;
	}

	// Create service
	const auto service = lv2_config_service::create(service_id, user_id, verbosity, 0, data_buf.get_ptr(), size);
	if (!service)
	{
		return CELL_EAGAIN;
	}

	// Notify all listeners
	service->notify();

	// Done!
	*out_service_hdl = service->get_id();
	return CELL_OK;
}

error_code sys_config_unregister_service(u32 config_hdl, u32 service_hdl)
{
	sys_config.trace("sys_config_unregister_service(config_hdl=0x%x, service_hdl=0x%x)", config_hdl, service_hdl);

	// Remove listener from IDM
	auto service = idm::withdraw<lv2_config_service>(service_hdl);
	if (!service)
	{
		return CELL_ESRCH;
	}

	// Unregister service
	service->unregister();

	// Done!
	return CELL_OK;
}



/*
 * IO Events - TODO
 */
error_code sys_config_get_io_event(u32 config_hdl, u32 event_id /*?*/, vm::ptr<void> out_buf /*?*/, u64 size /*?*/)
{
	sys_config.todo("sys_config_get_io_event(config_hdl=0x%x, event_id=0x%x, out_buf=*0x%x, size=%lld)", config_hdl, event_id, out_buf, size);
	return CELL_OK;
}

error_code sys_config_register_io_error_listener(u32 config_hdl)
{
	sys_config.todo("sys_config_register_io_error_listener(config_hdl=0x%x)", config_hdl);
	return CELL_OK;
}

error_code sys_config_unregister_io_error_listener(u32 config_hdl)
{
	sys_config.todo("sys_config_unregister_io_error_listener(config_hdl=0x%x)", config_hdl);
	return CELL_OK;
}
