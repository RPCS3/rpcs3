#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

#include "Emu/Cell/PPUThread.h"

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

class ppu_thread;

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
	atomic_t<u32> lock_count{0}; // Recursive Locks

	struct alignas(16) control_data_t
	{
		u32 owner{};
		u32 reserved{};
		ppu_thread* sq{};
	};

	atomic_t<control_data_t> control{};

	lv2_mutex(u32 protocol, u32 recursive,u32 adaptive, u64 key, u64 name) noexcept
		: protocol{static_cast<u8>(protocol)}
		, recursive(recursive)
		, adaptive(adaptive)
		, key(key)
		, name(name)
	{
	}

	lv2_mutex(utils::serial& ar);
	static std::function<void(void*)> load(utils::serial& ar);
	void save(utils::serial& ar);

	template <typename T>
	CellError try_lock(T& cpu)
	{
		auto it = control.load();

		if (!it.owner)
		{
			auto store = it;
			store.owner = cpu.id;
			if (!control.compare_and_swap_test(it, store))
			{
				return CELL_EBUSY;
			}

			return {};
		}

		if (it.owner == cpu.id)
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

		return CELL_EBUSY;
	}

	template <typename T>
	bool try_own(T& cpu)
	{
		if (control.atomic_op([&](control_data_t& data)
		{
			if (data.owner)
			{
				cpu.prio.atomic_op([tag = ++g_priority_order_tag](std::common_type_t<decltype(T::prio)>& prio)
				{
					prio.order = tag;
				});

				cpu.next_cpu = data.sq;
				data.sq = &cpu;
				return false;
			}
			else
			{
				data.owner = cpu.id;
				return true;
			}
		}))
		{
			cpu.next_cpu = nullptr;
			return true;
		}

		return false;
	}

	template <typename T>
	CellError try_unlock(T& cpu)
	{
		auto it = control.load();

		if (it.owner != cpu.id)
		{
			return CELL_EPERM;
		}

		if (lock_count)
		{
			lock_count--;
			return {};
		}

		if (!it.sq)
		{
			auto store = it;
			store.owner = 0;

			if (control.compare_and_swap_test(it, store))
			{
				return {};
			}
		}

		return CELL_EBUSY;
	}

	template <typename T>
	T* reown()
	{
		T* res{};

		control.fetch_op([&](control_data_t& data)
		{
			res = nullptr;

			if (auto sq = static_cast<T*>(data.sq))
			{
				res = schedule<T>(data.sq, protocol, false);

				if (sq == data.sq)
				{
					atomic_storage<u32>::release(control.raw().owner, res->id);
					return false;
				}

				data.owner = res->id;
				return true;
			}
			else
			{
				data.owner = 0;
				return true;
			}
		});

		if (res && cpu_flag::again - res->state)
		{
			// Detach manually (fetch_op can fail, so avoid side-effects  on the first node in this case)
			res->next_cpu = nullptr;
		}

		return res;
	}
};


// Syscalls

error_code sys_mutex_create(ppu_thread& ppu, vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute_t> attr);
error_code sys_mutex_destroy(ppu_thread& ppu, u32 mutex_id);
error_code sys_mutex_lock(ppu_thread& ppu, u32 mutex_id, u64 timeout);
error_code sys_mutex_trylock(ppu_thread& ppu, u32 mutex_id);
error_code sys_mutex_unlock(ppu_thread& ppu, u32 mutex_id);
