#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

struct sys_mutex_attribute_t
{
	be_t<u32> protocol; // SYS_SYNC_FIFO, SYS_SYNC_PRIORITY or SYS_SYNC_PRIORITY_INHERIT
	be_t<u32> recursive; // SYS_SYNC_RECURSIVE or SYS_SYNC_NOT_RECURSIVE
	be_t<u32> pshared;
	be_t<u32> adaptive;
	be_t<u64> ipc_key;
	be_t<s32> flags;
	be_t<u32> pad;

	union
	{
		nse_t<u64, 1> name_u64;
		char name[sizeof(u64)];
	};
};

struct lv2_mutex final : lv2_obj
{
	static const u32 id_base = 0x85000000;

	const u32 protocol;
	const u32 recursive;
	const u32 shared;
	const u32 adaptive;
	const u64 key;
	const u64 name;
	const s32 flags;

	shared_mutex mutex;
	atomic_t<u32> owner{0}; // Owner Thread ID
	atomic_t<u32> lock_count{0}; // Recursive Locks
	atomic_t<u32> cond_count{0}; // Condition Variables
	std::deque<cpu_thread*> sq;

	lv2_mutex(u32 protocol, u32 recursive, u32 shared, u32 adaptive, u64 key, s32 flags, u64 name)
		: protocol(protocol)
		, recursive(recursive)
		, shared(shared)
		, adaptive(adaptive)
		, key(key)
		, name(name)
		, flags(flags)
	{
	}

	CellError try_lock(u32 id)
	{
		const u32 value = owner;

		if (value >> 1 == id)
		{
			// Recursive locking
			if (recursive == SYS_SYNC_RECURSIVE)
			{
				if (lock_count == 0xffffffffu)
				{
					return CELL_EKRESOURCE;
				}

				lock_count++;
				return {};
			}

			return CELL_EDEADLK;
		}

		if (value == 0)
		{
			if (owner.compare_and_swap_test(0, id << 1))
			{
				return {};
			}
		}

		return CELL_EBUSY;
	}

	bool try_own(cpu_thread& cpu, u32 id)
	{
		if (owner.fetch_op([&](u32& val)
		{
			if (val == 0)
			{
				val = id << 1;
			}
			else
			{
				val |= 1;
			}
		}))
		{
			sq.emplace_back(&cpu);
			return false;
		}

		return true;
	}

	CellError try_unlock(u32 id)
	{
		const u32 value = owner;

		if (value >> 1 != id)
		{
			return CELL_EPERM;
		}

		if (lock_count)
		{
			lock_count--;
			return {};
		}

		if (value == id << 1)
		{
			if (owner.compare_and_swap_test(value, 0))
			{
				return {};
			}
		}

		return CELL_EBUSY;
	}

	template <typename T>
	T* reown()
	{
		if (auto cpu = schedule<T>(sq, protocol))
		{
			owner = cpu->id << 1 | !sq.empty();
			return static_cast<T*>(cpu);
		}
		else
		{
			owner = 0;
			return nullptr;
		}
	}
};

class ppu_thread;

// Syscalls

error_code sys_mutex_create(ppu_thread& ppu, vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute_t> attr);
error_code sys_mutex_destroy(ppu_thread& ppu, u32 mutex_id);
error_code sys_mutex_lock(ppu_thread& ppu, u32 mutex_id, u64 timeout);
error_code sys_mutex_trylock(ppu_thread& ppu, u32 mutex_id);
error_code sys_mutex_unlock(ppu_thread& ppu, u32 mutex_id);
