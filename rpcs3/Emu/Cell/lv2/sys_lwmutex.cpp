#include "stdafx.h"
#include "sys_lwmutex.h"

#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

LOG_CHANNEL(sys_lwmutex);

error_code _sys_lwmutex_create(ppu_thread& ppu, vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, s32 has_name, u64 name)
{
	ppu.state += cpu_flag::wait;

	sys_lwmutex.warning(u8"_sys_lwmutex_create(lwmutex_id=*0x%x, protocol=0x%x, control=*0x%x, has_name=0x%x, name=0x%llx (“%s”))", lwmutex_id, protocol, control, has_name, name, lv2_obj::name64(std::bit_cast<be_t<u64>>(name)));

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
		*lwmutex_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code _sys_lwmutex_destroy(ppu_thread& ppu, u32 lwmutex_id)
{
	ppu.state += cpu_flag::wait;

	sys_lwmutex.warning("_sys_lwmutex_destroy(lwmutex_id=0x%x)", lwmutex_id);

	std::shared_ptr<lv2_lwmutex> _mutex;

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

			if (!mutex.sq.empty())
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

	const auto mutex = idm::get<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex)
	{
		if (mutex.signaled.try_dec(0))
		{
			return true;
		}

		std::lock_guard lock(mutex.mutex);

		auto [old, _] = mutex.signaled.fetch_op([](s32& value)
		{
			if (value)
			{
				value = 0;
				return true;
			}

			return false;
		});

		if (old)
		{
			if (old == smin)
			{
				ppu.gpr[3] = CELL_EBUSY;
			}

			return true;
		}

		mutex.add_waiter(&ppu);
		mutex.sleep(ppu, timeout);
		return false;
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex.ret)
	{
		return not_an_error(ppu.gpr[3]);
	}

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

				std::lock_guard lock(mutex->mutex);

				if (!mutex->unqueue(mutex->sq, &ppu))
				{
					break;
				}

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

error_code _sys_lwmutex_trylock(ppu_thread& ppu, u32 lwmutex_id)
{
	ppu.state += cpu_flag::wait;

	sys_lwmutex.trace("_sys_lwmutex_trylock(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex)
	{
		auto [_, ok] = mutex.signaled.fetch_op([](s32& value)
		{
			if (value & 1)
			{
				value = 0;
				return true;
			}

			return false;
		});

		return ok;
	});

	if (!mutex)
	{
		return CELL_ESRCH;
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

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex)
	{
		std::lock_guard lock(mutex.mutex);

		if (const auto cpu = mutex.schedule<ppu_thread>(mutex.sq, mutex.protocol))
		{
			mutex.awake(cpu);
			return;
		}

		mutex.signaled |= 1;
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

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex)
	{
		std::lock_guard lock(mutex.mutex);

		if (const auto cpu = mutex.schedule<ppu_thread>(mutex.sq, mutex.protocol))
		{
			static_cast<ppu_thread*>(cpu)->gpr[3] = CELL_EBUSY;
			mutex.awake(cpu);
			return;
		}

		mutex.signaled |= smin;
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}
