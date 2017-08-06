#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_mutex.h"
#include "sysPrxForUser.h"

extern logs::channel sysPrxForUser;

extern bool g_avoid_lwm;

error_code sys_lwmutex_create(vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr)
{
	sysPrxForUser.trace("sys_lwmutex_create(lwmutex=*0x%x, attr=*0x%x)", lwmutex, attr);

	const u32 recursive = attr->recursive;

	if (recursive != SYS_SYNC_RECURSIVE && recursive != SYS_SYNC_NOT_RECURSIVE)
	{
		sysPrxForUser.error("sys_lwmutex_create(): invalid recursive attribute (0x%x)", recursive);
		return CELL_EINVAL;
	}

	const u32 protocol = attr->protocol;

	switch (protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_RETRY: break;
	case SYS_SYNC_PRIORITY: break;
	default: sysPrxForUser.error("sys_lwmutex_create(): invalid protocol (0x%x)", protocol); return CELL_EINVAL;
	}

	vm::var<u32> out_id;
	vm::var<sys_mutex_attribute_t> attrs;
	attrs->protocol  = attr->protocol;
	attrs->recursive = attr->recursive;
	attrs->pshared   = SYS_SYNC_NOT_PROCESS_SHARED;
	attrs->adaptive  = SYS_SYNC_NOT_ADAPTIVE;
	attrs->ipc_key   = 0;
	attrs->flags     = 0;
	attrs->name_u64  = attr->name_u64;

	if (error_code res = g_avoid_lwm ? sys_mutex_create(out_id, attrs) : _sys_lwmutex_create(out_id, protocol, lwmutex, 0x80000001, attr->name_u64, 0))
	{
		return res;
	}

	lwmutex->lock_var.store({ lwmutex_free, 0 });
	lwmutex->attribute = attr->recursive | attr->protocol;
	lwmutex->recursive_count = 0;
	lwmutex->sleep_queue = *out_id;
	return CELL_OK;
}

error_code sys_lwmutex_destroy(ppu_thread& ppu, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.trace("sys_lwmutex_destroy(lwmutex=*0x%x)", lwmutex);

	if (g_avoid_lwm)
	{
		return sys_mutex_destroy(lwmutex->sleep_queue);
	}

	// check to prevent recursive locking in the next call
	if (lwmutex->vars.owner.load() == ppu.id)
	{
		return CELL_EBUSY;
	}

	// attempt to lock the mutex
	if (error_code res = sys_lwmutex_trylock(ppu, lwmutex))
	{
		return res;
	}

	// call the syscall
	if (error_code res = _sys_lwmutex_destroy(lwmutex->sleep_queue))
	{
		// unlock the mutex if failed
		sys_lwmutex_unlock(ppu, lwmutex);

		return res;
	}

	// deleting succeeded
	lwmutex->vars.owner.exchange(lwmutex_dead);

	return CELL_OK;
}

error_code sys_lwmutex_lock(ppu_thread& ppu, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sysPrxForUser.trace("sys_lwmutex_lock(lwmutex=*0x%x, timeout=0x%llx)", lwmutex, timeout);

	if (g_avoid_lwm)
	{
		return sys_mutex_lock(ppu, lwmutex->sleep_queue, timeout);
	}

	const be_t<u32> tid(ppu.id);

	// try to lock lightweight mutex
	const be_t<u32> old_owner = lwmutex->vars.owner.compare_and_swap(lwmutex_free, tid);

	if (old_owner == lwmutex_free)
	{
		// locking succeeded
		return CELL_OK;
	}

	if (old_owner == tid)
	{
		// recursive locking

		if ((lwmutex->attribute & SYS_SYNC_RECURSIVE) == 0)
		{
			// if not recursive
			return CELL_EDEADLK;
		}

		if (lwmutex->recursive_count == -1)
		{
			// if recursion limit reached
			return CELL_EKRESOURCE;
		}

		// recursive locking succeeded
		lwmutex->recursive_count++;
		_mm_mfence();

		return CELL_OK;
	}

	if (old_owner == lwmutex_dead)
	{
		// invalid or deleted mutex
		return CELL_EINVAL;
	}

	for (u32 i = 0; i < 10; i++)
	{
		busy_wait();

		if (lwmutex->vars.owner.load() == lwmutex_free)
		{
			if (lwmutex->vars.owner.compare_and_swap_test(lwmutex_free, tid))
			{
				// locking succeeded
				return CELL_OK;
			}
		}
	}

	// atomically increment waiter value using 64 bit op
	lwmutex->all_info++;

	if (lwmutex->vars.owner.compare_and_swap_test(lwmutex_free, tid))
	{
		// locking succeeded
		--lwmutex->all_info;

		return CELL_OK;
	}

	// lock using the syscall
	const error_code res = _sys_lwmutex_lock(ppu, lwmutex->sleep_queue, timeout);

	lwmutex->all_info--;

	if (res == CELL_OK)
	{
		// locking succeeded
		auto old = lwmutex->vars.owner.exchange(tid);

		if (old != lwmutex_reserved)
		{
			fmt::throw_exception("Locking failed (lwmutex=*0x%x, owner=0x%x)" HERE, lwmutex, old);
		}

		return CELL_OK;
	}

	if (res == CELL_EBUSY && lwmutex->attribute & SYS_SYNC_RETRY)
	{
		// TODO (protocol is ignored in current implementation)
		fmt::throw_exception("Unimplemented" HERE);
	}

	return res;
}

error_code sys_lwmutex_trylock(ppu_thread& ppu, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.trace("sys_lwmutex_trylock(lwmutex=*0x%x)", lwmutex);

	if (g_avoid_lwm)
	{
		return sys_mutex_trylock(ppu, lwmutex->sleep_queue);
	}

	const be_t<u32> tid(ppu.id);

	// try to lock lightweight mutex
	const be_t<u32> old_owner = lwmutex->vars.owner.compare_and_swap(lwmutex_free, tid);

	if (old_owner == lwmutex_free)
	{
		// locking succeeded
		return CELL_OK;
	}

	if (old_owner == tid)
	{
		// recursive locking

		if ((lwmutex->attribute & SYS_SYNC_RECURSIVE) == 0)
		{
			// if not recursive
			return CELL_EDEADLK;
		}

		if (lwmutex->recursive_count == -1)
		{
			// if recursion limit reached
			return CELL_EKRESOURCE;
		}

		// recursive locking succeeded
		lwmutex->recursive_count++;
		_mm_mfence();

		return CELL_OK;
	}

	if (old_owner == lwmutex_dead)
	{
		// invalid or deleted mutex
		return CELL_EINVAL;
	}

	if (old_owner == lwmutex_reserved)
	{
		// should be locked by the syscall
		const error_code res = _sys_lwmutex_trylock(lwmutex->sleep_queue);

		if (res == CELL_OK)
		{
			// locking succeeded
			auto old = lwmutex->vars.owner.exchange(tid);

			if (old != lwmutex_reserved)
			{
				fmt::throw_exception("Locking failed (lwmutex=*0x%x, owner=0x%x)" HERE, lwmutex, old);
			}
		}

		return res;
	}

	// locked by another thread
	return not_an_error(CELL_EBUSY);
}

error_code sys_lwmutex_unlock(ppu_thread& ppu, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.trace("sys_lwmutex_unlock(lwmutex=*0x%x)", lwmutex);

	if (g_avoid_lwm)
	{
		return sys_mutex_unlock(ppu, lwmutex->sleep_queue);
	}

	const be_t<u32> tid(ppu.id);

	// check owner
	if (lwmutex->vars.owner.load() != tid)
	{
		return CELL_EPERM;
	}

	if (lwmutex->recursive_count)
	{
		// recursive unlocking succeeded
		lwmutex->recursive_count--;

		return CELL_OK;
	}

	// ensure that waiter is zero
	if (lwmutex->lock_var.compare_and_swap_test({ tid, 0 }, { lwmutex_free, 0 }))
	{
		// unlocking succeeded
		return CELL_OK;
	}

	if (lwmutex->attribute & SYS_SYNC_RETRY)
	{
		// TODO (protocol is ignored in current implementation)
	}

	// set special value
	lwmutex->vars.owner.exchange(lwmutex_reserved);

	// call the syscall
	if (_sys_lwmutex_unlock(ppu, lwmutex->sleep_queue) == CELL_ESRCH)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

void sysPrxForUser_sys_lwmutex_init()
{
	REG_FUNC(sysPrxForUser, sys_lwmutex_create);
	REG_FUNC(sysPrxForUser, sys_lwmutex_destroy);
	REG_FUNC(sysPrxForUser, sys_lwmutex_lock);
	REG_FUNC(sysPrxForUser, sys_lwmutex_trylock);
	REG_FUNC(sysPrxForUser, sys_lwmutex_unlock);
}
