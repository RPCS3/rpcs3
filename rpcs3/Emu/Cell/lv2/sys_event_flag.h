#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

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
		nse_t<u64, 1> name_u64;
		char name[sizeof(u64)];
	};
};

struct lv2_event_flag final : lv2_obj
{
	static const u32 id_base = 0x98000000;

	const lv2_protocol protocol;
	const u64 key;
	const s32 type;
	const u64 name;

	shared_mutex mutex;
	atomic_t<u64> pattern;
	ppu_thread* sq{};

	lv2_event_flag(u32 protocol, u64 key, s32 type, u64 name, u64 pattern) noexcept
		: protocol{static_cast<u8>(protocol)}
		, key(key)
		, type(type)
		, name(name)
		, pattern(pattern)
	{
	}

	lv2_event_flag(utils::serial& ar);
	static std::function<void(void*)> load(utils::serial& ar);
	void save(utils::serial& ar);

	// Check mode arg
	static bool check_mode(u32 mode)
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

	// Check and clear pattern (must be atomic op)
	static bool check_pattern(u64& pattern, u64 bitptn, u64 mode, u64* result)
	{
		// Write pattern
		if (result)
		{
			*result = pattern;
		}

		// Check pattern
		if (((mode & 0xf) == SYS_EVENT_FLAG_WAIT_AND && (pattern & bitptn) != bitptn) ||
			((mode & 0xf) == SYS_EVENT_FLAG_WAIT_OR && (pattern & bitptn) == 0))
		{
			return false;
		}

		// Clear pattern if necessary
		if ((mode & ~0xf) == SYS_EVENT_FLAG_WAIT_CLEAR)
		{
			pattern &= ~bitptn;
		}
		else if ((mode & ~0xf) == SYS_EVENT_FLAG_WAIT_CLEAR_ALL)
		{
			pattern = 0;
		}

		return true;
	}
};

// Aux
class ppu_thread;

// Syscalls

error_code sys_event_flag_create(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<sys_event_flag_attribute_t> attr, u64 init);
error_code sys_event_flag_destroy(ppu_thread& ppu, u32 id);
error_code sys_event_flag_wait(ppu_thread& ppu, u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result, u64 timeout);
error_code sys_event_flag_trywait(ppu_thread& ppu, u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result);
error_code sys_event_flag_set(cpu_thread& cpu, u32 id, u64 bitptn);
error_code sys_event_flag_clear(ppu_thread& ppu, u32 id, u64 bitptn);
error_code sys_event_flag_cancel(ppu_thread& ppu, u32 id, vm::ptr<u32> num);
error_code sys_event_flag_get(ppu_thread& ppu, u32 id, vm::ptr<u64> flags);
