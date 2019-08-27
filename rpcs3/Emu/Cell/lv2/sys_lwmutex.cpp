#include "stdafx.h"
#include "sys_lwmutex.h"

#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

LOG_CHANNEL(sys_lwmutex);

error_code _sys_lwmutex_create(ppu_thread& ppu, vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, s32 has_name, u64 name)
{
	vm::temporary_unlock(ppu);

	sys_lwmutex.warning("_sys_lwmutex_create(lwmutex_id=*0x%x, protocol=0x%x, control=*0x%x, has_name=0x%x, name=0x%llx)", lwmutex_id, protocol, control, has_name, name);

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
	vm::temporary_unlock(ppu);

	sys_lwmutex.warning("_sys_lwmutex_destroy(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::withdraw<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex) -> CellError
	{
		std::lock_guard lock(mutex.mutex);

		if (!mutex.sq.empty())
		{
			return CELL_EBUSY;
		}

		return {};
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex.ret)
	{
		return mutex.ret;
	}

	return CELL_OK;
}

error_code _sys_lwmutex_lock(ppu_thread& ppu, u32 lwmutex_id, u64 timeout)
{
	vm::temporary_unlock(ppu);

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
			if (old == (1 << 31))
			{
				ppu.gpr[3] = CELL_EBUSY;
			}

			return true;
		}

		mutex.sq.emplace_back(&ppu);
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
				ppu.check_state();

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
			thread_ctrl::wait();
		}
	}

	return not_an_error(ppu.gpr[3]);
}

error_code _sys_lwmutex_trylock(ppu_thread& ppu, u32 lwmutex_id)
{
	vm::temporary_unlock(ppu);

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
	vm::temporary_unlock(ppu);

	sys_lwmutex.trace("_sys_lwmutex_unlock(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex) -> cpu_thread*
	{
		std::lock_guard lock(mutex.mutex);

		if (const auto cpu = mutex.schedule<ppu_thread>(mutex.sq, mutex.protocol))
		{
			return cpu;
		}

		mutex.signaled |= 1;
		return nullptr;
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex.ret)
	{
		mutex->awake(mutex.ret);
	}

	return CELL_OK;
}

error_code _sys_lwmutex_unlock2(ppu_thread& ppu, u32 lwmutex_id)
{
	vm::temporary_unlock(ppu);

	sys_lwmutex.warning("_sys_lwmutex_unlock2(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex) -> cpu_thread*
	{
		std::lock_guard lock(mutex.mutex);

		if (const auto cpu = mutex.schedule<ppu_thread>(mutex.sq, mutex.protocol))
		{
			static_cast<ppu_thread*>(cpu)->gpr[3] = CELL_EBUSY;
			return cpu;
		}

		mutex.signaled |= 1 << 31;
		return nullptr;
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex.ret)
	{
		mutex->awake(mutex.ret);
	}

	return CELL_OK;
}
