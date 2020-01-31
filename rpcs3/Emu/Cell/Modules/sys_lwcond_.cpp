#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_cond.h"
#include "sysPrxForUser.h"

LOG_CHANNEL(sysPrxForUser);

error_code sys_lwcond_create(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwcond_attribute_t> attr)
{
	sysPrxForUser.trace("sys_lwcond_create(lwcond=*0x%x, lwmutex=*0x%x, attr=*0x%x)", lwcond, lwmutex, attr);

	vm::var<u32> out_id;
	vm::var<sys_cond_attribute_t> attrs;
	attrs->pshared  = SYS_SYNC_NOT_PROCESS_SHARED;
	attrs->name_u64 = attr->name_u64;

	if (auto res = g_cfg.core.hle_lwmutex ? sys_cond_create(ppu, out_id, lwmutex->sleep_queue, attrs) : _sys_lwcond_create(ppu, out_id, lwmutex->sleep_queue, lwcond, attr->name_u64, 0))
	{
		return res;
	}

	lwcond->lwmutex      = lwmutex;
	lwcond->lwcond_queue = *out_id;
	return CELL_OK;
}

error_code sys_lwcond_destroy(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.trace("sys_lwcond_destroy(lwcond=*0x%x)", lwcond);

	if (g_cfg.core.hle_lwmutex)
	{
		return sys_cond_destroy(ppu, lwcond->lwcond_queue);
	}

	if (error_code res = _sys_lwcond_destroy(ppu, lwcond->lwcond_queue))
	{
		return res;
	}

	lwcond->lwcond_queue = lwmutex_dead;
	return CELL_OK;
}

error_code sys_lwcond_signal(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.trace("sys_lwcond_signal(lwcond=*0x%x)", lwcond);

	if (g_cfg.core.hle_lwmutex)
	{
		return sys_cond_signal(ppu, lwcond->lwcond_queue);
	}

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if ((lwmutex->attribute & SYS_SYNC_ATTR_PROTOCOL_MASK) == SYS_SYNC_RETRY)
	{
		return _sys_lwcond_signal(ppu, lwcond->lwcond_queue, 0, -1, 2);
	}

	if (lwmutex->vars.owner.load() == ppu.id)
	{
		// if owns the mutex
		lwmutex->all_info++;

		// call the syscall
		if (error_code res = _sys_lwcond_signal(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, -1, 1))
		{
			if (ppu.test_stopped())
			{
				return 0;
			}

			lwmutex->all_info--;

			if (res != CELL_EPERM)
			{
				return res;
			}
		}

		return CELL_OK;
	}

	if (error_code res = sys_lwmutex_trylock(ppu, lwmutex))
	{
		// if locking failed

		if (res != CELL_EBUSY)
		{
			return CELL_ESRCH;
		}

		// call the syscall
		return _sys_lwcond_signal(ppu, lwcond->lwcond_queue, 0, -1, 2);
	}

	// if locking succeeded
	lwmutex->all_info++;

	// call the syscall
	if (error_code res = _sys_lwcond_signal(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, -1, 3))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}

		lwmutex->all_info--;

		// unlock the lightweight mutex
		sys_lwmutex_unlock(ppu, lwmutex);

		if (res != CELL_ENOENT)
		{
			return res;
		}
	}

	return CELL_OK;
}

error_code sys_lwcond_signal_all(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.trace("sys_lwcond_signal_all(lwcond=*0x%x)", lwcond);

	if (g_cfg.core.hle_lwmutex)
	{
		return sys_cond_signal_all(ppu, lwcond->lwcond_queue);
	}

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if ((lwmutex->attribute & SYS_SYNC_ATTR_PROTOCOL_MASK) == SYS_SYNC_RETRY)
	{
		return _sys_lwcond_signal_all(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, 2);
	}

	if (lwmutex->vars.owner.load() == ppu.id)
	{
		// if owns the mutex, call the syscall
		const error_code res = _sys_lwcond_signal_all(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, 1);

		if (res <= 0)
		{
			// return error or CELL_OK
			return res;
		}

		if (ppu.test_stopped())
		{
			return 0;
		}

		lwmutex->all_info += +res;
		return CELL_OK;
	}

	if (error_code res = sys_lwmutex_trylock(ppu, lwmutex))
	{
		// if locking failed

		if (res != CELL_EBUSY)
		{
			return CELL_ESRCH;
		}

		// call the syscall
		return _sys_lwcond_signal_all(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, 2);
	}

	// if locking succeeded, call the syscall
	error_code res = _sys_lwcond_signal_all(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, 1);

	if (ppu.test_stopped())
	{
		return 0;
	}

	if (res > 0)
	{
		lwmutex->all_info += +res;

		res = CELL_OK;
	}

	// unlock mutex
	sys_lwmutex_unlock(ppu, lwmutex);

	return res;
}

error_code sys_lwcond_signal_to(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond, u32 ppu_thread_id)
{
	sysPrxForUser.trace("sys_lwcond_signal_to(lwcond=*0x%x, ppu_thread_id=0x%x)", lwcond, ppu_thread_id);

	if (g_cfg.core.hle_lwmutex)
	{
		return sys_cond_signal_to(ppu, lwcond->lwcond_queue, ppu_thread_id);
	}

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if ((lwmutex->attribute & SYS_SYNC_ATTR_PROTOCOL_MASK) == SYS_SYNC_RETRY)
	{
		return _sys_lwcond_signal(ppu, lwcond->lwcond_queue, 0, ppu_thread_id, 2);
	}

	if (lwmutex->vars.owner.load() == ppu.id)
	{
		// if owns the mutex
		lwmutex->all_info++;

		// call the syscall
		if (error_code res = _sys_lwcond_signal(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, ppu_thread_id, 1))
		{
			if (ppu.test_stopped())
			{
				return 0;
			}

			lwmutex->all_info--;

			return res;
		}

		return CELL_OK;
	}

	if (error_code res = sys_lwmutex_trylock(ppu, lwmutex))
	{
		// if locking failed

		if (res != CELL_EBUSY)
		{
			return CELL_ESRCH;
		}

		// call the syscall
		return _sys_lwcond_signal(ppu, lwcond->lwcond_queue, 0, ppu_thread_id, 2);
	}

	// if locking succeeded
	lwmutex->all_info++;

	// call the syscall
	if (error_code res = _sys_lwcond_signal(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, ppu_thread_id, 3))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}

		lwmutex->all_info--;

		// unlock the lightweight mutex
		sys_lwmutex_unlock(ppu, lwmutex);

		return res;
	}

	return CELL_OK;
}

error_code sys_lwcond_wait(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond, u64 timeout)
{
	sysPrxForUser.trace("sys_lwcond_wait(lwcond=*0x%x, timeout=0x%llx)", lwcond, timeout);

	if (g_cfg.core.hle_lwmutex)
	{
		return sys_cond_wait(ppu, lwcond->lwcond_queue, timeout);
	}

	const be_t<u32> tid(ppu.id);

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if (lwmutex->vars.owner.load() != tid)
	{
		// if not owner of the mutex
		return CELL_EPERM;
	}

	// save old recursive value
	const be_t<u32> recursive_value = lwmutex->recursive_count;

	// set special value
	lwmutex->vars.owner = lwmutex_reserved;
	lwmutex->recursive_count = 0;

	// call the syscall
	const error_code res = _sys_lwcond_queue_wait(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, timeout);

	if (ppu.test_stopped())
	{
		return 0;
	}

	if (res == CELL_OK || res == CELL_ESRCH)
	{
		if (res == CELL_OK)
		{
			lwmutex->all_info--;
		}

		// restore owner and recursive value
		const auto old = lwmutex->vars.owner.exchange(tid);
		lwmutex->recursive_count = recursive_value;

		if (old == lwmutex_free || old == lwmutex_dead)
		{
			fmt::throw_exception("Locking failed (lwmutex=*0x%x, owner=0x%x)" HERE, lwmutex, old);
		}

		return res;
	}

	if (res == CELL_EBUSY || res == CELL_ETIMEDOUT)
	{
		if (error_code res2 = sys_lwmutex_lock(ppu, lwmutex, 0))
		{
			return res2;
		}

		// if successfully locked, restore recursive value
		lwmutex->recursive_count = recursive_value;

		if (res == CELL_EBUSY)
		{
			return CELL_OK;
		}

		return res;
	}

	if (res == CELL_EDEADLK)
	{
		// restore owner and recursive value
		const auto old = lwmutex->vars.owner.exchange(tid);
		lwmutex->recursive_count = recursive_value;

		if (old == lwmutex_free || old == lwmutex_dead)
		{
			fmt::throw_exception("Locking failed (lwmutex=*0x%x, owner=0x%x)" HERE, lwmutex, old);
		}

		return not_an_error(CELL_ETIMEDOUT);
	}

	fmt::throw_exception("Unexpected syscall result (lwcond=*0x%x, result=0x%x)" HERE, lwcond, +res);
}

void sysPrxForUser_sys_lwcond_init()
{
	REG_FUNC(sysPrxForUser, sys_lwcond_create).flag(g_cfg.core.hle_lwmutex ? MFF_FORCED_HLE : MFF_PERFECT);
	REG_FUNC(sysPrxForUser, sys_lwcond_destroy).flag(g_cfg.core.hle_lwmutex ? MFF_FORCED_HLE : MFF_PERFECT);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal).flag(g_cfg.core.hle_lwmutex ? MFF_FORCED_HLE : MFF_PERFECT);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_all).flag(g_cfg.core.hle_lwmutex ? MFF_FORCED_HLE : MFF_PERFECT);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_to).flag(g_cfg.core.hle_lwmutex ? MFF_FORCED_HLE : MFF_PERFECT);
	REG_FUNC(sysPrxForUser, sys_lwcond_wait).flag(g_cfg.core.hle_lwmutex ? MFF_FORCED_HLE : MFF_PERFECT);
}
