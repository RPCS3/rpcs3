#pragma once

// Event Queue Type
enum : u32
{
	SYS_PPU_QUEUE = 1,
	SYS_SPU_QUEUE = 2,
};

// Event Queue Destroy Mode
enum : s32
{
	SYS_EVENT_QUEUE_DESTROY_FORCE = 1,
};

// Event Port Type
enum : s32
{
	SYS_EVENT_PORT_LOCAL = 1,
};

// Event Source Type
enum : u32
{
	SYS_SPU_THREAD_EVENT_USER = 1,
	SYS_SPU_THREAD_EVENT_DMA  = 2, // not supported
};

// Event Source Key
enum : u64
{
	SYS_SPU_THREAD_EVENT_USER_KEY = 0xFFFFFFFF53505501,
	SYS_SPU_THREAD_EVENT_DMA_KEY  = 0xFFFFFFFF53505502, // ???
};

struct sys_event_queue_attr
{
	be_t<u32> protocol; // SYS_SYNC_PRIORITY or SYS_SYNC_FIFO
	be_t<s32> type; // SYS_PPU_QUEUE or SYS_SPU_QUEUE

	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct sys_event_t
{
	be_t<u64> source;
	be_t<u64> data1;
	be_t<u64> data2;
	be_t<u64> data3;
};

struct event_t
{
	u64 source;
	u64 data1;
	u64 data2;
	u64 data3;

	event_t(u64 source, u64 data1, u64 data2, u64 data3)
		: source(source)
		, data1(data1)
		, data2(data2)
		, data3(data3)
	{
	}
};

struct event_queue_t
{
	const u32 protocol;
	const s32 type;
	const u64 name;
	const u64 key;
	const s32 size;

	std::deque<event_t> events;

	// TODO: use sleep queue, remove condition variable (use thread's one instead)
	std::condition_variable cv;
	std::atomic<s32> waiters;

	event_queue_t(u32 protocol, s32 type, u64 name, u64 key, s32 size)
		: protocol(protocol)
		, type(type)
		, name(name)
		, key(key)
		, size(size)
		, waiters(0)
	{
	}
};

struct event_port_t
{
	const s32 type; // port type, must be SYS_EVENT_PORT_LOCAL
	const u64 name; // passed as event source (generated from id and process id if not set)
	std::weak_ptr<event_queue_t> queue; // event queue this port is connected to

	event_port_t(s32 type, u64 name)
		: type(type)
		, name(name)
	{
	}
};

class PPUThread;

// Aux
u32 event_port_create(u64 name);
u32 event_queue_create(u32 protocol, s32 type, u64 name_u64, u64 event_queue_key, s32 size);

// SysCalls
s32 sys_event_queue_create(vm::ptr<u32> equeue_id, vm::ptr<sys_event_queue_attr> attr, u64 event_queue_key, s32 size);
s32 sys_event_queue_destroy(u32 equeue_id, s32 mode);
s32 sys_event_queue_receive(PPUThread& CPU, u32 equeue_id, vm::ptr<sys_event_t> dummy_event, u64 timeout);
s32 sys_event_queue_tryreceive(u32 equeue_id, vm::ptr<sys_event_t> event_array, s32 size, vm::ptr<u32> number);
s32 sys_event_queue_drain(u32 event_queue_id);

s32 sys_event_port_create(vm::ptr<u32> eport_id, s32 port_type, u64 name);
s32 sys_event_port_destroy(u32 eport_id);
s32 sys_event_port_connect_local(u32 event_port_id, u32 event_queue_id);
s32 sys_event_port_disconnect(u32 eport_id);
s32 sys_event_port_send(u32 event_port_id, u64 data1, u64 data2, u64 data3);
