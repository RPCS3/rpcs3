#pragma once

#include "util/atomic.hpp"
#include "util/shared_ptr.hpp"
#include "Emu/Cell/timers.hpp"

/*
 * sys_config is a "subscription-based data storage API"
 *
 * It has the concept of services and listeners. Services provide data, listeners subscribe to registration/unregistration events from specific services.
 *
 * Services are divided into two classes: LV2 services (positive service IDs) and User services (negative service IDs).
 * LV2 services seem to be implictly "available", probably constructed on-demand with internal LV2 code generating the data. An example is PadManager (service ID 0x11).
 * User services may be registered through a syscall, and have negative IDs. An example is libPad (service ID 0x8000'0000'0000'0001).
 * Note that user-mode *cannot* register positive service IDs.
 *
 * To start with, you have to get a sys_config handle by calling sys_config_open and providing an event queue.
 * This event queue will be used for sys_config notifications if a subscribed config event is registered.
 *
 * With a sys_config handle, listeners can be added to specific services using sys_config_add_service_listener.
 * This syscall returns a service listener handle, which can be used to close the listener and stop further notifications.
 * Once subscribed, any matching past service registrations will be automatically sent to the supplied queue (thus the "data storage").
 *
 * Services exist "implicitly", and data may be registered *onto* a service by calling sys_config_register_service.
 * You can remove config events by calling sys_config_unregister_service and providing the handle returned when registering a service.
 *
 * If a service is registered (or unregistered) and matches any active listener, that listener will get an event sent to the event queue provided in the call to sys_config_open.
 *
 * This event will contain the type of config event ("service event" or "IO event", in event.source),
 * the corresponding sys_config handle (event.data1), the config event ID (event.data2 & 0xffff'ffff),
 * whether the service was registered or unregistered ('data2 >> 32'), and what buffer size will be needed to read the corresponding service event (event.data3).
 *
 * NOTE: if multiple listeners exist, each gets a separate event ID even though all events are the same!
 *
 * After receiving such an event from the event queue, the user should allocate enough buffer and call sys_config_get_service_event
 * (or sys_config_io_event) with the given event ID, in order to obtain a sys_config_service_event_t (or sys_config_io_event_t) structure
 * with the contents of the service that was (un)registered.
 */

class lv2_config_handle;
class lv2_config_service;
class lv2_config_service_listener;
class lv2_config_service_event;


// Known sys_config service IDs
enum sys_config_service_id : s64 {
	SYS_CONFIG_SERVICE_PADMANAGER  = 0x11,
	SYS_CONFIG_SERVICE_PADMANAGER2 = 0x12, // lv2 seems to send padmanager events to both 0x11 and 0x12
	SYS_CONFIG_SERVICE_0x20        = 0x20,
	SYS_CONFIG_SERVICE_0x30        = 0x30,

	SYS_CONFIG_SERVICE_USER_BASE     = static_cast<s64>(UINT64_C(0x8000'0000'0000'0000)),
	SYS_CONFIG_SERVICE_USER_LIBPAD   = SYS_CONFIG_SERVICE_USER_BASE +      1,
	SYS_CONFIG_SERVICE_USER_LIBKB    = SYS_CONFIG_SERVICE_USER_BASE +      2,
	SYS_CONFIG_SERVICE_USER_LIBMOUSE = SYS_CONFIG_SERVICE_USER_BASE +      3,
	SYS_CONFIG_SERVICE_USER_0x1000   = SYS_CONFIG_SERVICE_USER_BASE + 0x1000,
	SYS_CONFIG_SERVICE_USER_0x1010   = SYS_CONFIG_SERVICE_USER_BASE + 0x1010,
	SYS_CONFIG_SERVICE_USER_0x1011   = SYS_CONFIG_SERVICE_USER_BASE + 0x1011,
	SYS_CONFIG_SERVICE_USER_0x1013   = SYS_CONFIG_SERVICE_USER_BASE + 0x1013,
	SYS_CONFIG_SERVICE_USER_0x1020   = SYS_CONFIG_SERVICE_USER_BASE + 0x1020,
	SYS_CONFIG_SERVICE_USER_0x1030   = SYS_CONFIG_SERVICE_USER_BASE + 0x1030,
};

enum sys_config_service_listener_type : u32 {
	SYS_CONFIG_SERVICE_LISTENER_ONCE      = 0,
	SYS_CONFIG_SERVICE_LISTENER_REPEATING = 1
};

enum sys_config_event_source : u64 {
	SYS_CONFIG_EVENT_SOURCE_SERVICE = 1,
	SYS_CONFIG_EVENT_SOURCE_IO      = 2
};


/*
 * Dynamic-sized struct to describe a sys_config_service_event
 * We never allocate it - the guest does it for us and provides a pointer
 */
struct sys_config_service_event_t {
	// Handle to the service listener for whom this event is destined
	be_t<u32> service_listener_handle;

	// 1 if this service is currently registered or unregistered
	be_t<u32> registered;

	// Service ID that triggered this event
	be_t<u64> service_id;

	// Custom ID provided by the user, used to uniquely identify service events (provided to sys_config_register_event)
	// When a service is unregistered, this is the only value available to distinguish which service event was unregistered.
	be_t<u64> user_id;

	/* if added==0, the structure ends here */

	// Verbosity of this service event (provided to sys_config_register_event)
	be_t<u64> verbosity;

	// Size of 'data'
	be_t<u32> data_size;

	// Ignored, seems to be simply 32-bits of padding
	be_t<u32> padding;

	// Buffer containing event data (copy of the buffer supplied to sys_config_register_service)
	// NOTE: This buffer size is dynamic, according to 'data_size', and can be 0. Here it is set to 1 since zero-sized buffers are not standards-compliant
	u8 data[1];
};


/*
 * Event data structure for SYS_CONFIG_SERVICE_PADMANAGER
 * This is a guess
 */
struct sys_config_padmanager_data_t {
	be_t<u16> unk[5]; // hid device type ?
	be_t<u16> vid;
	be_t<u16> pid;
	be_t<u16> unk2[6]; // bluetooth address?
};
static_assert(sizeof(sys_config_padmanager_data_t) == 26);


/*
 * Global sys_config state
 */

class lv2_config
{
	atomic_t<u32> m_state = 0;

	// LV2 Config mutex
	shared_mutex m_mutex;

	// Map of LV2 Service Events
	std::unordered_map<u32, shared_ptr<lv2_config_service_event>> events;

public:
	void initialize();

	// Service Events
	void add_service_event(shared_ptr<lv2_config_service_event> event);
	void remove_service_event(u32 id);

	shared_ptr<lv2_config_service_event> find_event(u32 id)
	{
		reader_lock lock(m_mutex);

		const auto it = events.find(id);

		if (it == events.cend())
			return null_ptr;

		if (it->second)
		{
			return it->second;
		}

		return null_ptr;
	}

	~lv2_config() noexcept;
};

/*
 * LV2 Config Handle object, managed by IDM
 */
class lv2_config_handle
{
public:
	static const u32 id_base = 0x41000000;
	static const u32 id_step = 0x100;
	static const u32 id_count = 2048;
	SAVESTATE_INIT_POS(37);

private:
	u32 idm_id;

	// queue for service/io event notifications
	const shared_ptr<lv2_event_queue> queue;

	bool send_queue_event(u64 source, u64 d1, u64 d2, u64 d3) const
	{
		if (auto sptr = queue)
		{
			return sptr->send(source, d1, d2, d3) == 0;
		}

		return false;
	}

public:
	// Constructors (should not be used directly)
	lv2_config_handle(shared_ptr<lv2_event_queue> _queue) noexcept
		: queue(std::move(_queue))
	{
	}

	// Factory
	template <typename... Args>
	static shared_ptr<lv2_config_handle> create(Args&&... args)
	{
		if (auto cfg = idm::make_ptr<lv2_config_handle>(std::forward<Args>(args)...))
		{
			cfg->idm_id = idm::last_id();
			return cfg;
		}
		return null_ptr;
	}

	// Notify event queue for this handle
	bool notify(u64 source, u64 data2, u64 data3) const
	{
		return send_queue_event(source, idm_id, data2, data3);
	}
};

/*
 * LV2 Service object, managed by IDM
 */
class lv2_config_service
{
public:
	static const u32 id_base = 0x43000000;
	static const u32 id_step = 0x100;
	static const u32 id_count = 2048;
	SAVESTATE_INIT_POS(38);

private:
	// IDM data
	u32 idm_id;

	// Whether this service is currently registered or not
	bool registered = true;

public:
	const u64 timestamp;
	const sys_config_service_id id;

	const u64 user_id;
	const u64 verbosity;
	const u32 padding; // not used, but stored here just in case
	const std::vector<u8> data;

	// Constructors (should not be used directly)
	lv2_config_service(sys_config_service_id _id, u64 _user_id, u64 _verbosity, u32 _padding, const u8* _data, usz size) noexcept
		: timestamp(get_system_time())
		, id(_id)
		, user_id(_user_id)
		, verbosity(_verbosity)
		, padding(_padding)
		, data(&_data[0], &_data[size])
	{
	}

	// Factory
	template <typename... Args>
	static shared_ptr<lv2_config_service> create(Args&&... args)
	{
		if (auto service = idm::make_ptr<lv2_config_service>(std::forward<Args>(args)...))
		{
			service->idm_id = idm::last_id();
			return service;
		}

		return null_ptr;
	}

	// Registration
	bool is_registered() const { return registered; }
	void unregister();

	// Notify listeners
	void notify() const;

	// Utilities
	usz get_size() const { return sizeof(sys_config_service_event_t)-1 + data.size(); }
	shared_ptr<lv2_config_service> get_shared_ptr () const { return stx::make_shared_from_this<lv2_config_service>(this); }
	u32 get_id() const { return idm_id; }
};

/*
 * LV2 Service Event Listener object, managed by IDM
 */
class lv2_config_service_listener
{
public:
	static const u32 id_base = 0x42000000;
	static const u32 id_step = 0x100;
	static const u32 id_count = 2048;
	SAVESTATE_INIT_POS(39);

private:
	// IDM data
	u32 idm_id;

	// The service listener owns the service events - service events will not be freed as long as their corresponding listener exists
	// This has been confirmed to be the case in realhw
	std::vector<shared_ptr<lv2_config_service_event>> service_events;
	shared_ptr<lv2_config_handle> handle;

	bool notify(const shared_ptr<lv2_config_service_event>& event);

public:
	const sys_config_service_id service_id;
	const u64 min_verbosity;
	const sys_config_service_listener_type type;

	const std::vector<u8> data;

	// Constructors (should not be used directly)
	lv2_config_service_listener(shared_ptr<lv2_config_handle> _handle, sys_config_service_id _service_id, u64 _min_verbosity, sys_config_service_listener_type _type, const u8* _data, usz size) noexcept
		: handle(std::move(_handle))
		, service_id(_service_id)
		, min_verbosity(_min_verbosity)
		, type(_type)
		, data(&_data[0], &_data[size])
	{}

	// Factory
	template <typename... Args>
	static shared_ptr<lv2_config_service_listener> create(Args&&... args)
	{
		if (auto listener = idm::make_ptr<lv2_config_service_listener>(std::forward<Args>(args)...))
		{
			listener->idm_id = idm::last_id();
			return listener;
		}

		return null_ptr;
	}

	// Check whether service matches
	bool check_service(const lv2_config_service& service) const;

	// Register new event, and notify queue
	bool notify(const shared_ptr<lv2_config_service>& service);

	// (Re-)notify about all still-registered past events
	void notify_all();

	// Utilities
	u32 get_id() const { return idm_id; }
	shared_ptr<lv2_config_service_listener> get_shared_ptr() const { return stx::make_shared_from_this<lv2_config_service_listener>(this); }
};

/*
 * LV2 Service Event object (*not* managed by IDM)
 */
class lv2_config_service_event
{
	static u32 get_next_id()
	{
		struct service_event_id
		{
			atomic_t<u32> next_id = 0;
		};

		return g_fxo->get<service_event_id>().next_id++;
	}

	atomic_t<bool> m_destroyed = false;

	friend class lv2_config;

public:
	const u32 id;

	// Note: Events hold a shared_ptr to their corresponding service - services only get freed once there are no more pending service events
	// This has been confirmed to be the case in realhw
	const shared_ptr<lv2_config_handle> handle;
	const shared_ptr<lv2_config_service> service;
	const lv2_config_service_listener& listener;

	// Constructors (should not be used directly)
	lv2_config_service_event(shared_ptr<lv2_config_handle> _handle, shared_ptr<lv2_config_service> _service, const lv2_config_service_listener& _listener) noexcept
		: id(get_next_id())
		, handle(std::move(_handle))
		, service(std::move(_service))
		, listener(_listener)
	{
	}

	// Factory
	template <typename... Args>
	static shared_ptr<lv2_config_service_event> create(Args&&... args)
	{
		auto ev = make_shared<lv2_config_service_event>(std::forward<Args>(args)...);

		g_fxo->get<lv2_config>().add_service_event(ev);

		return ev;
	}

	// Destructor
	lv2_config_service_event& operator=(thread_state s) noexcept;
	~lv2_config_service_event() noexcept;

	// Notify queue that this event exists
	bool notify() const;

	// Write event to buffer
	void write(sys_config_service_event_t *dst) const;

	// Check if the buffer can fit the current event, return false otherwise
	bool check_buffer_size(usz size) const { return service->get_size() <= size; }
};

/*
 * Syscalls
 */
/*516*/ error_code sys_config_open(u32 equeue_hdl, vm::ptr<u32> out_config_hdl);
/*517*/ error_code sys_config_close(u32 config_hdl);
/*518*/ error_code sys_config_get_service_event(u32 config_hdl, u32 event_id, vm::ptr<sys_config_service_event_t> dst, u64 size);
/*519*/ error_code sys_config_add_service_listener(u32 config_hdl, sys_config_service_id service_id, u64 min_verbosity, vm::ptr<void> in, u64 size, sys_config_service_listener_type type, vm::ptr<u32> out_listener_hdl);
/*520*/ error_code sys_config_remove_service_listener(u32 config_hdl, u32 listener_hdl);
/*521*/ error_code sys_config_register_service(u32 config_hdl, sys_config_service_id service_id, u64 user_id, u64 verbosity, vm::ptr<u8> data_buf, u64 size, vm::ptr<u32> out_service_hdl);
/*522*/ error_code sys_config_unregister_service(u32 config_hdl, u32 service_hdl);

// Following syscalls have not been REd yet
/*523*/ error_code sys_config_get_io_event(u32 config_hdl, u32 event_id /*?*/, vm::ptr<void> out_buf /*?*/, u64 size /*?*/);
/*524*/ error_code sys_config_register_io_error_listener(u32 config_hdl);
/*525*/ error_code sys_config_unregister_io_error_listener(u32 config_hdl);
