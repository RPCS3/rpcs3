#include "stdafx.h"
#include "sys_mutex.h"

#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

LOG_CHANNEL(sys_mutex);

lv2_mutex::lv2_mutex(utils::serial& ar)
	: protocol(ar)
	, recursive(ar)
	, adaptive(ar)
	, key(ar)
	, name(ar)
{
	ar(lock_count, owner);
}

std::shared_ptr<void> lv2_mutex::load(utils::serial& ar)
{
	auto mtx = std::make_shared<lv2_mutex>(ar);
	return lv2_obj::load(mtx->key, mtx);
}

void lv2_mutex::save(utils::serial& ar)
{
	ar(protocol, recursive, adaptive, key, name, lock_count, owner & -2);
}

error_code sys_mutex_create(ppu_thread& ppu, vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute_t> attr)
{
	ppu.state += cpu_flag::wait;

	sys_mutex.warning("sys_mutex_create(mutex_id=*0x%x, attr=*0x%x)", mutex_id, attr);

	if (!mutex_id || !attr)
	{
		return CELL_EFAULT;
	}

	const auto _attr = *attr;

	switch (_attr.protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_PRIORITY: break;
	case SYS_SYNC_PRIORITY_INHERIT:
		sys_mutex.warning("sys_mutex_create(): SYS_SYNC_PRIORITY_INHERIT");
		break;
	default:
	{
		sys_mutex.error("sys_mutex_create(): unknown protocol (0x%x)", _attr.protocol);
		return CELL_EINVAL;
	}
	}

	switch (_attr.recursive)
	{
	case SYS_SYNC_RECURSIVE: break;
	case SYS_SYNC_NOT_RECURSIVE: break;
	default:
	{
		sys_mutex.error("sys_mutex_create(): unknown recursive (0x%x)", _attr.recursive);
		return CELL_EINVAL;
	}
	}

	if (_attr.adaptive != SYS_SYNC_NOT_ADAPTIVE)
	{
		sys_mutex.todo("sys_mutex_create(): unexpected adaptive (0x%x)", _attr.adaptive);
	}

	if (auto error = lv2_obj::create<lv2_mutex>(_attr.pshared, _attr.ipc_key, _attr.flags, [&]()
	{
		return std::make_shared<lv2_mutex>(
			_attr.protocol,
			_attr.recursive,
			_attr.adaptive,
			_attr.ipc_key,
			_attr.name_u64);
	}))
	{
		return error;
	}

	*mutex_id = idm::last_id();
	return CELL_OK;
}

error_code sys_mutex_destroy(ppu_thread& ppu, u32 mutex_id)
{
	ppu.state += cpu_flag::wait;

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

		lv2_obj::on_id_destroy(mutex, mutex.key);
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
	ppu.state += cpu_flag::wait;

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

	while (auto state = ppu.state.fetch_sub(cpu_flag::signal))
	{
		if (state & cpu_flag::signal)
		{
			break;
		}

		if (is_stopped(state))
		{
			std::lock_guard lock(mutex->mutex);

			if (std::find(mutex->sq.begin(), mutex->sq.end(), &ppu) == mutex->sq.end())
			{
				break;
			}

			ppu.state += cpu_flag::incomplete_syscall;
			return {};
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

error_code sys_mutex_trylock(ppu_thread& ppu, u32 mutex_id)
{
	ppu.state += cpu_flag::wait;

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
	ppu.state += cpu_flag::wait;

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
