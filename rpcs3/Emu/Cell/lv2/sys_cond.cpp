﻿#include "stdafx.h"
#include "sys_cond.h"

#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

LOG_CHANNEL(sys_cond);

template<> DECLARE(ipc_manager<lv2_cond, u64>::g_ipc) {};

error_code sys_cond_create(ppu_thread& ppu, vm::ptr<u32> cond_id, u32 mutex_id, vm::ptr<sys_cond_attribute_t> attr)
{
	vm::temporary_unlock(ppu);

	sys_cond.warning("sys_cond_create(cond_id=*0x%x, mutex_id=0x%x, attr=*0x%x)", cond_id, mutex_id, attr);

	auto mutex = idm::get<lv2_obj, lv2_mutex>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	const auto _attr = *attr;

	if (auto error = lv2_obj::create<lv2_cond>(_attr.pshared, _attr.ipc_key, _attr.flags, [&]
	{
		return std::make_shared<lv2_cond>(
			_attr.pshared,
			_attr.flags,
			_attr.ipc_key,
			_attr.name_u64,
			std::move(mutex));
	}))
	{
		return error;
	}

	*cond_id = idm::last_id();
	return CELL_OK;
}

error_code sys_cond_destroy(ppu_thread& ppu, u32 cond_id)
{
	vm::temporary_unlock(ppu);

	sys_cond.warning("sys_cond_destroy(cond_id=0x%x)", cond_id);

	const auto cond = idm::withdraw<lv2_obj, lv2_cond>(cond_id, [&](lv2_cond& cond) -> CellError
	{
		std::lock_guard lock(cond.mutex->mutex);

		if (cond.waiters)
		{
			return CELL_EBUSY;
		}

		cond.mutex->cond_count--;
		return {};
	});

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (cond.ret)
	{
		return cond.ret;
	}

	return CELL_OK;
}

error_code sys_cond_signal(ppu_thread& ppu, u32 cond_id)
{
	vm::temporary_unlock(ppu);

	sys_cond.trace("sys_cond_signal(cond_id=0x%x)", cond_id);

	const auto cond = idm::check<lv2_obj, lv2_cond>(cond_id, [](lv2_cond& cond)
	{
		if (cond.waiters)
		{
			std::lock_guard lock(cond.mutex->mutex);

			if (const auto cpu = cond.schedule<ppu_thread>(cond.sq, cond.mutex->protocol))
			{
				// TODO: Is EBUSY returned after reqeueing, on sys_cond_destroy?
				cond.waiters--;

				if (cond.mutex->try_own(*cpu, cpu->id))
				{
					cond.awake(cpu);
				}
			}
		}
	});

	if (!cond)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_cond_signal_all(ppu_thread& ppu, u32 cond_id)
{
	vm::temporary_unlock(ppu);

	sys_cond.trace("sys_cond_signal_all(cond_id=0x%x)", cond_id);

	const auto cond = idm::check<lv2_obj, lv2_cond>(cond_id, [](lv2_cond& cond)
	{
		if (cond.waiters)
		{
			std::lock_guard lock(cond.mutex->mutex);

			cpu_thread* result = nullptr;
			cond.waiters -= ::size32(cond.sq);

			while (const auto cpu = cond.schedule<ppu_thread>(cond.sq, SYS_SYNC_PRIORITY))
			{
				if (cond.mutex->try_own(*cpu, cpu->id))
				{
					verify(HERE), !std::exchange(result, cpu);
				}
			}

			if (result)
			{
				lv2_obj::awake(result);
			}
		}
	});

	if (!cond)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_cond_signal_to(ppu_thread& ppu, u32 cond_id, u32 thread_id)
{
	vm::temporary_unlock(ppu);

	sys_cond.trace("sys_cond_signal_to(cond_id=0x%x, thread_id=0x%x)", cond_id, thread_id);

	const auto cond = idm::check<lv2_obj, lv2_cond>(cond_id, [&](lv2_cond& cond) -> int
	{
		if (const auto cpu = idm::check_unlocked<named_thread<ppu_thread>>(thread_id);
			!cpu || cpu->joiner == ppu_join_status::exited)
		{
			return -1;
		}

		if (cond.waiters)
		{
			std::lock_guard lock(cond.mutex->mutex);

			for (auto cpu : cond.sq)
			{
				if (cpu->id == thread_id)
				{
					verify(HERE), cond.unqueue(cond.sq, cpu);

					cond.waiters--;

					if (cond.mutex->try_own(*cpu, cpu->id))
					{
						cond.awake(cpu);
					}

					return 1;
				}
			}
		}

		return 0;
	});

	if (!cond || cond.ret == -1)
	{
		return CELL_ESRCH;
	}

	if (!cond.ret)
	{
		return not_an_error(CELL_EPERM);
	}

	return CELL_OK;
}

error_code sys_cond_wait(ppu_thread& ppu, u32 cond_id, u64 timeout)
{
	vm::temporary_unlock(ppu);

	sys_cond.trace("sys_cond_wait(cond_id=0x%x, timeout=%lld)", cond_id, timeout);

	const auto cond = idm::get<lv2_obj, lv2_cond>(cond_id, [&](lv2_cond& cond)
	{
		if (cond.mutex->owner >> 1 == ppu.id)
		{
			// Add a "promise" to add a waiter
			cond.waiters++;
		}

		// Save the recursive value
		return cond.mutex->lock_count.load();
	});

	if (!cond)
	{
		return CELL_ESRCH;
	}

	// Verify ownership
	if (cond->mutex->owner >> 1 != ppu.id)
	{
		return CELL_EPERM;
	}
	else
	{
		// Further function result
		ppu.gpr[3] = CELL_OK;

		std::lock_guard lock(cond->mutex->mutex);

		// Register waiter
		cond->sq.emplace_back(&ppu);

		// Unlock the mutex
		cond->mutex->lock_count = 0;

		if (auto cpu = cond->mutex->reown<ppu_thread>())
		{
			cond->mutex->append(cpu);
		}

		// Sleep current thread and schedule mutex waiter
		cond->sleep(ppu, timeout);
	}

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (ppu.is_stopped())
		{
			return 0;
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

				std::lock_guard lock(cond->mutex->mutex);

				// Try to cancel the waiting
				if (cond->unqueue(cond->sq, &ppu))
				{
					// TODO: Is EBUSY returned after reqeueing, on sys_cond_destroy?
					cond->waiters--;

					ppu.gpr[3] = CELL_ETIMEDOUT;

					// Own or requeue
					if (cond->mutex->try_own(ppu, ppu.id))
					{
						break;
					}
				}
				else if (cond->mutex->owner >> 1 == ppu.id)
				{
					break;
				}

				cond->mutex->sleep(ppu);
				timeout = 0;
				continue;
			}
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	// Verify ownership
	verify(HERE), cond->mutex->owner >> 1 == ppu.id;

	// Restore the recursive value
	cond->mutex->lock_count = cond.ret;

	return not_an_error(ppu.gpr[3]);
}
