#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "sysPrxForUser.h"

extern Module sysPrxForUser;

s32 sys_lwmutex_create(vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr)
{
	sysPrxForUser.Warning("sys_lwmutex_create(lwmutex=*0x%x, attr=*0x%x)", lwmutex, attr);

	const bool recursive = attr->recursive == SYS_SYNC_RECURSIVE;

	if (!recursive && attr->recursive != SYS_SYNC_NOT_RECURSIVE)
	{
		sysPrxForUser.Error("sys_lwmutex_create(): invalid recursive attribute (0x%x)", attr->recursive);
		return CELL_EINVAL;
	}

	const u32 protocol = attr->protocol;

	switch (protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_RETRY: break;
	case SYS_SYNC_PRIORITY: break;
	default: sysPrxForUser.Error("sys_lwmutex_create(): invalid protocol (0x%x)", protocol); return CELL_EINVAL;
	}

	lwmutex->lock_var = { { lwmutex_free, 0 } };
	lwmutex->attribute = attr->recursive | attr->protocol;
	lwmutex->recursive_count = 0;
	lwmutex->sleep_queue = idm::make<lv2_lwmutex_t>(protocol, attr->name_u64);

	return CELL_OK;
}

s32 sys_lwmutex_destroy(PPUThread& ppu, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.Log("sys_lwmutex_destroy(lwmutex=*0x%x)", lwmutex);

	// check to prevent recursive locking in the next call
	if (lwmutex->vars.owner.load() == ppu.get_id())
	{
		return CELL_EBUSY;
	}

	// attempt to lock the mutex
	if (s32 res = sys_lwmutex_trylock(ppu, lwmutex))
	{
		return res;
	}

	// call the syscall
	if (s32 res = _sys_lwmutex_destroy(lwmutex->sleep_queue))
	{
		// unlock the mutex if failed
		sys_lwmutex_unlock(ppu, lwmutex);

		return res;
	}

	// deleting succeeded
	lwmutex->vars.owner.exchange(lwmutex_dead);

	return CELL_OK;
}

s32 sys_lwmutex_lock(PPUThread& ppu, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sysPrxForUser.Log("sys_lwmutex_lock(lwmutex=*0x%x, timeout=0x%llx)", lwmutex, timeout);

	const be_t<u32> tid = ppu.get_id();

	// try to lock lightweight mutex
	const be_t<u32> old_owner = lwmutex->vars.owner.compare_and_swap(lwmutex_free, tid);

	if (old_owner == lwmutex_free)
	{
		// locking succeeded
		return CELL_OK;
	}

	if (old_owner.data() == tid.data())
	{
		// recursive locking

		if ((lwmutex->attribute & SYS_SYNC_RECURSIVE) == 0)
		{
			// if not recursive
			return CELL_EDEADLK;
		}

		if (lwmutex->recursive_count.data() == -1)
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

	for (u32 i = 0; i < 300; i++)
	{
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
		lwmutex->all_info--;

		return CELL_OK;
	}

	// lock using the syscall
	const s32 res = _sys_lwmutex_lock(ppu, lwmutex->sleep_queue, timeout);

	lwmutex->all_info--;

	if (res == CELL_OK)
	{
		// locking succeeded
		auto old = lwmutex->vars.owner.exchange(tid);

		if (old != lwmutex_reserved)
		{
			throw EXCEPTION("Locking failed (lwmutex=*0x%x, owner=0x%x)", lwmutex, old);
		}

		return CELL_OK;
	}

	if (res == CELL_EBUSY && lwmutex->attribute & SYS_SYNC_RETRY)
	{
		// TODO (protocol is ignored in current implementation)
		throw EXCEPTION("");
	}

	return res;
}

s32 sys_lwmutex_trylock(PPUThread& ppu, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.Log("sys_lwmutex_trylock(lwmutex=*0x%x)", lwmutex);

	const be_t<u32> tid = ppu.get_id();

	// try to lock lightweight mutex
	const be_t<u32> old_owner = lwmutex->vars.owner.compare_and_swap(lwmutex_free, tid);

	if (old_owner == lwmutex_free)
	{
		// locking succeeded
		return CELL_OK;
	}

	if (old_owner.data() == tid.data())
	{
		// recursive locking

		if ((lwmutex->attribute & SYS_SYNC_RECURSIVE) == 0)
		{
			// if not recursive
			return CELL_EDEADLK;
		}

		if (lwmutex->recursive_count.data() == -1)
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
		const s32 res = _sys_lwmutex_trylock(lwmutex->sleep_queue);

		if (res == CELL_OK)
		{
			// locking succeeded
			auto old = lwmutex->vars.owner.exchange(tid);

			if (old != lwmutex_reserved)
			{
				throw EXCEPTION("Locking failed (lwmutex=*0x%x, owner=0x%x)", lwmutex, old);
			}
		}

		return res;
	}

	// locked by another thread
	return CELL_EBUSY;
}

s32 sys_lwmutex_unlock(PPUThread& ppu, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.Log("sys_lwmutex_unlock(lwmutex=*0x%x)", lwmutex);

	const be_t<u32> tid = ppu.get_id();

	// check owner
	if (lwmutex->vars.owner.load() != tid)
	{
		return CELL_EPERM;
	}

	if (lwmutex->recursive_count.data())
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
	if (_sys_lwmutex_unlock(lwmutex->sleep_queue) == CELL_ESRCH)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

u32 g_ppu_func_index__sys_lwmutex_unlock; // test

void sysPrxForUser_sys_lwmutex_init()
{
	REG_FUNC(sysPrxForUser, sys_lwmutex_create);
	REG_FUNC(sysPrxForUser, sys_lwmutex_destroy);
	REG_FUNC(sysPrxForUser, sys_lwmutex_lock);
	REG_FUNC(sysPrxForUser, sys_lwmutex_trylock);
	g_ppu_func_index__sys_lwmutex_unlock = REG_FUNC(sysPrxForUser, sys_lwmutex_unlock); // test
}
