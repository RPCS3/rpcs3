#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_semaphore.h"



logs::channel sys_semaphore("sys_semaphore");

template<> DECLARE(ipc_manager<lv2_sema, u64>::g_ipc) {};

extern u64 get_system_time();

error_code sys_semaphore_create(vm::ptr<u32> sem_id, vm::ptr<sys_semaphore_attribute_t> attr, s32 initial_val, s32 max_val)
{
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

	const u32 protocol = attr->protocol;

	if (protocol == SYS_SYNC_PRIORITY_INHERIT)
		sys_semaphore.todo("sys_semaphore_create(): SYS_SYNC_PRIORITY_INHERIT");

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_PRIORITY && protocol != SYS_SYNC_PRIORITY_INHERIT)
	{
		sys_semaphore.error("sys_semaphore_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	if (auto error = lv2_obj::create<lv2_sema>(attr->pshared, attr->ipc_key, attr->flags, [&]
	{
		return std::make_shared<lv2_sema>(protocol, attr->pshared, attr->ipc_key, attr->flags, attr->name_u64, max_val, initial_val);
	}))
	{
		return error;
	}

	*sem_id = idm::last_id();
	return CELL_OK;
}

error_code sys_semaphore_destroy(u32 sem_id)
{
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

		semaphore_lock lock(sema.mutex);

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

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (timeout)
		{
			const u64 passed = get_system_time() - ppu.start_time;

			if (passed >= timeout)
			{
				semaphore_lock lock(sem->mutex);

				if (!sem->unqueue(sem->sq, &ppu))
				{
					timeout = 0;
					continue;
				}

				verify(HERE), 0 > sem->val.fetch_op([](s32& val)
				{
					if (val < 0)
					{
						val++;
					}
				});

				ppu.gpr[3] = CELL_ETIMEDOUT;
				break;
			}

			thread_ctrl::wait_for(timeout - passed);
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	return not_an_error(ppu.gpr[3]);
}

error_code sys_semaphore_trywait(u32 sem_id)
{
	sys_semaphore.trace("sys_semaphore_trywait(sem_id=0x%x)", sem_id);

	const auto sem = idm::check<lv2_obj, lv2_sema>(sem_id, [&](lv2_sema& sema)
	{
		const s32 val = sema.val;

		if (val > 0)
		{
			if (sema.val.compare_and_swap_test(val, val - 1))
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

	if (!sem.ret)
	{
		return not_an_error(CELL_EBUSY);
	}

	return CELL_OK;
}

error_code sys_semaphore_post(ppu_thread& ppu, u32 sem_id, s32 count)
{
	sys_semaphore.trace("sys_semaphore_post(sem_id=0x%x, count=%d)", sem_id, count);

	if (count < 0)
	{
		return CELL_EINVAL;
	}

	const auto sem = idm::get<lv2_obj, lv2_sema>(sem_id, [&](lv2_sema& sema)
	{
		const s32 val = sema.val;

		if (val >= 0 && count <= sema.max - val)
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

	if (sem.ret)
	{
		return CELL_OK;
	}
	else
	{
		semaphore_lock lock(sem->mutex);

		const s32 val = sem->val.fetch_op([=](s32& val)
		{
			if (val + s64{count} <= sem->max)
			{
				val += count;
			}
		});

		if (val + s64{count} > sem->max)
		{
			return not_an_error(CELL_EBUSY);
		}

		// Wake threads
		for (s32 i = std::min<s32>(-std::min<s32>(val, 0), count); i > 0; i--)
		{
			sem->awake(*verify(HERE, sem->schedule<ppu_thread>(sem->sq, sem->protocol)));
		}
	}

	return CELL_OK;
}

error_code sys_semaphore_get_value(u32 sem_id, vm::ptr<s32> count)
{
	sys_semaphore.trace("sys_semaphore_get_value(sem_id=0x%x, count=*0x%x)", sem_id, count);

	if (!count)
	{
		return CELL_EFAULT;
	}

	if (!idm::check<lv2_obj, lv2_sema>(sem_id, [=](lv2_sema& sema)
	{
		*count = std::max<s32>(0, sema.val);
	}))
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}
