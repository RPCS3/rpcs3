#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_lwmutex.h"
#include "sys_lwcond.h"



LOG_CHANNEL(sys_lwcond);

extern u64 get_system_time();

error_code _sys_lwcond_create(vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name, u32 arg5)
{
	sys_lwcond.warning("_sys_lwcond_create(lwcond_id=*0x%x, lwmutex_id=0x%x, control=*0x%x, name=0x%llx, arg5=0x%x)", lwcond_id, lwmutex_id, control, name, arg5);

	// Temporarily
	if (!idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id))
	{
		return CELL_ESRCH;
	}

	if (const u32 id = idm::make<lv2_obj, lv2_lwcond>(name, lwmutex_id, control))
	{
		*lwcond_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code _sys_lwcond_destroy(u32 lwcond_id)
{
	sys_lwcond.warning("_sys_lwcond_destroy(lwcond_id=0x%x)", lwcond_id);

	const auto cond = idm::withdraw<lv2_obj, lv2_lwcond>(lwcond_id, [&](lv2_lwcond& cond) -> CellError
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

error_code _sys_lwcond_signal(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u32 ppu_thread_id, u32 mode)
{
	sys_lwcond.trace("_sys_lwcond_signal(lwcond_id=0x%x, lwmutex_id=0x%x, ppu_thread_id=0x%x, mode=%d)", lwcond_id, lwmutex_id, ppu_thread_id, mode);

	// Mode 1: lwmutex was initially owned by the calling thread
	// Mode 2: lwmutex was not owned by the calling thread and waiter hasn't been increased
	// Mode 3: lwmutex was forcefully owned by the calling thread

	if (mode < 1 || mode > 3)
	{
		fmt::throw_exception("Unknown mode (%d)" HERE, mode);
	}

	const auto cond = idm::check<lv2_obj, lv2_lwcond>(lwcond_id, [&](lv2_lwcond& cond) -> cpu_thread*
	{
		if (ppu_thread_id != -1 && !idm::check_unlocked<named_thread<ppu_thread>>(ppu_thread_id))
		{
			return (cpu_thread*)(1);
		}

		lv2_lwmutex* mutex;

		if (mode != 2)
		{
			mutex = idm::check_unlocked<lv2_obj, lv2_lwmutex>(lwmutex_id);

			if (!mutex)
			{
				return (cpu_thread*)(1);
			}
		}

		if (cond.waiters)
		{
			std::lock_guard lock(cond.mutex);

			cpu_thread* result = nullptr;

			if (ppu_thread_id != -1)
			{
				for (auto cpu : cond.sq)
				{
					if (cpu->id == ppu_thread_id)
					{
						verify(HERE), cond.unqueue(cond.sq, cpu);
						result = cpu;
						break;
					}
				}
			}
			else
			{
				result = cond.schedule<ppu_thread>(cond.sq, cond.control->lwmutex->attribute & SYS_SYNC_ATTR_PROTOCOL_MASK);
			}

			if (result)
			{
				cond.waiters--;

				if (mode == 2)
				{
					static_cast<ppu_thread*>(result)->gpr[3] = CELL_EBUSY;
				}

				if (mode == 1)
				{
					verify(HERE), !mutex->signaled;
					std::lock_guard lock(mutex->mutex);
					mutex->sq.emplace_back(result);
				}

				return result;
			}
		}

		return nullptr;
	});

	if (!cond || cond.ret == (cpu_thread*)(1))
	{
		return CELL_ESRCH;
	}

	if (!cond.ret)
	{
		if (ppu_thread_id == -1)
		{
			if (mode == 3)
			{
				return not_an_error(CELL_ENOENT);
			}
			else if (mode == 2)
			{
				return CELL_OK;
			}
		}

		return not_an_error(CELL_EPERM);
	}

	if (mode != 1)
	{
		cond->awake(*cond.ret);
	}

	return CELL_OK;
}

error_code _sys_lwcond_signal_all(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u32 mode)
{
	sys_lwcond.trace("_sys_lwcond_signal_all(lwcond_id=0x%x, lwmutex_id=0x%x, mode=%d)", lwcond_id, lwmutex_id, mode);

	// Mode 1: lwmutex was initially owned by the calling thread
	// Mode 2: lwmutex was not owned by the calling thread and waiter hasn't been increased

	if (mode < 1 || mode > 2)
	{
		fmt::throw_exception("Unknown mode (%d)" HERE, mode);
	}

	std::basic_string<cpu_thread*> threads;

	const auto cond = idm::check<lv2_obj, lv2_lwcond>(lwcond_id, [&](lv2_lwcond& cond) -> s32
	{
		lv2_lwmutex* mutex;

		if (mode != 2)
		{
			mutex = idm::check_unlocked<lv2_obj, lv2_lwmutex>(lwmutex_id);

			if (!mutex)
			{
				return -1;
			}
		}

		if (cond.waiters)
		{
			std::lock_guard lock(cond.mutex);

			u32 result = 0;

			while (const auto cpu = cond.schedule<ppu_thread>(cond.sq, cond.control->lwmutex->attribute & SYS_SYNC_ATTR_PROTOCOL_MASK))
			{
				cond.waiters--;

				if (mode == 2)
				{
					static_cast<ppu_thread*>(cpu)->gpr[3] = CELL_EBUSY;
				}

				if (mode == 1)
				{
					verify(HERE), !mutex->signaled;
					std::lock_guard lock(mutex->mutex);
					mutex->sq.emplace_back(cpu);
				}
				else
				{
					threads.push_back(cpu);
				}

				result++;
			}

			return result;
		}

		return 0;
	});

	if (!cond || cond.ret == -1)
	{
		return CELL_ESRCH;
	}

	for (auto cpu : threads)
	{
		cond->awake(*cpu);
	}

	if (mode == 1)
	{
		// Mode 1: return the amount of threads (TODO)
		return not_an_error(cond.ret);
	}

	return CELL_OK;
}

error_code _sys_lwcond_queue_wait(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u64 timeout)
{
	sys_lwcond.trace("_sys_lwcond_queue_wait(lwcond_id=0x%x, lwmutex_id=0x%x, timeout=0x%llx)", lwcond_id, lwmutex_id, timeout);

	ppu.gpr[3] = CELL_OK;

	std::shared_ptr<lv2_lwmutex> mutex;

	const auto cond = idm::get<lv2_obj, lv2_lwcond>(lwcond_id, [&](lv2_lwcond& cond) -> cpu_thread*
	{
		mutex = idm::get_unlocked<lv2_obj, lv2_lwmutex>(lwmutex_id);

		if (!mutex)
		{
			return nullptr;
		}

		std::lock_guard lock(cond.mutex);

		// Add a waiter
		cond.waiters++;
		cond.sq.emplace_back(&ppu);
		cond.sleep(ppu, timeout);

		std::lock_guard lock2(mutex->mutex);

		// Process lwmutex sleep queue
		if (const auto cpu = mutex->schedule<ppu_thread>(mutex->sq, mutex->protocol))
		{
			return cpu;
		}

		mutex->signaled |= 1;
		return nullptr;
	});

	if (!cond || !mutex)
	{
		return CELL_ESRCH;
	}

	if (cond.ret)
	{
		cond->awake(*cond.ret);
	}

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (ppu.is_stopped())
		{
			return 0;
		}

		if (timeout)
		{
			const u64 passed = get_system_time() - ppu.start_time;

			if (passed >= timeout)
			{
				std::lock_guard lock(cond->mutex);

				if (!cond->unqueue(cond->sq, &ppu))
				{
					timeout = 0;
					continue;
				}

				cond->waiters--;

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

	// Return cause
	return not_an_error(ppu.gpr[3]);
}
