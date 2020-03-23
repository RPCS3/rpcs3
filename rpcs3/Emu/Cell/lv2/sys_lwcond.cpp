#include "stdafx.h"
#include "sys_lwcond.h"

#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_lwmutex.h"

LOG_CHANNEL(sys_lwcond);

error_code _sys_lwcond_create(ppu_thread& ppu, vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name)
{
	vm::temporary_unlock(ppu);

	sys_lwcond.warning(u8"_sys_lwcond_create(lwcond_id=*0x%x, lwmutex_id=0x%x, control=*0x%x, name=0x%llx (“%s”))", lwcond_id, lwmutex_id, control, name, lv2_obj::name64(std::bit_cast<be_t<u64>>(name)));

	u32 protocol;

	// Extract protocol from lwmutex
	if (!idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&protocol](lv2_lwmutex& mutex)
	{
		protocol = mutex.protocol;
	}))
	{
		return CELL_ESRCH;
	}

	if (protocol == SYS_SYNC_RETRY)
	{
		// Lwcond can't have SYS_SYNC_RETRY protocol
		protocol = SYS_SYNC_PRIORITY;
	}

	if (const u32 id = idm::make<lv2_obj, lv2_lwcond>(name, lwmutex_id, protocol, control))
	{
		*lwcond_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code _sys_lwcond_destroy(ppu_thread& ppu, u32 lwcond_id)
{
	vm::temporary_unlock(ppu);

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
	vm::temporary_unlock(ppu);

	sys_lwcond.trace("_sys_lwcond_signal(lwcond_id=0x%x, lwmutex_id=0x%x, ppu_thread_id=0x%x, mode=%d)", lwcond_id, lwmutex_id, ppu_thread_id, mode);

	// Mode 1: lwmutex was initially owned by the calling thread
	// Mode 2: lwmutex was not owned by the calling thread and waiter hasn't been increased
	// Mode 3: lwmutex was forcefully owned by the calling thread

	if (mode < 1 || mode > 3)
	{
		fmt::throw_exception("Unknown mode (%d)" HERE, mode);
	}

	const auto cond = idm::check<lv2_obj, lv2_lwcond>(lwcond_id, [&](lv2_lwcond& cond) -> int
	{
		if (ppu_thread_id != umax)
		{
			if (const auto cpu = idm::check_unlocked<named_thread<ppu_thread>>(ppu_thread_id);
				!cpu || cpu->joiner == ppu_join_status::exited)
			{
				return -1;
			}
		}

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

			cpu_thread* result = nullptr;

			if (ppu_thread_id != umax)
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
				result = cond.schedule<ppu_thread>(cond.sq, cond.protocol);
			}

			if (result)
			{
				cond.waiters--;

				if (mode == 2)
				{
					static_cast<ppu_thread*>(result)->gpr[3] = CELL_EBUSY;
				}

				if (mode != 2)
				{
					verify(HERE), !mutex->signaled;
					std::lock_guard lock(mutex->mutex);

					if (mode == 3 && !mutex->sq.empty()) [[unlikely]]
					{
						// Respect ordering of the sleep queue
						mutex->sq.emplace_back(result);
						result = mutex->schedule<ppu_thread>(mutex->sq, mutex->protocol);
					}
					else if (mode == 1)
					{
						verify(HERE), mutex->add_waiter(result);
						result = nullptr;
					}
				}

				if (result)
				{
					cond.awake(result);
				}

				return 1;
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
		if (ppu_thread_id == umax)
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

	return CELL_OK;
}

error_code _sys_lwcond_signal_all(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u32 mode)
{
	vm::temporary_unlock(ppu);

	sys_lwcond.trace("_sys_lwcond_signal_all(lwcond_id=0x%x, lwmutex_id=0x%x, mode=%d)", lwcond_id, lwmutex_id, mode);

	// Mode 1: lwmutex was initially owned by the calling thread
	// Mode 2: lwmutex was not owned by the calling thread and waiter hasn't been increased

	if (mode < 1 || mode > 2)
	{
		fmt::throw_exception("Unknown mode (%d)" HERE, mode);
	}

	bool need_awake = false;

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

			while (const auto cpu = cond.schedule<ppu_thread>(cond.sq, cond.protocol))
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
					verify(HERE), mutex->add_waiter(cpu);
				}
				else
				{
					lv2_obj::append(cpu);
					need_awake = true;
				}

				result++;
			}

			if (need_awake)
			{
				lv2_obj::awake_all();
			}

			return result;
		}

		return 0;
	});

	if (!cond || cond.ret == -1)
	{
		return CELL_ESRCH;
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
	vm::temporary_unlock(ppu);

	sys_lwcond.trace("_sys_lwcond_queue_wait(lwcond_id=0x%x, lwmutex_id=0x%x, timeout=0x%llx)", lwcond_id, lwmutex_id, timeout);

	ppu.gpr[3] = CELL_OK;

	std::shared_ptr<lv2_lwmutex> mutex;

	const auto cond = idm::get<lv2_obj, lv2_lwcond>(lwcond_id, [&](lv2_lwcond& cond)
	{
		mutex = idm::get_unlocked<lv2_obj, lv2_lwmutex>(lwmutex_id);

		if (!mutex)
		{
			return;
		}

		// Try to increment lwmutex's lwcond's waiters count
		if (!mutex->lwcond_waiters.fetch_op([](s32& val)
		{
			if (val == INT32_MIN)
			{
				return false;
			}

			val++;
			return true;
		}).second)
		{
			// Failed - lwmutex was detroyed and all waiters have quit
			mutex.reset();
			return;
		}

		std::lock_guard lock(cond.mutex);

		// Add a waiter
		cond.waiters++;
		cond.sq.emplace_back(&ppu);

		{
			std::lock_guard lock2(mutex->mutex);

			// Process lwmutex sleep queue
			if (const auto cpu = mutex->schedule<ppu_thread>(mutex->sq, mutex->protocol))
			{
				cond.append(cpu);
			}
			else
			{
				mutex->signaled |= 1;
			}
		}

		// Sleep current thread and schedule lwmutex waiter
		cond.sleep(ppu, timeout);
	});

	if (!cond || !mutex)
	{
		return CELL_ESRCH;
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
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	if (--mutex->lwcond_waiters == INT32_MIN)
	{
		// Notify the thread destroying lwmutex on last waiter
		mutex->lwcond_waiters.notify_all();
	}

	// Return cause
	return not_an_error(ppu.gpr[3]);
}
