#include "stdafx.h"
#include "sys_mutex.h"

#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

LOG_CHANNEL(sys_mutex);

template<> DECLARE(ipc_manager<lv2_mutex, u64>::g_ipc) {};

error_code sys_mutex_create(ppu_thread& ppu, vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute_t> attr)
{
	vm::temporary_unlock(ppu);

	sys_mutex.warning("sys_mutex_create(mutex_id=*0x%x, attr=*0x%x)", mutex_id, attr);

	if (!mutex_id || !attr)
	{
		return CELL_EFAULT;
	}

	switch (attr->protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_PRIORITY: break;
	case SYS_SYNC_PRIORITY_INHERIT:
		sys_mutex.warning("sys_mutex_create(): SYS_SYNC_PRIORITY_INHERIT");
		break;
	default:
	{
		sys_mutex.error("sys_mutex_create(): unknown protocol (0x%x)", attr->protocol);
		return CELL_EINVAL;
	}
	}

	switch (attr->recursive)
	{
	case SYS_SYNC_RECURSIVE: break;
	case SYS_SYNC_NOT_RECURSIVE: break;
	default:
	{
		sys_mutex.error("sys_mutex_create(): unknown recursive (0x%x)", attr->recursive);
		return CELL_EINVAL;
	}
	}

	if (attr->adaptive != SYS_SYNC_NOT_ADAPTIVE)
	{
		sys_mutex.todo("sys_mutex_create(): unexpected adaptive (0x%x)", attr->adaptive);
	}

	if (auto error = lv2_obj::create<lv2_mutex>(attr->pshared, attr->ipc_key, attr->flags, [&]()
	{
		return std::make_shared<lv2_mutex>(
			attr->protocol,
			attr->recursive,
			attr->pshared,
			attr->adaptive,
			attr->ipc_key,
			attr->flags,
			attr->name_u64);
	}))
	{
		return error;
	}

	*mutex_id = idm::last_id();
	return CELL_OK;
}

error_code sys_mutex_destroy(ppu_thread& ppu, u32 mutex_id)
{
	vm::temporary_unlock(ppu);

	sys_mutex.warning("sys_mutex_destroy(mutex_id=0x%x)", mutex_id);

	const auto mutex = idm::withdraw<lv2_obj, lv2_mutex>(mutex_id, [](lv2_mutex& mutex) -> CellError
	{
		std::lock_guard lock(mutex.mutex);

		if (mutex.owner || mutex.lock_count)
		{
			return CELL_EBUSY;
		}

		if (mutex.cond_count)
		{
			return CELL_EPERM;
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

error_code sys_mutex_lock(ppu_thread& ppu, u32 mutex_id, u64 timeout)
{
	vm::temporary_unlock(ppu);

	sys_mutex.trace("sys_mutex_lock(mutex_id=0x%x, timeout=0x%llx)", mutex_id, timeout);

	const auto mutex = idm::get<lv2_obj, lv2_mutex>(mutex_id, [&](lv2_mutex& mutex)
	{
		CellError result = mutex.try_lock(ppu.id);

		if (result == CELL_EBUSY)
		{
			std::lock_guard lock(mutex.mutex);

			if (mutex.try_own(ppu, ppu.id))
			{
				result = {};
			}
			else
			{
				mutex.sleep(ppu, timeout);
			}
		}

		return result;
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex.ret)
	{
		if (mutex.ret != CELL_EBUSY)
		{
			return mutex.ret;
		}
	}
	else
	{
		return CELL_OK;
	}

	ppu.gpr[3] = CELL_OK;

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
					timeout = 0;
					continue;
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

error_code sys_mutex_trylock(ppu_thread& ppu, u32 mutex_id)
{
	vm::temporary_unlock(ppu);

	sys_mutex.trace("sys_mutex_trylock(mutex_id=0x%x)", mutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_mutex>(mutex_id, [&](lv2_mutex& mutex)
	{
		return mutex.try_lock(ppu.id);
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex.ret)
	{
		if (mutex.ret == CELL_EBUSY)
		{
			return not_an_error(CELL_EBUSY);
		}

		return mutex.ret;
	}

	return CELL_OK;
}

error_code sys_mutex_unlock(ppu_thread& ppu, u32 mutex_id)
{
	vm::temporary_unlock(ppu);

	sys_mutex.trace("sys_mutex_unlock(mutex_id=0x%x)", mutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_mutex>(mutex_id, [&](lv2_mutex& mutex)
	{
		return mutex.try_unlock(ppu.id);
	});

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex.ret == CELL_EBUSY)
	{
		std::lock_guard lock(mutex->mutex);

		if (auto cpu = mutex->reown<ppu_thread>())
		{
			mutex->awake(cpu);
		}
	}
	else if (mutex.ret)
	{
		return mutex.ret;
	}

	return CELL_OK;
}
