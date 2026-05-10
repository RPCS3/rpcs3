#include "stdafx.h"
#include "sys_lwmutex.h"

#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

#include "util/asm.hpp"

LOG_CHANNEL(sys_lwmutex);

lv2_lwmutex::lv2_lwmutex(utils::serial& ar)
	: protocol(ar)
	, control(ar.pop<decltype(control)>())
	, name(ar.pop<be_t<u64>>())
{
	ar(lv2_control.raw().signaled);
}

void lv2_lwmutex::save(utils::serial& ar)
{
	ar(protocol, control, name, lv2_control.raw().signaled);
}

error_code _sys_lwmutex_create(ppu_thread& ppu, vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, s32 has_name, u64 name)
{
	ppu.state += cpu_flag::wait;

	sys_lwmutex.trace(u8"_sys_lwmutex_create(lwmutex_id=*0x%x, protocol=0x%x, control=*0x%x, has_name=0x%x, name=0x%llx (“%s”))", lwmutex_id, protocol, control, has_name, name, lv2_obj::name_64{std::bit_cast<be_t<u64>>(name)});

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_RETRY && protocol != SYS_SYNC_PRIORITY)
	{
		sys_lwmutex.error("_sys_lwmutex_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	if (!(has_name < 0))
	{
		name = 0;
	}

	if (const u32 id = idm::make<lv2_obj, lv2_lwmutex>(protocol, control, name))
	{
		ppu.check_state();
		*lwmutex_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code _sys_lwmutex_destroy(ppu_thread& ppu, u32 lwmutex_id)
{
	ppu.state += cpu_flag::wait;

	sys_lwmutex.trace("_sys_lwmutex_destroy(lwmutex_id=0x%x)", lwmutex_id);

	shared_ptr<lv2_lwmutex> _mutex;

	while (true)
	{
		s32 old_val = 0;

		auto [ptr, ret] = idm::withdraw<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex) -> CellError
		{
			// Ignore check on first iteration
			if (_mutex && std::addressof(mutex) != _mutex.get())
			{
				// Other thread has destroyed the lwmutex earlier
				return CELL_ESRCH;
			}

			std::lock_guard lock(mutex.mutex);

			if (mutex.load_sq())
			{
				return CELL_EBUSY;
			}

			old_val = mutex.lwcond_waiters.or_fetch(smin);

			if (old_val != smin)
			{
				// Deschedule if waiters were found
				lv2_obj::sleep(ppu);

				// Repeat loop: there are lwcond waiters
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

		_mutex = std::move(ptr);

		// Wait for all lwcond waiters to quit
		while (old_val + 0u > 1u << 31)
		{
			thread_ctrl::wait_on(_mutex->lwcond_waiters, old_val);

			if (ppu.is_stopped())
			{
				ppu.state += cpu_flag::again;
				return {};
			}

			old_val = _mutex->lwcond_waiters;
		}

		// Wake up from sleep
		ppu.check_state();
	}

	return CELL_OK;
}

error_code _sys_lwmutex_lock(ppu_thread& ppu, u32 lwmutex_id, u64 timeout)
{
	ppu.state += cpu_flag::wait;

	sys_lwmutex.trace("_sys_lwmutex_lock(lwmutex_id=0x%x, timeout=0x%llx)", lwmutex_id, timeout);

	ppu.gpr[3] = CELL_OK;

	const auto mutex = idm::get<lv2_obj, lv2_lwmutex>(lwmutex_id, [&, notify = lv2_obj::notify_all_t()](lv2_lwmutex& mutex)
	{
		if (s32 signal = mutex.lv2_control.fetch_op([](lv2_lwmutex::control_data_t& data)
		{
			if (data.signaled)
			{
				data.signaled = 0;
				return true;
			}

			return false;
		}).first.signaled)
		{
			if (~signal & 1)
			{
				ppu.gpr[3] = CELL_EBUSY;
			}

			return true;
		}

		lv2_obj::prepare_for_sleep(ppu);

		ppu.cancel_sleep = 1;

		if (s32 signal = mutex.try_own(&ppu))
		{
			if (~signal & 1)
			{
				ppu.gpr[3] = CELL_EBUSY;
			}

			ppu.cancel_sleep = 0;
			return true;
		}

		const bool finished = !mutex.sleep(ppu, timeout);
		notify.cleanup();
		return finished;
	});

	if (!mutex)
	{
		if (lwmutex_id >> 24 == lv2_lwmutex::id_base >> 24)
		{
			return { CELL_ESRCH, lwmutex_id };
		}

		return { CELL_ESRCH, "Invalid ID" };
	}

	if (mutex.ret)
	{
		return not_an_error(ppu.gpr[3]);
	}

	while (auto state = +ppu.state)
	{
		if (state & cpu_flag::signal && ppu.state.test_and_reset(cpu_flag::signal))
		{
			break;
		}

		if (is_stopped(state))
		{
			std::lock_guard lock(mutex->mutex);

			for (auto cpu = mutex->load_sq(); cpu; cpu = cpu->next_cpu)
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

				if (!mutex->load_sq())
				{
					// Sleep queue is empty, so the thread must have been signaled
					mutex->mutex.lock_unlock();
					break;
				}

				std::lock_guard lock(mutex->mutex);

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

	return not_an_error(ppu.gpr[3]);
}

error_code _sys_lwmutex_trylock(ppu_thread& ppu, u32 lwmutex_id)
{
	ppu.state += cpu_flag::wait;

	sys_lwmutex.trace("_sys_lwmutex_trylock(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex)
	{
		auto [_, ok] = mutex.lv2_control.fetch_op([](lv2_lwmutex::control_data_t& data)
		{
			if (data.signaled & 1)
			{
				data.signaled = 0;
				return true;
			}

			return false;
		});

		return ok;
	});

	if (!mutex)
	{
		if (lwmutex_id >> 24 == lv2_lwmutex::id_base >> 24)
		{
			return { CELL_ESRCH, lwmutex_id };
		}

		return { CELL_ESRCH, "Invalid ID" };
	}

	if (!mutex.ret)
	{
		return not_an_error(CELL_EBUSY);
	}

	return CELL_OK;
}

error_code _sys_lwmutex_unlock(ppu_thread& ppu, u32 lwmutex_id)
{
	ppu.state += cpu_flag::wait;

	sys_lwmutex.trace("_sys_lwmutex_unlock(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&, notify = lv2_obj::notify_all_t()](lv2_lwmutex& mutex)
	{
		if (mutex.try_unlock(false))
		{
			return;
		}

		std::lock_guard lock(mutex.mutex);

		if (const auto cpu = mutex.reown<ppu_thread>())
		{
			if (static_cast<ppu_thread*>(cpu)->state & cpu_flag::again)
			{
				ppu.state += cpu_flag::again;
				return;
			}

			mutex.awake(cpu);
			notify.cleanup(); // lv2_lwmutex::mutex is not really active 99% of the time, can be ignored
		}
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code _sys_lwmutex_unlock2(ppu_thread& ppu, u32 lwmutex_id)
{
	ppu.state += cpu_flag::wait;

	sys_lwmutex.warning("_sys_lwmutex_unlock2(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&, notify = lv2_obj::notify_all_t()](lv2_lwmutex& mutex)
	{
		if (mutex.try_unlock(true))
		{
			return;
		}

		std::lock_guard lock(mutex.mutex);

		if (const auto cpu = mutex.reown<ppu_thread>(true))
		{
			if (static_cast<ppu_thread*>(cpu)->state & cpu_flag::again)
			{
				ppu.state += cpu_flag::again;
				return;
			}

			static_cast<ppu_thread*>(cpu)->gpr[3] = CELL_EBUSY;
			mutex.awake(cpu);
			notify.cleanup(); // lv2_lwmutex::mutex is not really active 99% of the time, can be ignored
		}
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}
