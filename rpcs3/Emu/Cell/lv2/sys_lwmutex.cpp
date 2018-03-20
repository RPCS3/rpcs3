#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_lwmutex.h"



logs::channel sys_lwmutex("sys_lwmutex");

extern u64 get_system_time();

error_code _sys_lwmutex_create(vm::ptr<u32> lwmutex_id, u32 protocol, vm::ptr<sys_lwmutex_t> control, u32 arg4, u64 name, u32 arg6)
{
	sys_lwmutex.warning("_sys_lwmutex_create(lwmutex_id=*0x%x, protocol=0x%x, control=*0x%x, arg4=0x%x, name=0x%llx, arg6=0x%x)", lwmutex_id, protocol, control, arg4, name, arg6);

	if (protocol == SYS_SYNC_RETRY)
		sys_lwmutex.todo("_sys_lwmutex_create(): SYS_SYNC_RETRY");

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_RETRY && protocol != SYS_SYNC_PRIORITY)
	{
		sys_lwmutex.error("_sys_lwmutex_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	if (arg4 != 0x80000001 || arg6)
	{
		fmt::throw_exception("Unknown arguments (arg4=0x%x, arg6=0x%x)" HERE, arg4, arg6);
	}

	if (const u32 id = idm::make<lv2_obj, lv2_lwmutex>(protocol, control, name))
	{
		*lwmutex_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code _sys_lwmutex_destroy(u32 lwmutex_id)
{
	sys_lwmutex.warning("_sys_lwmutex_destroy(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::withdraw<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex) -> CellError
	{
		semaphore_lock lock(mutex.mutex);

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
	sys_lwmutex.trace("_sys_lwmutex_lock(lwmutex_id=0x%x, timeout=0x%llx)", lwmutex_id, timeout);

	const auto mutex = idm::get<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex)
	{
		if (u32 value = mutex.signaled)
		{
			if (mutex.signaled.compare_and_swap_test(value, value - 1))
			{
				return true;
			}
		}

		semaphore_lock lock(mutex.mutex);

		if (u32 value = mutex.signaled)
		{
			if (mutex.signaled.compare_and_swap_test(value, value - 1))
			{
				return true;
			}
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
				semaphore_lock lock(mutex->mutex);

				if (!mutex->unqueue(mutex->sq, &ppu))
				{
					timeout = 0;
					continue;
				}

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

error_code _sys_lwmutex_trylock(u32 lwmutex_id)
{
	sys_lwmutex.trace("_sys_lwmutex_trylock(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex)
	{
		if (u32 value = mutex.signaled)
		{
			if (mutex.signaled.compare_and_swap_test(value, value - 1))
			{
				return true;
			}
		}

		return false;
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
	sys_lwmutex.trace("_sys_lwmutex_unlock(lwmutex_id=0x%x)", lwmutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_lwmutex>(lwmutex_id, [&](lv2_lwmutex& mutex) -> cpu_thread*
	{
		semaphore_lock lock(mutex.mutex);

		if (const auto cpu = mutex.schedule<ppu_thread>(mutex.sq, mutex.protocol))
		{
			return cpu;
		}

		mutex.signaled++;
		return nullptr;
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex.ret)
	{
		mutex->awake(*mutex.ret);
	}

	return CELL_OK;
}
