#pragma once

struct sys_lwmutex_attribute_t
{
	be_t<u32> protocol;
	be_t<u32> recursive;

	union
	{
		char name[8];
		u64 name_u64;
	};
};

enum : u32
{
	lwmutex_zero     = 0u,
	lwmutex_free     = 0u - 1u,
	lwmutex_dead     = 0u - 2u,
	lwmutex_reserved = 0u - 3u,
};

namespace lwmutex
{
	template<u32 _value>
	struct const_be_u32_t
	{
		operator const be_t<u32>() const
		{
			return be_t<u32>::make(_value);
		}
	};

	static const_be_u32_t<lwmutex_zero> zero;
	static const_be_u32_t<lwmutex_free> free;
	static const_be_u32_t<lwmutex_dead> dead;
	static const_be_u32_t<lwmutex_reserved> reserved;
}

struct sys_lwmutex_t
{
	struct sync_var_t
	{
		be_t<u32> owner;
		be_t<u32> waiter;
	};

	union
	{
		atomic_be_t<sync_var_t> lock_var;

		struct
		{
			atomic_be_t<u32> owner;
			atomic_be_t<u32> waiter;
		};

		atomic_be_t<u64> all_info;
	};
	
	be_t<u32> attribute;
	be_t<u32> recursive_count;
	be_t<u32> sleep_queue; // lwmutex pseudo-id
	be_t<u32> pad;
};

struct lv2_lwmutex_t
{
	const u32 protocol;
	const u64 name;

	// this object is not truly a mutex and its syscall names are wrong, it's probabably sleep queue or something
	std::atomic<u32> signaled;

	// TODO: use sleep queue, possibly remove condition variable
	std::condition_variable cv;
	std::atomic<u32> waiters;

	lv2_lwmutex_t(u32 protocol, u64 name)
		: protocol(protocol)
		, name(name)
		, signaled(0)
		, waiters(0)
	{
	}
};

REG_ID_TYPE(lv2_lwmutex_t, 0x95); // SYS_LWMUTEX_OBJECT

// Aux
void lwmutex_create(sys_lwmutex_t& lwmutex, bool recursive, u32 protocol, u64 name);

class PPUThread;

// SysCalls
s32 _sys_lwmutex_create(vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, u32 arg4, u64 name, u32 arg6);
s32 _sys_lwmutex_destroy(u32 lwmutex_id);
s32 _sys_lwmutex_lock(u32 lwmutex_id, u64 timeout);
s32 _sys_lwmutex_trylock(u32 lwmutex_id);
s32 _sys_lwmutex_unlock(u32 lwmutex_id);
