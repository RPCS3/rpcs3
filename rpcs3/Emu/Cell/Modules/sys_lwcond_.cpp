#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "sysPrxForUser.h"

extern logs::channel sysPrxForUser;

s32 sys_lwcond_create(vm::ptr<sys_lwcond_t> lwcond, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwcond_attribute_t> attr)
{
	sysPrxForUser.trace("sys_lwcond_create(lwcond=*0x%x, lwmutex=*0x%x, attr=*0x%x)", lwcond, lwmutex, attr);

	vm::var<u32> out_id;

	if (s32 res = _sys_lwcond_create(out_id, lwmutex->sleep_queue, lwcond, attr->name_u64, 0))
	{
		return res;
	}

	lwcond->lwmutex      = lwmutex;
	lwcond->lwcond_queue = *out_id;
	return CELL_OK;
}

s32 sys_lwcond_destroy(vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.trace("sys_lwcond_destroy(lwcond=*0x%x)", lwcond);

	if (s32 res = _sys_lwcond_destroy(lwcond->lwcond_queue))
	{
		return res;
	}

	lwcond->lwcond_queue = lwmutex_dead;
	return CELL_OK;
}

s32 sys_lwcond_signal(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.trace("sys_lwcond_signal(lwcond=*0x%x)", lwcond);

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if ((lwmutex->attribute & SYS_SYNC_ATTR_PROTOCOL_MASK) == SYS_SYNC_RETRY)
	{
		// TODO (protocol ignored)
		//return _sys_lwcond_signal(lwcond->lwcond_queue, 0, -1, 2);
	}

	if (lwmutex->vars.owner.load() == ppu.id)
	{
		// if owns the mutex
		lwmutex->all_info++;

		// call the syscall
		if (s32 res = _sys_lwcond_signal(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, -1, 1))
		{
			ppu.test_state();
			lwmutex->all_info--;

			return res == CELL_EPERM ? CELL_OK : res;
		}

		return CELL_OK;
	}

	if (s32 res = sys_lwmutex_trylock(ppu, lwmutex))
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
	if (s32 res = _sys_lwcond_signal(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, -1, 3))
	{
		ppu.test_state();
		lwmutex->all_info--;

		// unlock the lightweight mutex
		sys_lwmutex_unlock(ppu, lwmutex);

		return res == CELL_ENOENT ? CELL_OK : res;
	}

	return CELL_OK;
}

s32 sys_lwcond_signal_all(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.trace("sys_lwcond_signal_all(lwcond=*0x%x)", lwcond);

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if ((lwmutex->attribute & SYS_SYNC_ATTR_PROTOCOL_MASK) == SYS_SYNC_RETRY)
	{
		// TODO (protocol ignored)
		//return _sys_lwcond_signal_all(lwcond->lwcond_queue, lwmutex->sleep_queue, 2);
	}

	if (lwmutex->vars.owner.load() == ppu.id)
	{
		// if owns the mutex, call the syscall
		const s32 res = _sys_lwcond_signal_all(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, 1);

		if (res <= 0)
		{
			// return error or CELL_OK
			return res;
		}

		ppu.test_state();
		lwmutex->all_info += res;

		return CELL_OK;
	}

	if (s32 res = sys_lwmutex_trylock(ppu, lwmutex))
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
	s32 res = _sys_lwcond_signal_all(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, 1);

	ppu.test_state();

	if (res > 0)
	{
		lwmutex->all_info += res;

		res = CELL_OK;
	}

	// unlock mutex
	sys_lwmutex_unlock(ppu, lwmutex);

	return res;
}

s32 sys_lwcond_signal_to(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond, u32 ppu_thread_id)
{
	sysPrxForUser.trace("sys_lwcond_signal_to(lwcond=*0x%x, ppu_thread_id=0x%x)", lwcond, ppu_thread_id);

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if ((lwmutex->attribute & SYS_SYNC_ATTR_PROTOCOL_MASK) == SYS_SYNC_RETRY)
	{
		// TODO (protocol ignored)
		//return _sys_lwcond_signal(lwcond->lwcond_queue, 0, ppu_thread_id, 2);
	}

	if (lwmutex->vars.owner.load() == ppu.id)
	{
		// if owns the mutex
		lwmutex->all_info++;

		// call the syscall
		if (s32 res = _sys_lwcond_signal(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, ppu_thread_id, 1))
		{
			ppu.test_state();
			lwmutex->all_info--;

			return res;
		}

		return CELL_OK;
	}

	if (s32 res = sys_lwmutex_trylock(ppu, lwmutex))
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
	if (s32 res = _sys_lwcond_signal(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, ppu_thread_id, 3))
	{
		ppu.test_state();
		lwmutex->all_info--;

		// unlock the lightweight mutex
		sys_lwmutex_unlock(ppu, lwmutex);

		return res;
	}

	return CELL_OK;
}

s32 sys_lwcond_wait(ppu_thread& ppu, vm::ptr<sys_lwcond_t> lwcond, u64 timeout)
{
	sysPrxForUser.trace("sys_lwcond_wait(lwcond=*0x%x, timeout=0x%llx)", lwcond, timeout);

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
	s32 res = _sys_lwcond_queue_wait(ppu, lwcond->lwcond_queue, lwmutex->sleep_queue, timeout);

	if (res == CELL_OK || res == CELL_ESRCH)
	{
		if (res == CELL_OK)
		{
			lwmutex->all_info--;
		}

		// restore owner and recursive value
		const auto old = lwmutex->vars.owner.exchange(tid);
		lwmutex->recursive_count = recursive_value;

		if (old != lwmutex_reserved)
		{
			fmt::throw_exception("Locking failed (lwmutex=*0x%x, owner=0x%x)" HERE, lwmutex, old);
		}

		return res;
	}

	if (res == CELL_EBUSY || res == CELL_ETIMEDOUT)
	{
		const s32 res2 = sys_lwmutex_lock(ppu, lwmutex, 0);

		if (res2 == CELL_OK)
		{
			// if successfully locked, restore recursive value
			lwmutex->recursive_count = recursive_value;

			return res == CELL_EBUSY ? CELL_OK : res;
		}

		return res2;
	}

	if (res == CELL_EDEADLK)
	{
		// restore owner and recursive value
		const auto old = lwmutex->vars.owner.exchange(tid);
		lwmutex->recursive_count = recursive_value;

		if (old != lwmutex_reserved)
		{
			fmt::throw_exception("Locking failed (lwmutex=*0x%x, owner=0x%x)" HERE, lwmutex, old);
		}

		return CELL_ETIMEDOUT;
	}

	fmt::throw_exception("Unexpected syscall result (lwcond=*0x%x, result=0x%x)" HERE, lwcond, res);
}

void sysPrxForUser_sys_lwcond_init()
{
	REG_FUNC(sysPrxForUser, sys_lwcond_create);
	REG_FUNC(sysPrxForUser, sys_lwcond_destroy);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_all);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_to);
	REG_FUNC(sysPrxForUser, sys_lwcond_wait);
}
