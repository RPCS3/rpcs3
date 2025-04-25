#include "stdafx.h"
#include "sys_lwcond.h"

#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_lwmutex.h"

#include "util/asm.hpp"

LOG_CHANNEL(sys_lwcond);

lv2_lwcond::lv2_lwcond(utils::serial& ar)
	: name(ar.pop<be_t<u64>>())
	, lwid(ar)
	, protocol(ar)
	, control(ar.pop<decltype(control)>())
{
}

void lv2_lwcond::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(lv2_sync);
	ar(name, lwid, protocol, control);
}

error_code _sys_lwcond_create(ppu_thread& ppu, vm::ptr<u32> lwcond_id, u32 lwmutex_id, vm::ptr<sys_lwcond_t> control, u64 name)
{
	ppu.state += cpu_flag::wait;

	sys_lwcond.trace(u8"_sys_lwcond_create(lwcond_id=*0x%x, lwmutex_id=0x%x, control=*0x%x, name=0x%llx (“%s”))", lwcond_id, lwmutex_id, control, name, lv2_obj::name_64{std::bit_cast<be_t<u64>>(name)});

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
		ppu.check_state();
		*lwcond_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code _sys_lwcond_destroy(ppu_thread& ppu, u32 lwcond_id)
{
	ppu.state += cpu_flag::wait;

	sys_lwcond.trace("_sys_lwcond_destroy(lwcond_id=0x%x)", lwcond_id);

	shared_ptr<lv2_lwcond> _cond;

	while (true)
	{
		s32 old_val = 0;

		auto [ptr, ret] = idm::withdraw<lv2_obj, lv2_lwcond>(lwcond_id, [&](lv2_lwcond& cond) -> CellError
		{
			// Ignore check on first iteration
			if (_cond && std::addressof(cond) != _cond.get())
			{
				// Other thread has destroyed the lwcond earlier
				return CELL_ESRCH;
			}

			std::lock_guard lock(cond.mutex);

			if (atomic_storage<ppu_thread*>::load(cond.sq))
			{
				return CELL_EBUSY;
			}

			old_val = cond.lwmutex_waiters.or_fetch(smin);

			if (old_val != smin)
			{
				// De-schedule if waiters were found
				lv2_obj::sleep(ppu);

				// Repeat loop: there are lwmutex waiters inside _sys_lwcond_queue_wait
				return CELL_EAGAIN;
			}

			return {};
		});

		if (!ptr)
		{
			return CELL_ESRCH;
		}

		if (ret)
		{
			if (ret != CELL_EAGAIN)
			{
				return ret;
			}
		}
		else
		{
			break;
		}

		_cond = std::move(ptr);

		// Wait for all lwcond waiters to quit
		while (old_val + 0u > 1u << 31)
		{
			thread_ctrl::wait_on(_cond->lwmutex_waiters, old_val);

			if (ppu.is_stopped())
			{
				ppu.state += cpu_flag::again;
				return {};
			}

			old_val = _cond->lwmutex_waiters;
		}

		// Wake up from sleep
		ppu.check_state();
	}

	return CELL_OK;
}

error_code _sys_lwcond_signal(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u64 ppu_thread_id, u32 mode)
{
	ppu.state += cpu_flag::wait;

	sys_lwcond.trace("_sys_lwcond_signal(lwcond_id=0x%x, lwmutex_id=0x%x, ppu_thread_id=0x%llx, mode=%d)", lwcond_id, lwmutex_id, ppu_thread_id, mode);

	// Mode 1: lwmutex was initially owned by the calling thread
	// Mode 2: lwmutex was not owned by the calling thread and waiter hasn't been increased
	// Mode 3: lwmutex was forcefully owned by the calling thread

	if (mode < 1 || mode > 3)
	{
		fmt::throw_exception("Unknown mode (%d)", mode);
	}

	while (true)
	{
		if (ppu.test_stopped())
		{
			ppu.state += cpu_flag::again;
			return {};
		}

		bool finished = true;

		ppu.state += cpu_flag::wait;

		const auto cond = idm::check<lv2_obj, lv2_lwcond>(lwcond_id, [&, notify = lv2_obj::notify_all_t()](lv2_lwcond& cond) -> int
		{
			ppu_thread* cpu = nullptr;

			if (ppu_thread_id != u32{umax})
			{
				cpu = idm::check_unlocked<named_thread<ppu_thread>>(static_cast<u32>(ppu_thread_id));

				if (!cpu)
				{
					return -1;
				}
			}

			lv2_lwmutex* mutex = nullptr;

			if (mode != 2)
			{
				mutex = idm::check_unlocked<lv2_obj, lv2_lwmutex>(lwmutex_id);

				if (!mutex)
				{
					return -1;
				}
			}

			if (atomic_storage<ppu_thread*>::load(cond.sq))
			{
				std::lock_guard lock(cond.mutex);

				if (ppu.state & cpu_flag::suspend)
				{
					// Test if another signal caused the current thread to be suspended, in which case it needs to wait until the thread wakes up (otherwise the signal may cause unexpected results)
					finished = false;
					return 0;
				}

				if (cpu)
				{
					if (static_cast<ppu_thread*>(cpu)->state & cpu_flag::again)
					{
						ppu.state += cpu_flag::again;
						return 0;
					}
				}

				auto result = cpu ? cond.unqueue(cond.sq, cpu) :
					cond.schedule<ppu_thread>(cond.sq, cond.protocol);

				if (result)
				{
					if (static_cast<ppu_thread*>(result)->state & cpu_flag::again)
					{
						ppu.state += cpu_flag::again;
						return 0;
					}

					if (mode == 2)
					{
						static_cast<ppu_thread*>(result)->gpr[3] = CELL_EBUSY;
					}
					else if (mode == 3 && mutex->load_sq()) [[unlikely]]
					{
						std::lock_guard lock(mutex->mutex);

						// Respect ordering of the sleep queue
						mutex->try_own(result, true);
						auto result2 = mutex->reown<ppu_thread>();

						if (result2->state & cpu_flag::again)
						{
							ppu.state += cpu_flag::again;
							return 0;
						}

						if (result2 != result)
						{
							cond.awake(result2);
							result = nullptr;
						}
					}
					else if (mode == 1)
					{
						mutex->try_own(result, true);
						result = nullptr;
					}

					if (result)
					{
						cond.awake(result);
					}

					return 1;
				}
			}
			else
			{
				cond.mutex.lock_unlock();

				if (ppu.state & cpu_flag::suspend)
				{
					finished = false;
					return 0;
				}
			}

			return 0;
		});

		if (!finished)
		{
			continue;
		}

		if (!cond || cond.ret == -1)
		{
			return CELL_ESRCH;
		}

		if (!cond.ret)
		{
			if (ppu_thread_id == u32{umax})
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
}

error_code _sys_lwcond_signal_all(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u32 mode)
{
	ppu.state += cpu_flag::wait;

	sys_lwcond.trace("_sys_lwcond_signal_all(lwcond_id=0x%x, lwmutex_id=0x%x, mode=%d)", lwcond_id, lwmutex_id, mode);

	// Mode 1: lwmutex was initially owned by the calling thread
	// Mode 2: lwmutex was not owned by the calling thread and waiter hasn't been increased

	if (mode < 1 || mode > 2)
	{
		fmt::throw_exception("Unknown mode (%d)", mode);
	}

	while (true)
	{
		if (ppu.test_stopped())
		{
			ppu.state += cpu_flag::again;
			return {};
		}

		bool finished = true;

		ppu.state += cpu_flag::wait;

		const auto cond = idm::check<lv2_obj, lv2_lwcond>(lwcond_id, [&, notify = lv2_obj::notify_all_t()](lv2_lwcond& cond) -> int
		{
			lv2_lwmutex* mutex{};

			if (mode != 2)
			{
				mutex = idm::check_unlocked<lv2_obj, lv2_lwmutex>(lwmutex_id);

				if (!mutex)
				{
					return -1;
				}
			}

			if (atomic_storage<ppu_thread*>::load(cond.sq))
			{
				std::lock_guard lock(cond.mutex);

				if (ppu.state & cpu_flag::suspend)
				{
					// Test if another signal caused the current thread to be suspended, in which case it needs to wait until the thread wakes up (otherwise the signal may cause unexpected results)
					finished = false;
					return 0;
				}

				u32 result = 0;

				for (auto cpu = +cond.sq; cpu; cpu = cpu->next_cpu)
				{
					if (cpu->state & cpu_flag::again)
					{
						ppu.state += cpu_flag::again;
						return 0;
					}
				}

				auto sq = cond.sq;
				atomic_storage<ppu_thread*>::release(cond.sq, nullptr);

				while (const auto cpu = cond.schedule<ppu_thread>(sq, cond.protocol))
				{
					if (mode == 2)
					{
						static_cast<ppu_thread*>(cpu)->gpr[3] = CELL_EBUSY;
					}

					if (mode == 1)
					{
						mutex->try_own(cpu, true);
					}
					else
					{
						lv2_obj::append(cpu);
					}

					result++;
				}

				if (result && mode == 2)
				{
					lv2_obj::awake_all();
				}

				return result;
			}
			else
			{
				cond.mutex.lock_unlock();

				if (ppu.state & cpu_flag::suspend)
				{
					finished = false;
					return 0;
				}
			}

			return 0;
		});

		if (!finished)
		{
			continue;
		}

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
}

error_code _sys_lwcond_queue_wait(ppu_thread& ppu, u32 lwcond_id, u32 lwmutex_id, u64 timeout)
{
	ppu.state += cpu_flag::wait;

	sys_lwcond.trace("_sys_lwcond_queue_wait(lwcond_id=0x%x, lwmutex_id=0x%x, timeout=0x%llx)", lwcond_id, lwmutex_id, timeout);

	ppu.gpr[3] = CELL_OK;

	shared_ptr<lv2_lwmutex> mutex;

	auto& sstate = *ppu.optional_savestate_state;

	const auto cond = idm::get<lv2_obj, lv2_lwcond>(lwcond_id, [&, notify = lv2_obj::notify_all_t()](lv2_lwcond& cond)
	{
		mutex = idm::get_unlocked<lv2_obj, lv2_lwmutex>(lwmutex_id);

		if (!mutex)
		{
			return;
		}

		// Increment lwmutex's lwcond's waiters count
		mutex->lwcond_waiters++;

		lv2_obj::prepare_for_sleep(ppu);

		std::lock_guard lock(cond.mutex);

		cond.lwmutex_waiters++;

		const bool mutex_sleep = sstate.try_read<bool>().second;
		sstate.clear();

		if (mutex_sleep)
		{
			// Special: loading state from the point of waiting on lwmutex sleep queue
			mutex->try_own(&ppu, true);
		}
		else
		{
			// Add a waiter
			lv2_obj::emplace(cond.sq, &ppu);
		}

		if (!ppu.loaded_from_savestate && !mutex->try_unlock(false))
		{
			std::lock_guard lock2(mutex->mutex);

			// Process lwmutex sleep queue
			if (const auto cpu = mutex->reown<ppu_thread>())
			{
				if (static_cast<ppu_thread*>(cpu)->state & cpu_flag::again)
				{
					ensure(cond.unqueue(cond.sq, &ppu));
					ppu.state += cpu_flag::again;
					return;
				}

				// Put the current thread to sleep and schedule lwmutex waiter atomically
				cond.append(cpu);
				cond.sleep(ppu, timeout);
				return;
			}
		}

		cond.sleep(ppu, timeout);
	});

	if (!cond || !mutex)
	{
		return { CELL_ESRCH, fmt::format("lwmutex_id: 0x%x, lwcond_id: 0x%x", lwmutex_id, lwcond_id) };
	}

	if (ppu.state & cpu_flag::again)
	{
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
			std::scoped_lock lock(cond->mutex, mutex->mutex);

			bool mutex_sleep = false;
			bool cond_sleep = false;

			for (auto cpu = mutex->load_sq(); cpu; cpu = cpu->next_cpu)
			{
				if (cpu == &ppu)
				{
					mutex_sleep = true;
					break;
				}
			}

			for (auto cpu = atomic_storage<ppu_thread*>::load(cond->sq); cpu; cpu = cpu->next_cpu)
			{
				if (cpu == &ppu)
				{
					cond_sleep = true;
					break;
				}
			}

			if (!cond_sleep && !mutex_sleep)
			{
				break;
			}

			sstate(mutex_sleep);
			ppu.state += cpu_flag::again;
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

				std::lock_guard lock(cond->mutex);

				if (cond->unqueue(cond->sq, &ppu))
				{
					ppu.gpr[3] = CELL_ETIMEDOUT;
					break;
				}

				std::lock_guard lock2(mutex->mutex);

				bool success = false;

				mutex->lv2_control.fetch_op([&](lv2_lwmutex::control_data_t& data)
				{
					success = false;

					ppu_thread* sq = static_cast<ppu_thread*>(data.sq);

					const bool retval = &ppu == sq;

					if (!mutex->unqueue<false>(sq, &ppu))
					{
						return false;
					}

					success = true;

					if (!retval)
					{
						return false;
					}

					data.sq = sq;
					return true;
				});

				if (success)
				{
					ppu.next_cpu = nullptr;
					ppu.gpr[3] = CELL_ETIMEDOUT;
				}

				break;
			}
		}
		else
		{
			ppu.state.wait(state);
		}
	}

	if (--mutex->lwcond_waiters == smin)
	{
		// Notify the thread destroying lwmutex on last waiter
		mutex->lwcond_waiters.notify_all();
	}

	if (--cond->lwmutex_waiters == smin)
	{
		// Notify the thread destroying lwcond on last waiter
		cond->lwmutex_waiters.notify_all();
	}

	// Return cause
	return not_an_error(ppu.gpr[3]);
}
