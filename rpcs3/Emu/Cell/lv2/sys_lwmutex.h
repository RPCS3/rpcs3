#pragma once

#include "sys_sync.h"

#include "Emu/Memory/vm_ptr.h"

struct sys_lwmutex_attribute_t
{
	be_t<u32> protocol;
	be_t<u32> recursive;

	union
	{
		nse_t<u64, 1> name_u64;
		char name[sizeof(u64)];
	};
};

enum : u32
{
	lwmutex_free     = 0xffffffffu,
	lwmutex_dead     = 0xfffffffeu,
	lwmutex_reserved = 0xfffffffdu,
};

struct sys_lwmutex_t
{
	struct alignas(8) sync_var_t
	{
		be_t<u32> owner;
		be_t<u32> waiter;
	};

	union
	{
		atomic_t<sync_var_t> lock_var;

		struct
		{
			atomic_be_t<u32> owner;
			atomic_be_t<u32> waiter;
		}
		vars;

		atomic_be_t<u64> all_info;
	};

	be_t<u32> attribute;
	be_t<u32> recursive_count;
	be_t<u32> sleep_queue; // lwmutex pseudo-id
	be_t<u32> pad;
};

struct lv2_lwmutex final : lv2_obj
{
	static const u32 id_base = 0x95000000;

	const lv2_protocol protocol;
	const vm::ptr<sys_lwmutex_t> control;
	const be_t<u64> name;

	shared_mutex mutex;
	atomic_t<s32> lwcond_waiters{0};

	struct alignas(16) control_data_t
	{
		s32 signaled{0};
		u32 reserved{};
		ppu_thread* sq{};
	};

	atomic_t<control_data_t> lv2_control{};

	lv2_lwmutex(u32 protocol, vm::ptr<sys_lwmutex_t> control, u64 name) noexcept
		: protocol{static_cast<u8>(protocol)}
		, control(control)
		, name(std::bit_cast<be_t<u64>>(name))
	{
	}

	lv2_lwmutex(utils::serial& ar);
	void save(utils::serial& ar);

	ppu_thread* load_sq() const
	{
		return atomic_storage<ppu_thread*>::load(lv2_control.raw().sq);
	}

	template <typename T>
	s32 try_own(T* cpu, bool wait_only = false)
	{
		const s32 signal = lv2_control.fetch_op([&](control_data_t& data)
		{
			if (!data.signaled)
			{
				cpu->prio.atomic_op([tag = ++g_priority_order_tag](std::common_type_t<decltype(T::prio)>& prio)
				{
					prio.order = tag;
				});

				cpu->next_cpu = data.sq;
				data.sq = cpu;
			}
			else
			{
				ensure(!wait_only);
				data.signaled = 0;
			}
		}).signaled;

		if (signal)
		{
			cpu->next_cpu = nullptr;
		}
		else
		{
			const bool notify = lwcond_waiters.fetch_op([](s32& val)
			{
				if (val + 0u <= 1u << 31)
				{
					// Value was either positive or INT32_MIN
					return false;
				}

				// lwmutex was set to be destroyed, but there are lwcond waiters
				// Turn off the "lwcond_waiters notification" bit as we are adding an lwmutex waiter
				val &= 0x7fff'ffff;
				return true;
			}).second;

			if (notify)
			{
				// Notify lwmutex destroyer (may cause EBUSY to be returned for it)
				lwcond_waiters.notify_all();
			}
		}

		return signal;
	}

	bool try_unlock(bool unlock2)
	{
		if (!load_sq())
		{
			control_data_t old{};
			old.signaled = atomic_storage<s32>::load(lv2_control.raw().signaled);
			control_data_t store = old;
			store.signaled |= (unlock2 ? s32{smin} : 1);

			if (lv2_control.compare_exchange(old, store))
			{
				return true;
			}
		}

		return false;
	}

	template <typename T>
	T* reown(bool unlock2 = false)
	{
		T* res = nullptr;

		lv2_control.fetch_op([&](control_data_t& data)
		{
			res = nullptr;

			if (auto sq = static_cast<T*>(data.sq))
			{
				res = schedule<T>(data.sq, protocol, false);

				if (sq == data.sq)
				{
					return false;
				}

				return true;
			}
			else
			{
				data.signaled |= (unlock2 ? s32{smin} : 1);
				return true;
			}
		});

		if (res && cpu_flag::again - res->state)
		{
			// Detach manually (fetch_op can fail, so avoid side-effects on the first node in this case)
			res->next_cpu = nullptr;
		}

		return res;
	}
};

// Aux
class ppu_thread;

// Syscalls

error_code _sys_lwmutex_create(ppu_thread& ppu, vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, s32 has_name, u64 name);
error_code _sys_lwmutex_destroy(ppu_thread& ppu, u32 lwmutex_id);
error_code _sys_lwmutex_lock(ppu_thread& ppu, u32 lwmutex_id, u64 timeout);
error_code _sys_lwmutex_trylock(ppu_thread& ppu, u32 lwmutex_id);
error_code _sys_lwmutex_unlock(ppu_thread& ppu, u32 lwmutex_id);
error_code _sys_lwmutex_unlock2(ppu_thread& ppu, u32 lwmutex_id);
