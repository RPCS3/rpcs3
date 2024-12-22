#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

#include <deque>

class cpu_thread;
class spu_thrread;

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

// Event Queue Ipc Key
enum : u64
{
	SYS_EVENT_QUEUE_LOCAL = 0,
};

// Event Port Type
enum : s32
{
	SYS_EVENT_PORT_LOCAL = 1,
	SYS_EVENT_PORT_IPC   = 3, // Unofficial name
};

// Event Port Name
enum : u64
{
	SYS_EVENT_PORT_NO_NAME = 0,
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
	SYS_SPU_THREAD_EVENT_USER_KEY      = 0xFFFFFFFF53505501ull,
	SYS_SPU_THREAD_EVENT_DMA_KEY       = 0xFFFFFFFF53505502ull,
	SYS_SPU_THREAD_EVENT_EXCEPTION_KEY = 0xFFFFFFFF53505503ull,
};

struct sys_event_queue_attribute_t
{
	be_t<u32> protocol; // SYS_SYNC_PRIORITY or SYS_SYNC_FIFO
	be_t<s32> type; // SYS_PPU_QUEUE or SYS_SPU_QUEUE

	union
	{
		nse_t<u64, 1> name_u64;
		char name[sizeof(u64)];
	};
};

struct sys_event_t
{
	be_t<u64> source;
	be_t<u64> data1;
	be_t<u64> data2;
	be_t<u64> data3;
};

// Source, data1, data2, data3
using lv2_event = std::tuple<u64, u64, u64, u64>;

struct lv2_event_port;

struct lv2_event_queue final : public lv2_obj
{
	static const u32 id_base = 0x8d000000;

	const u32 id;
	const lv2_protocol protocol;
	const u8 type;
	const u8 size;
	const u64 name;
	const u64 key;

	shared_mutex mutex;
	std::deque<lv2_event> events;
	spu_thread* sq{};
	ppu_thread* pq{};

	lv2_event_queue(u32 protocol, s32 type, s32 size, u64 name, u64 ipc_key) noexcept;

	lv2_event_queue(utils::serial& ar) noexcept;
	static std::function<void(void*)> load(utils::serial& ar);
	void save(utils::serial& ar);
	static void save_ptr(utils::serial&, lv2_event_queue*);
	static shared_ptr<lv2_event_queue> load_ptr(utils::serial& ar, shared_ptr<lv2_event_queue>& queue, std::string_view msg = {});

	CellError send(lv2_event event, bool* notified_thread = nullptr, lv2_event_port* port = nullptr);

	CellError send(u64 source, u64 d1, u64 d2, u64 d3, bool* notified_thread = nullptr, lv2_event_port* port = nullptr)
	{
		return send(std::make_tuple(source, d1, d2, d3), notified_thread, port);
	}

	// Get event queue by its global key
	static shared_ptr<lv2_event_queue> find(u64 ipc_key);
};

struct lv2_event_port final : lv2_obj
{
	static const u32 id_base = 0x0e000000;

	const s32 type; // Port type, either IPC or local
	const u64 name; // Event source (generated from id and process id if not set)

	atomic_t<usz> is_busy = 0; // Counts threads waiting on event sending
	shared_ptr<lv2_event_queue> queue; // Event queue this port is connected to

	lv2_event_port(s32 type, u64 name)
		: type(type)
		, name(name)
	{
	}

	lv2_event_port(utils::serial& ar);
	void save(utils::serial& ar);
};

class ppu_thread;

// Syscalls

error_code sys_event_queue_create(cpu_thread& cpu, vm::ptr<u32> equeue_id, vm::ptr<sys_event_queue_attribute_t> attr, u64 event_queue_key, s32 size);
error_code sys_event_queue_destroy(ppu_thread& ppu, u32 equeue_id, s32 mode);
error_code sys_event_queue_receive(ppu_thread& ppu, u32 equeue_id, vm::ptr<sys_event_t> dummy_event, u64 timeout);
error_code sys_event_queue_tryreceive(ppu_thread& ppu, u32 equeue_id, vm::ptr<sys_event_t> event_array, s32 size, vm::ptr<u32> number);
error_code sys_event_queue_drain(ppu_thread& ppu, u32 event_queue_id);

error_code sys_event_port_create(cpu_thread& cpu, vm::ptr<u32> eport_id, s32 port_type, u64 name);
error_code sys_event_port_destroy(ppu_thread& ppu, u32 eport_id);
error_code sys_event_port_connect_local(cpu_thread& cpu, u32 event_port_id, u32 event_queue_id);
error_code sys_event_port_connect_ipc(ppu_thread& ppu, u32 eport_id, u64 ipc_key);
error_code sys_event_port_disconnect(ppu_thread& ppu, u32 eport_id);
error_code sys_event_port_send(u32 event_port_id, u64 data1, u64 data2, u64 data3);
