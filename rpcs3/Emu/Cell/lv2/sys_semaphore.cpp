#include "stdafx.h"
#include "sys_semaphore.h"

#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

LOG_CHANNEL(sys_semaphore);

template<> DECLARE(ipc_manager<lv2_sema, u64>::g_ipc) {};

error_code sys_semaphore_create(ppu_thread& ppu, vm::ptr<u32> sem_id, vm::ptr<sys_semaphore_attribute_t> attr, s32 initial_val, s32 max_val)
{
	ppu.state += cpu_flag::wait;

	sys_semaphore.warning("sys_semaphore_create(sem_id=*0x%x, attr=*0x%x, initial_val=%d, max_val=%d)", sem_id, attr, initial_val, max_val);

	if (!sem_id || !attr)
	{
		return CELL_EFAULT;
	}

	if (max_val <= 0 || initial_val > max_val || initial_val < 0)
	{
		sys_semaphore.error("sys_semaphore_create(): invalid parameters (initial_val=%d, max_val=%d)", initial_val, max_val);
		return CELL_EINVAL;
	}

	const auto _attr = *attr;

	const u32 protocol = _attr.protocol;

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_PRIORITY)
	{
		sys_semaphore.error("sys_semaphore_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	if (auto error = lv2_obj::create<lv2_sema>(_attr.pshared, _attr.ipc_key, _attr.flags, [&]
	{
		return std::make_shared<lv2_sema>(protocol, _attr.pshared, _attr.ipc_key, _attr.flags, _attr.name_u64, max_val, initial_val);
	}))
	{
		return error;
	}

	if (ppu.test_stopped())
	{
		return {};
	}

	*sem_id = idm::last_id();
	return CELL_OK;
}

error_code sys_semaphore_destroy(ppu_thread& ppu, u32 sem_id)
{
	ppu.state += cpu_flag::wait;

	sys_semaphore.warning("sys_semaphore_destroy(sem_id=0x%x)", sem_id);

	const auto sem = idm::withdraw<lv2_obj, lv2_sema>(sem_id, [](lv2_sema& sema) -> CellError
	{
		if (sema.val < 0)
		{
			return CELL_EBUSY;
		}

		return {};
	});

	if (!sem)
	{
		return CELL_ESRCH;
	}

	if (sem.ret)
	{
		return sem.ret;
	}

	return CELL_OK;
}

error_code sys_semaphore_wait(ppu_thread& ppu, u32 sem_id, u64 timeout)
{
	ppu.state += cpu_flag::wait;

	sys_semaphore.trace("sys_semaphore_wait(sem_id=0x%x, timeout=0x%llx)", sem_id, timeout);

	const auto sem = idm::get<lv2_obj, lv2_sema>(sem_id, [&](lv2_sema& sema)
	{
		const s32 val = sema.val;

		if (val > 0)
		{
			if (sema.val.compare_and_swap_test(val, val - 1))
			{
				return true;
			}
		}

		std::lock_guard lock(sema.mutex);

		if (sema.val-- <= 0)
		{
			sema.sq.emplace_back(&ppu);
			sema.sleep(ppu, timeout);
			return false;
		}

		return true;
	});

	if (!sem)
	{
		return CELL_ESRCH;
	}

	if (sem.ret)
	{
		return CELL_OK;
	}

	ppu.gpr[3] = CELL_OK;

	while (auto state = ppu.state.fetch_sub(cpu_flag::signal))
	{
		if (is_stopped(state))
		{
			return {};
		}

		if (state & cpu_flag::signal)
		{
			break;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				if (ppu.check_state())
				{
					return {};
				}

				std::lock_guard lock(sem->mutex);

				if (!sem->unqueue(sem->sq, &ppu))
				{
					break;
				}

				ensure(0 > sem->val.fetch_op([](s32& val)
				{
					if (val < 0)
					{
						val++;
					}
				}));

				ppu.gpr[3] = CELL_ETIMEDOUT;
				break;
			}
		}
		else
		{
			thread_ctrl::wait_on(ppu.state, state);
		}
	}

	return not_an_error(ppu.gpr[3]);
}

error_code sys_semaphore_trywait(ppu_thread& ppu, u32 sem_id)
{
	ppu.state += cpu_flag::wait;

	sys_semaphore.trace("sys_semaphore_trywait(sem_id=0x%x)", sem_id);

	const auto sem = idm::check<lv2_obj, lv2_sema>(sem_id, [&](lv2_sema& sema)
	{
		return sema.val.try_dec(0);
	});

	if (!sem)
	{
		return CELL_ESRCH;
	}

	if (!sem.ret)
	{
		return not_an_error(CELL_EBUSY);
	}

	return CELL_OK;
}

error_code sys_semaphore_post(ppu_thread& ppu, u32 sem_id, s32 count)
{
	ppu.state += cpu_flag::wait;

	sys_semaphore.trace("sys_semaphore_post(sem_id=0x%x, count=%d)", sem_id, count);

	const auto sem = idm::get<lv2_obj, lv2_sema>(sem_id, [&](lv2_sema& sema)
	{
		const s32 val = sema.val;

		if (val >= 0 && count > 0 && count <= sema.max - val)
		{
			if (sema.val.compare_and_swap_test(val, val + count))
			{
				return true;
			}
		}

		return false;
	});

	if (!sem)
	{
		return CELL_ESRCH;
	}

	if (count <= 0)
	{
		return CELL_EINVAL;
	}

	if (sem.ret)
	{
		return CELL_OK;
	}
	else
	{
		std::lock_guard lock(sem->mutex);

		const auto [val, ok] = sem->val.fetch_op([&](s32& val)
		{
			if (count + 0u <= sem->max + 0u - val)
			{
				val += count;
				return true;
			}

			return false;
		});

		if (!ok)
		{
			return not_an_error(CELL_EBUSY);
		}

		// Wake threads
		const s32 to_awake = std::min<s32>(-std::min<s32>(val, 0), count);

		for (s32 i = 0; i < to_awake; i++)
		{
			sem->append((ensure(sem->schedule<ppu_thread>(sem->sq, sem->protocol))));
		}

		if (to_awake > 0)
		{
			lv2_obj::awake_all();
		}
	}

	return CELL_OK;
}

error_code sys_semaphore_get_value(ppu_thread& ppu, u32 sem_id, vm::ptr<s32> count)
{
	ppu.state += cpu_flag::wait;

	sys_semaphore.trace("sys_semaphore_get_value(sem_id=0x%x, count=*0x%x)", sem_id, count);

	const auto sema = idm::check<lv2_obj, lv2_sema>(sem_id, [](lv2_sema& sema)
	{
		return std::max<s32>(0, sema.val);
	});

	if (!sema)
	{
		return CELL_ESRCH;
	}

	if (!count)
	{
		return CELL_EFAULT;
	}

	if (ppu.test_stopped())
	{
		return {};
	}

	*count = sema.ret;
	return CELL_OK;
}
