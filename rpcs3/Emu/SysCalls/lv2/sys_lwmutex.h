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
	lwmutex_zero = 0u,
	lwmutex_free = 0u - 1u,
	lwmutex_dead = 0u - 2u,
	lwmutex_busy = 0u - 3u,
};

namespace lwmutex
{
	template<u32 _value>
	struct const_be_u32_t
	{
		static const u32 value = _value;

		operator const be_t<u32>() const
		{
			return be_t<u32>::make(value);
		}
	};

	static const_be_u32_t<lwmutex_zero> zero;
	static const_be_u32_t<lwmutex_free> free;
	static const_be_u32_t<lwmutex_dead> dead;
	static const_be_u32_t<lwmutex_busy> busy;
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
		atomic_t<sync_var_t> lock_var;

		struct
		{
			atomic_t<u32> owner;
			atomic_t<u32> waiter;
		};
	};
	
	be_t<u32> attribute;
	be_t<u32> recursive_count;
	be_t<u32> sleep_queue; // lwmutex pseudo-id
	be_t<u32> pad;
};

struct lwmutex_t
{
	const u32 protocol;
	const u64 name;

	lwmutex_t(u32 protocol, u64 name)
		: protocol(protocol)
		, name(name)
	{
	}	
};

// Aux
void lwmutex_create(sys_lwmutex_t& lwmutex, bool recursive, u32 protocol, u64 name);

class PPUThread;

// SysCalls
s32 _sys_lwmutex_create(vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, u32 arg4, u64 name, u32 arg6);
s32 _sys_lwmutex_destroy(PPUThread& CPU, u32 lwmutex_id);
s32 _sys_lwmutex_lock(PPUThread& CPU, u32 lwmutex_id, u64 timeout);
s32 _sys_lwmutex_trylock(PPUThread& CPU, u32 lwmutex_id);
s32 _sys_lwmutex_unlock(PPUThread& CPU, u32 lwmutex_id);
