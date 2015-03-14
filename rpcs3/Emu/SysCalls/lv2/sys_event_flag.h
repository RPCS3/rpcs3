#pragma once

enum
{
	SYS_SYNC_WAITER_SINGLE = 0x10000,
	SYS_SYNC_WAITER_MULTIPLE = 0x20000,

	SYS_EVENT_FLAG_WAIT_AND = 0x01,
	SYS_EVENT_FLAG_WAIT_OR = 0x02,

	SYS_EVENT_FLAG_WAIT_CLEAR = 0x10,
	SYS_EVENT_FLAG_WAIT_CLEAR_ALL = 0x20,
};

struct sys_event_flag_attr
{
	be_t<u32> protocol;
	be_t<u32> pshared;
	be_t<u64> ipc_key;
	be_t<s32> flags;
	be_t<s32> type;

	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct event_flag_t
{
	const u32 protocol;
	const s32 type;
	const u64 name;

	std::atomic<u64> flags;
	std::atomic<u32> cancelled;

	// TODO: use sleep queue, possibly remove condition variable
	std::condition_variable cv;
	std::atomic<u32> waiters;

	event_flag_t(u64 pattern, u32 protocol, s32 type, u64 name)
		: flags(pattern)
		, protocol(protocol)
		, type(type)
		, name(name)
		, cancelled(0)
		, waiters(0)
	{
	}
};

s32 sys_event_flag_create(vm::ptr<u32> id, vm::ptr<sys_event_flag_attr> attr, u64 init);
s32 sys_event_flag_destroy(u32 id);
s32 sys_event_flag_wait(u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result, u64 timeout);
s32 sys_event_flag_trywait(u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result);
s32 sys_event_flag_set(u32 id, u64 bitptn);
s32 sys_event_flag_clear(u32 id, u64 bitptn);
s32 sys_event_flag_cancel(u32 id, vm::ptr<u32> num);
s32 sys_event_flag_get(u32 id, vm::ptr<u64> flags);
