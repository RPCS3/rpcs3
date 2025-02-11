#include "stdafx.h"

#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

#include "util/asm.hpp"

#include "sys_mutex.h"

LOG_CHANNEL(sys_mutex);

lv2_mutex::lv2_mutex(utils::serial& ar)
	: protocol(ar)
	, recursive(ar)
	, adaptive(ar)
	, key(ar)
	, name(ar)
{
	ar(lock_count, control.raw().owner);

	// For backwards compatibility
	control.raw().owner >>= 1;
}

std::function<void(void*)> lv2_mutex::load(utils::serial& ar)
{
	return load_func(make_shared<lv2_mutex>(stx::exact_t<utils::serial&>(ar)));
}

void lv2_mutex::save(utils::serial& ar)
{
	ar(protocol, recursive, adaptive, key, name, lock_count, control.raw().owner << 1);
}

error_code sys_mutex_create(ppu_thread& ppu, vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute_t> attr)
{
	ppu.state += cpu_flag::wait;

	sys_mutex.trace("sys_mutex_create(mutex_id=*0x%x, attr=*0x%x)", mutex_id, attr);

	if (!mutex_id || !attr)
	{
		return CELL_EFAULT;
	}

	const auto _attr = *attr;

	const u64 ipc_key = lv2_obj::get_key(_attr);

	if (ipc_key)
	{
		sys_mutex.warning("sys_mutex_create(mutex_id=*0x%x, attr=*0x%x): IPC=0x%016x", mutex_id, attr, ipc_key);
	}

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
		return make_shared<lv2_mutex>(
			_attr.protocol,
			_attr.recursive,
			_attr.adaptive,
			ipc_key,
			_attr.name_u64);
	}))
	{
		return error;
	}

	ppu.check_state();
	*mutex_id = idm::last_id();
	return CELL_OK;
}

error_code sys_mutex_destroy(ppu_thread& ppu, u32 mutex_id)
{
	ppu.state += cpu_flag::wait;

	sys_mutex.trace("sys_mutex_destroy(mutex_id=0x%x)", mutex_id);

	const auto mutex = idm::withdraw<lv2_obj, lv2_mutex>(mutex_id, [](lv2_mutex& mutex) -> CellError
	{
		std::lock_guard lock(mutex.mutex);

		if (atomic_storage<u32>::load(mutex.control.raw().owner))
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

	if (mutex->key)
	{
		sys_mutex.warning("sys_mutex_destroy(mutex_id=0x%x): IPC=0x%016x", mutex_id, mutex->key);
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

	const auto mutex = idm::get<lv2_obj, lv2_mutex>(mutex_id, [&, notify = lv2_obj::notify_all_t()](lv2_mutex& mutex)
	{
		CellError result = mutex.try_lock(ppu);

		if (result == CELL_EBUSY && !atomic_storage<ppu_thread*>::load(mutex.control.raw().sq))
		{
			// Try busy waiting a bit if advantageous
			for (u32 i = 0, end = lv2_obj::has_ppus_in_running_state() ? 3 : 10; id_manager::g_mutex.is_lockable() && i < end; i++)
			{
				busy_wait(300);
				result = mutex.try_lock(ppu);

				if (!result || atomic_storage<ppu_thread*>::load(mutex.control.raw().sq))
				{
					break;
				}
			}
		}

		if (result == CELL_EBUSY)
		{
			lv2_obj::prepare_for_sleep(ppu);

			ppu.cancel_sleep = 1;

			if (mutex.try_own(ppu) || !mutex.sleep(ppu, timeout))
			{
				result = {};
			}

			if (ppu.cancel_sleep != 1)
			{
				notify.cleanup();
			}

			ppu.cancel_sleep = 0;
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

	while (auto state = +ppu.state)
	{
		if (state & cpu_flag::signal && ppu.state.test_and_reset(cpu_flag::signal))
		{
			break;
		}

		if (is_stopped(state))
		{
			std::lock_guard lock(mutex->mutex);

			for (auto cpu = atomic_storage<ppu_thread*>::load(mutex->control.raw().sq); cpu; cpu = cpu->next_cpu)
			{
				if (cpu == &ppu)
				{
					ppu.state += cpu_flag::again;
					return {};
				}
			}

			break;
		}

		for (usz i = 0; cpu_flag::signal - ppu.state && i < 40; i++)
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

				if (!atomic_storage<ppu_thread*>::load(mutex->control.raw().sq))
				{
					// Waiters queue is empty, so the thread must have been signaled
					mutex->mutex.lock_unlock();
					break;
				}

				std::lock_guard lock(mutex->mutex);

				bool success = false;

				mutex->control.fetch_op([&](lv2_mutex::control_data_t& data)
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

error_code sys_mutex_trylock(ppu_thread& ppu, u32 mutex_id)
{
	ppu.state += cpu_flag::wait;

	sys_mutex.trace("sys_mutex_trylock(mutex_id=0x%x)", mutex_id);

	const auto mutex = idm::check<lv2_obj, lv2_mutex>(mutex_id, [&](lv2_mutex& mutex)
	{
		return mutex.try_lock(ppu);
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

	const auto mutex = idm::check<lv2_obj, lv2_mutex>(mutex_id, [&, notify = lv2_obj::notify_all_t()](lv2_mutex& mutex) -> CellError
	{
		auto result = mutex.try_unlock(ppu);

		if (result == CELL_EBUSY)
		{
			std::lock_guard lock(mutex.mutex);

			if (auto cpu = mutex.reown<ppu_thread>())
			{
				if (cpu->state & cpu_flag::again)
				{
					ppu.state += cpu_flag::again;
					return {};
				}

				mutex.awake(cpu);
			}

			result = {};
		}

		notify.cleanup();
		return result;
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
