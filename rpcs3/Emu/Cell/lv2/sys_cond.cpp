#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_mutex.h"
#include "sys_cond.h"



logs::channel sys_cond("sys_cond");

template<> DECLARE(ipc_manager<lv2_cond, u64>::g_ipc) {};

extern u64 get_system_time();

error_code sys_cond_create(vm::ptr<u32> cond_id, u32 mutex_id, vm::ptr<sys_cond_attribute_t> attr)
{
	sys_cond.warning("sys_cond_create(cond_id=*0x%x, mutex_id=0x%x, attr=*0x%x)", cond_id, mutex_id, attr);

	auto mutex = idm::get<lv2_obj, lv2_mutex>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (auto error = lv2_obj::create<lv2_cond>(attr->pshared, attr->ipc_key, attr->flags, [&]
	{
		return std::make_shared<lv2_cond>(
			attr->pshared,
			attr->flags,
			attr->ipc_key,
			attr->name_u64,
			std::move(mutex));
	}))
	{
		return error;
	}

	*cond_id = idm::last_id();
	return CELL_OK;
}

error_code sys_cond_destroy(u32 cond_id)
{
	sys_cond.warning("sys_cond_destroy(cond_id=0x%x)", cond_id);

	const auto cond = idm::withdraw<lv2_obj, lv2_cond>(cond_id, [&](lv2_cond& cond) -> CellError
	{
		if (cond.waiters)
		{
			return CELL_EBUSY;
		}

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
	sys_cond.trace("sys_cond_signal(cond_id=0x%x)", cond_id);

	const auto cond = idm::check<lv2_obj, lv2_cond>(cond_id, [](lv2_cond& cond) -> cpu_thread*
	{
		if (cond.waiters)
		{
			semaphore_lock lock(cond.mutex->mutex);

			if (const auto cpu = cond.schedule<ppu_thread>(cond.sq, cond.mutex->protocol))
			{
				cond.waiters--;

				if (cond.mutex->try_own(*cpu, cpu->id))
				{
					return cpu;
				}
			}
		}

		return nullptr;
	});

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (cond.ret)
	{
		cond->awake(*cond.ret);
	}

	return CELL_OK;
}

error_code sys_cond_signal_all(ppu_thread& ppu, u32 cond_id)
{
	sys_cond.trace("sys_cond_signal_all(cond_id=0x%x)", cond_id);

	const auto cond = idm::check<lv2_obj, lv2_cond>(cond_id, [](lv2_cond& cond)
	{
		cpu_thread* result = nullptr;

		if (cond.waiters)
		{
			semaphore_lock lock(cond.mutex->mutex);

			while (const auto cpu = cond.schedule<ppu_thread>(cond.sq, cond.mutex->protocol))
			{
				cond.waiters--;

				if (cond.mutex->try_own(*cpu, cpu->id))
				{
					result = cpu;
				}
			}
		}

		return result;
	});

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (cond.ret)
	{
		cond->awake(*cond.ret);
	}

	return CELL_OK;
}

error_code sys_cond_signal_to(ppu_thread& ppu, u32 cond_id, u32 thread_id)
{
	sys_cond.trace("sys_cond_signal_to(cond_id=0x%x, thread_id=0x%x)", cond_id, thread_id);

	const auto cond = idm::check<lv2_obj, lv2_cond>(cond_id, [&](lv2_cond& cond) -> cpu_thread*
	{
		if (cond.waiters)
		{
			semaphore_lock lock(cond.mutex->mutex);

			for (auto cpu : cond.sq)
			{
				if (cpu->id == thread_id)
				{
					verify(HERE), cond.unqueue(cond.sq, cpu), cond.waiters--;

					if (cond.mutex->try_own(*cpu, cpu->id))
					{
						return cpu;
					}

					return (cpu_thread*)(1);
				}
			}
		}

		return nullptr;
	});

	if (!cond)
	{
		return CELL_ESRCH;
	}

	if (cond.ret && cond.ret != (cpu_thread*)(1))
	{
		cond->awake(*cond.ret);
	}
	else if (!cond.ret)
	{
		return not_an_error(CELL_EPERM);
	}

	return CELL_OK;
}

error_code sys_cond_wait(ppu_thread& ppu, u32 cond_id, u64 timeout)
{
	sys_cond.trace("sys_cond_wait(cond_id=0x%x, timeout=%lld)", cond_id, timeout);

	const auto cond = idm::get<lv2_obj, lv2_cond>(cond_id, [&](lv2_cond& cond)
	{
		// Add a "promise" to add a waiter
		cond.waiters++;

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
		// Awww
		cond->waiters--;
		return CELL_EPERM;
	}
	else
	{
		semaphore_lock lock(cond->mutex->mutex);

		// Register waiter
		cond->sq.emplace_back(&ppu);
		cond->sleep(ppu, timeout);

		// Unlock the mutex
		cond->mutex->lock_count = 0;

		if (auto cpu = cond->mutex->reown<ppu_thread>())
		{
			cond->mutex->awake(*cpu);
		}

		// Further function result
		ppu.gpr[3] = CELL_OK;
	}

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (timeout)
		{
			const u64 passed = get_system_time() - ppu.start_time;

			if (passed >= timeout)
			{
				semaphore_lock lock(cond->mutex->mutex);

				// Try to cancel the waiting
				if (cond->unqueue(cond->sq, &ppu))
				{
					cond->waiters--;

					ppu.gpr[3] = CELL_ETIMEDOUT;

					// Own or requeue
					if (cond->mutex->try_own(ppu, ppu.id))
					{
						break;
					}
				}

				timeout = 0;
				continue;
			}

			thread_ctrl::wait_for(timeout - passed);
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
