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

	const lv2_protocol protocol;
	const u32 recursive;
	const u32 adaptive;
	const u64 key;
	const u64 name;

	u32 cond_count = 0; // Condition Variables
	shared_mutex mutex;
	atomic_t<u32> owner{0};
	atomic_t<u32> lock_count{0}; // Recursive Locks
	atomic_t<ppu_thread*> sq{};

	lv2_mutex(u32 protocol, u32 recursive,u32 adaptive, u64 key, u64 name) noexcept
		: protocol{static_cast<u8>(protocol)}
		, recursive(recursive)
		, adaptive(adaptive)
		, key(key)
		, name(name)
	{
	}

	lv2_mutex(utils::serial& ar);
	static std::shared_ptr<void> load(utils::serial& ar); 
	void save(utils::serial& ar);

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

	template <typename T>
	bool try_own(T& cpu, u32 id)
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
			lv2_obj::emplace(sq, &cpu);
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
			if (cpu->state & cpu_flag::again)
			{
				return static_cast<T*>(cpu);
			}

			owner = cpu->id << 1 | !!sq;
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
