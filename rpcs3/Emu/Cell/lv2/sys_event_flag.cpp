#include "stdafx.h"
#include "sys_event_flag.h"

#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

#include "util/asm.hpp"

LOG_CHANNEL(sys_event_flag);

lv2_event_flag::lv2_event_flag(utils::serial& ar)
	: protocol(ar)
	, key(ar)
	, type(ar)
	, name(ar)
{
	ar(pattern);
}

std::function<void(void*)> lv2_event_flag::load(utils::serial& ar)
{
	return load_func(make_shared<lv2_event_flag>(stx::exact_t<utils::serial&>(ar)));
}

void lv2_event_flag::save(utils::serial& ar)
{
	ar(protocol, key, type, name, pattern);
}

error_code sys_event_flag_create(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<sys_event_flag_attribute_t> attr, u64 init)
{
	ppu.state += cpu_flag::wait;

	sys_event_flag.warning("sys_event_flag_create(id=*0x%x, attr=*0x%x, init=0x%llx)", id, attr, init);

	if (!id || !attr)
	{
		return CELL_EFAULT;
	}

	const auto _attr = *attr;

	const u32 protocol = _attr.protocol;

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_PRIORITY)
	{
		sys_event_flag.error("sys_event_flag_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	const u32 type = _attr.type;

	if (type != SYS_SYNC_WAITER_SINGLE && type != SYS_SYNC_WAITER_MULTIPLE)
	{
		sys_event_flag.error("sys_event_flag_create(): unknown type (0x%x)", type);
		return CELL_EINVAL;
	}

	const u64 ipc_key = lv2_obj::get_key(_attr);

	if (const auto error = lv2_obj::create<lv2_event_flag>(_attr.pshared, ipc_key, _attr.flags, [&]
	{
		return make_shared<lv2_event_flag>(
			_attr.protocol,
			ipc_key,
			_attr.type,
			_attr.name_u64,
			init);
	}))
	{
		return error;
	}

	ppu.check_state();
	*id = idm::last_id();
	return CELL_OK;
}

error_code sys_event_flag_destroy(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_event_flag.warning("sys_event_flag_destroy(id=0x%x)", id);

	const auto flag = idm::withdraw<lv2_obj, lv2_event_flag>(id, [&](lv2_event_flag& flag) -> CellError
	{
		if (flag.sq)
		{
			return CELL_EBUSY;
		}

		lv2_obj::on_id_destroy(flag, flag.key);
		return {};
	});

	if (!flag)
	{
		return CELL_ESRCH;
	}

	if (flag.ret)
	{
		return flag.ret;
	}

	return CELL_OK;
}

error_code sys_event_flag_wait(ppu_thread& ppu, u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result, u64 timeout)
{
	ppu.state += cpu_flag::wait;

	sys_event_flag.trace("sys_event_flag_wait(id=0x%x, bitptn=0x%llx, mode=0x%x, result=*0x%x, timeout=0x%llx)", id, bitptn, mode, result, timeout);

	// Fix function arguments for external access
	ppu.gpr[3] = -1;
	ppu.gpr[4] = bitptn;
	ppu.gpr[5] = mode;
	ppu.gpr[6] = 0;

	// Always set result
	struct store_result
	{
		vm::ptr<u64> ptr;
		u64 val = 0;

		~store_result() noexcept
		{
			if (ptr)
			{
				cpu_thread::get_current()->check_state();
				*ptr = val;
			}
		}
	} store{result};

	if (!lv2_event_flag::check_mode(mode))
	{
		sys_event_flag.error("sys_event_flag_wait(): unknown mode (0x%x)", mode);
		return CELL_EINVAL;
	}

	const auto flag = idm::get<lv2_obj, lv2_event_flag>(id, [&, notify = lv2_obj::notify_all_t()](lv2_event_flag& flag) -> CellError
	{
		if (flag.pattern.fetch_op([&](u64& pat)
		{
			return lv2_event_flag::check_pattern(pat, bitptn, mode, &ppu.gpr[6]);
		}).second)
		{
			// TODO: is it possible to return EPERM in this case?
			return {};
		}

		lv2_obj::prepare_for_sleep(ppu);

		std::lock_guard lock(flag.mutex);

		if (flag.pattern.fetch_op([&](u64& pat)
		{
			return lv2_event_flag::check_pattern(pat, bitptn, mode, &ppu.gpr[6]);
		}).second)
		{
			return {};
		}

		if (flag.type == SYS_SYNC_WAITER_SINGLE && flag.sq)
		{
			return CELL_EPERM;
		}

		flag.sleep(ppu, timeout);
		lv2_obj::emplace(flag.sq, &ppu);
		return CELL_EBUSY;
	});

	if (!flag)
	{
		return CELL_ESRCH;
	}

	if (flag.ret)
	{
		if (flag.ret != CELL_EBUSY)
		{
			return flag.ret;
		}
	}
	else
	{
		store.val = ppu.gpr[6];
		return CELL_OK;
	}

	while (auto state = +ppu.state)
	{
		if (state & cpu_flag::signal && ppu.state.test_and_reset(cpu_flag::signal))
		{
			break;
		}

		if (is_stopped(state))
		{
			std::lock_guard lock(flag->mutex);

			for (auto cpu = +flag->sq; cpu; cpu = cpu->next_cpu)
			{
				if (cpu == &ppu)
				{
					ppu.state += cpu_flag::again;
					return {};
				}
			}

			break;
		}

		for (usz i = 0; cpu_flag::signal - ppu.state && i < 50; i++)
		{
			busy_wait(500);
		}

		if (ppu.state & cpu_flag::signal)
 		{
			continue;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				if (ppu.check_state())
				{
					continue;
				}

				ppu.state += cpu_flag::wait;

				if (!atomic_storage<ppu_thread*>::load(flag->sq))
				{
					// Waiters queue is empty, so the thread must have been signaled
					flag->mutex.lock_unlock();
					break;
				}

				std::lock_guard lock(flag->mutex);

				if (!flag->unqueue(flag->sq, &ppu))
				{
					break;
				}

				ppu.gpr[3] = CELL_ETIMEDOUT;
				ppu.gpr[6] = flag->pattern;
				break;
			}
		}
		else
		{
			ppu.state.wait(state);
		}
	}

	store.val = ppu.gpr[6];
	return not_an_error(ppu.gpr[3]);
}

error_code sys_event_flag_trywait(ppu_thread& ppu, u32 id, u64 bitptn, u32 mode, vm::ptr<u64> result)
{
	ppu.state += cpu_flag::wait;

	sys_event_flag.trace("sys_event_flag_trywait(id=0x%x, bitptn=0x%llx, mode=0x%x, result=*0x%x)", id, bitptn, mode, result);

	// Always set result
	struct store_result
	{
		vm::ptr<u64> ptr;
		u64 val = 0;

		~store_result() noexcept
		{
			if (ptr)
			{
				cpu_thread::get_current()->check_state();
				*ptr = val;
			}
		}
	} store{result};

	if (!lv2_event_flag::check_mode(mode))
	{
		sys_event_flag.error("sys_event_flag_trywait(): unknown mode (0x%x)", mode);
		return CELL_EINVAL;
	}

	u64 pattern{};

	const auto flag = idm::check<lv2_obj, lv2_event_flag>(id, [&](lv2_event_flag& flag)
	{
		return flag.pattern.fetch_op([&](u64& pat)
		{
			return lv2_event_flag::check_pattern(pat, bitptn, mode, &pattern);
		}).second;
	});

	if (!flag)
	{
		return CELL_ESRCH;
	}

	if (!flag.ret)
	{
		return not_an_error(CELL_EBUSY);
	}

	store.val = pattern;
	return CELL_OK;
}

error_code sys_event_flag_set(cpu_thread& cpu, u32 id, u64 bitptn)
{
	cpu.state += cpu_flag::wait;

	// Warning: may be called from SPU thread.
	sys_event_flag.trace("sys_event_flag_set(id=0x%x, bitptn=0x%llx)", id, bitptn);

	const auto flag = idm::get_unlocked<lv2_obj, lv2_event_flag>(id);

	if (!flag)
	{
		return CELL_ESRCH;
	}

	if ((flag->pattern & bitptn) == bitptn)
	{
		return CELL_OK;
	}

	if (lv2_obj::notify_all_t notify; true)
	{
		std::lock_guard lock(flag->mutex);

		for (auto ppu = +flag->sq; ppu; ppu = ppu->next_cpu)
		{
			if (ppu->state & cpu_flag::again)
			{
				cpu.state += cpu_flag::again;

				// Fake error for abort
				return not_an_error(CELL_EAGAIN);
			}
		}

		u32 count = 0;

		// Process all waiters in single atomic op
		for (u64 pattern = flag->pattern, to_write = pattern, dependant_mask = 0;; to_write = pattern, dependant_mask = 0)
		{
			count = 0;
			to_write |= bitptn;
			dependant_mask = 0;

			for (auto ppu = +flag->sq; ppu; ppu = ppu->next_cpu)
			{
				ppu->gpr[7] = 0;
			}

			auto first = +flag->sq;

			auto get_next = [&]() -> ppu_thread*
			{
				s32 prio = smax;
				ppu_thread* it{};

				for (auto ppu = first; ppu; ppu = ppu->next_cpu)
				{
					if (!ppu->gpr[7] && (flag->protocol != SYS_SYNC_PRIORITY || ppu->prio.load().prio <= prio))
					{
						it = ppu;
						prio = ppu->prio.load().prio;
					}
				}

				if (it)
				{
					// Mark it so it won't reappear
					it->gpr[7] = 1;
				}

				return it;
			};

			while (auto it = get_next())
			{
				auto& ppu = *it;

				const u64 pattern = ppu.gpr[4];
				const u64 mode = ppu.gpr[5];

				// If it's OR mode, set bits must have waken up the thread therefore no
				// dependency on old value
				const u64 dependant_mask_or = ((mode & 0xf) == SYS_EVENT_FLAG_WAIT_OR || (bitptn & pattern & to_write) == pattern ? 0 : pattern);

				if (lv2_event_flag::check_pattern(to_write, pattern, mode, &ppu.gpr[6]))
				{
					dependant_mask |= dependant_mask_or;
					ppu.gpr[3] = CELL_OK;
					count++;

					if (!to_write)
					{
						break;
					}
				}
				else
				{
					ppu.gpr[3] = -1;
				}
			}

			dependant_mask &= ~bitptn;

			auto [new_val, ok] = flag->pattern.fetch_op([&](u64& x)
			{
				if ((x ^ pattern) & dependant_mask)
				{
					return false;
				}

				x |= bitptn;

				// Clear the bit-wise difference
				x &= ~((pattern | bitptn) & ~to_write);
				return true;
			});

			if (ok)
			{
				break;
			}

			pattern = new_val;
		}

		if (!count)
		{
			return CELL_OK;
		}

		// Remove waiters
		for (auto next_cpu = &flag->sq; *next_cpu;)
		{
			auto& ppu = **next_cpu;

			if (ppu.gpr[3] == CELL_OK)
			{
				atomic_storage<ppu_thread*>::release(*next_cpu, ppu.next_cpu);
				ppu.next_cpu = nullptr;
				flag->append(&ppu);
				continue;
			}

			next_cpu = &ppu.next_cpu;
		};

		lv2_obj::awake_all();
	}

	return CELL_OK;
}

error_code sys_event_flag_clear(ppu_thread& ppu, u32 id, u64 bitptn)
{
	ppu.state += cpu_flag::wait;

	sys_event_flag.trace("sys_event_flag_clear(id=0x%x, bitptn=0x%llx)", id, bitptn);

	const auto flag = idm::check<lv2_obj, lv2_event_flag>(id, [&](lv2_event_flag& flag)
	{
		flag.pattern &= bitptn;
	});

	if (!flag)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_event_flag_cancel(ppu_thread& ppu, u32 id, vm::ptr<u32> num)
{
	ppu.state += cpu_flag::wait;

	sys_event_flag.trace("sys_event_flag_cancel(id=0x%x, num=*0x%x)", id, num);

	if (num) *num = 0;

	const auto flag = idm::get_unlocked<lv2_obj, lv2_event_flag>(id);

	if (!flag)
	{
		return CELL_ESRCH;
	}

	u32 value = 0;
	{
		lv2_obj::notify_all_t notify;

		std::lock_guard lock(flag->mutex);

		for (auto cpu = +flag->sq; cpu; cpu = cpu->next_cpu)
		{
			if (cpu->state & cpu_flag::again)
			{
				ppu.state += cpu_flag::again;
				return {};
			}
		}

		// Get current pattern
		const u64 pattern = flag->pattern;

		// Signal all threads to return CELL_ECANCELED (protocol does not matter)
		while (auto ppu = flag->schedule<ppu_thread>(flag->sq, SYS_SYNC_FIFO))
		{
			ppu->gpr[3] = CELL_ECANCELED;
			ppu->gpr[6] = pattern;

			value++;
			flag->append(ppu);
		}

		if (value)
		{
			lv2_obj::awake_all();
		}
	}

	static_cast<void>(ppu.test_stopped());

	if (num) *num = value;
	return CELL_OK;
}

error_code sys_event_flag_get(ppu_thread& ppu, u32 id, vm::ptr<u64> flags)
{
	ppu.state += cpu_flag::wait;

	sys_event_flag.trace("sys_event_flag_get(id=0x%x, flags=*0x%x)", id, flags);

	const auto flag = idm::check<lv2_obj, lv2_event_flag>(id, [](lv2_event_flag& flag)
	{
		return +flag.pattern;
	});

	ppu.check_state();

	if (!flag)
	{
		if (flags) *flags = 0;
		return CELL_ESRCH;
	}

	if (!flags)
	{
		return CELL_EFAULT;
	}

	*flags = flag.ret;
	return CELL_OK;
}
