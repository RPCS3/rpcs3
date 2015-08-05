#pragma once

#include "sleep_queue.h"

namespace vm { using namespace ps3; }

enum
{
	SYS_SYNC_WAITER_SINGLE = 0x10000,
	SYS_SYNC_WAITER_MULTIPLE = 0x20000,

	SYS_EVENT_FLAG_WAIT_AND = 0x01,
	SYS_EVENT_FLAG_WAIT_OR = 0x02,

	SYS_EVENT_FLAG_WAIT_CLEAR = 0x10,
	SYS_EVENT_FLAG_WAIT_CLEAR_ALL = 0x20,
};

struct sys_event_flag_attribute_t
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

struct lv2_event_flag_t
{
	const u32 protocol;
	const s32 type;
	const u64 name;

	std::atomic<u64> pattern;

	sleep_queue_t sq;

	lv2_event_flag_t(u64 pattern, u32 protocol, s32 type, u64 name)
		: pattern(pattern)
		, protocol(protocol)
		, type(type)
		, name(name)
	{
	}

	static inline bool check_mode(u32 mode)
	{
		switch (mode & 0xf)
		{
		case SYS_EVENT_FLAG_WAIT_AND: break;
		case SYS_EVENT_FLAG_WAIT_OR: break;
		default: return false;
		}

		switch (mode & ~0xf)
		{
		case 0: break;
		case SYS_EVENT_FLAG_WAIT_CLEAR: break;
		case SYS_EVENT_FLAG_WAIT_CLEAR_ALL: break;
		default: return false;
		}

		return true;
	}

	inline bool check_pattern(u64 bitptn, u32 mode)
	{
		if ((mode & 0xf) == SYS_EVENT_FLAG_WAIT_AND)
		{
			return (pattern & bitptn) == bitptn;
		}
		else if ((mode & 0xf) == SYS_EVENT_FLAG_WAIT_OR)
		{
			return (pattern & bitptn) != 0;
		}
		else
		{
			throw EXCEPTION("Unknown mode (0x%x)", mode);
		}
	}

	inline u64 clear_pattern(u64 bitptn, u32 mode)
	{
		if ((mode & ~0xf) == SYS_EVENT_FLAG_WAIT_CLEAR)
		{
			return pattern.fetch_and(~bitptn);
		}
		else if ((mode & ~0xf) == SYS_EVENT_FLAG_WAIT_CLEAR_ALL)
		{
			return pattern.exchange(0);
		}
		else if ((mode & ~0xf) == 0)
		{
			return pattern;
		}
		else
		{
			throw EXCEPTION("Unknown mode (0x%x)", mode);
		}
	}

	void notify_all(lv2_lock_t& lv2_lock);
};

// Aux
class PPUThread;

// SysCalls
s32 sys_event_flag_create(vm::ptr<u32> id, vm::ptr<sys_event_flag_attribute_t> attr, u64 init);
s32 sys_event_flag_destroy(u32 id);
s32 sys_event_flag_wait(PPUThread& ppu, u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result, u64 timeout);
s32 sys_event_flag_trywait(u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result);
s32 sys_event_flag_set(u32 id, u64 bitptn);
s32 sys_event_flag_clear(u32 id, u64 bitptn);
s32 sys_event_flag_cancel(u32 id, vm::ptr<u32> num);
s32 sys_event_flag_get(u32 id, vm::ptr<u64> flags);
