#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_mutex.h"

SysCallBase sys_mutex("sys_mutex");

extern u64 get_system_time();

void lv2_mutex_t::unlock(lv2_lock_t& lv2_lock)
{
	CHECK_LV2_LOCK(lv2_lock);

	owner.reset();

	if (sq.size())
	{
		// pick new owner; protocol is ignored in current implementation
		owner = sq.front();

		if (!owner->signal())
		{
			throw EXCEPTION("Mutex owner already signaled");
		}
	}
}

s32 sys_mutex_create(vm::ptr<u32> mutex_id, vm::ptr<sys_mutex_attribute_t> attr)
{
	sys_mutex.Warning("sys_mutex_create(mutex_id=*0x%x, attr=*0x%x)", mutex_id, attr);

	if (!mutex_id || !attr)
	{
		return CELL_EFAULT;
	}

	const u32 protocol = attr->protocol;

	switch (protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_PRIORITY: break;
	case SYS_SYNC_PRIORITY_INHERIT: break;

	default:
	{
		sys_mutex.Error("sys_mutex_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}
	}

	const bool recursive = attr->recursive == SYS_SYNC_RECURSIVE;

	if ((!recursive && attr->recursive != SYS_SYNC_NOT_RECURSIVE) || attr->pshared != SYS_SYNC_NOT_PROCESS_SHARED || attr->adaptive != SYS_SYNC_NOT_ADAPTIVE || attr->ipc_key.data() || attr->flags.data())
	{
		sys_mutex.Error("sys_mutex_create(): unknown attributes (recursive=0x%x, pshared=0x%x, adaptive=0x%x, ipc_key=0x%llx, flags=0x%x)", attr->recursive, attr->pshared, attr->adaptive, attr->ipc_key, attr->flags);

		return CELL_EINVAL;
	}

	*mutex_id = Emu.GetIdManager().make<lv2_mutex_t>(recursive, protocol, attr->name_u64);

	return CELL_OK;
}

s32 sys_mutex_destroy(u32 mutex_id)
{
	sys_mutex.Warning("sys_mutex_destroy(mutex_id=0x%x)", mutex_id);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	if (mutex->owner || mutex->sq.size())
	{
		return CELL_EBUSY;
	}

	if (mutex->cond_count)
	{
		return CELL_EPERM;
	}

	Emu.GetIdManager().remove<lv2_mutex_t>(mutex_id);

	return CELL_OK;
}

s32 sys_mutex_lock(PPUThread& ppu, u32 mutex_id, u64 timeout)
{
	sys_mutex.Log("sys_mutex_lock(mutex_id=0x%x, timeout=0x%llx)", mutex_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	// check current ownership
	if (mutex->owner.get() == &ppu)
	{
		if (mutex->recursive)
		{
			if (mutex->recursive_count == 0xffffffffu)
			{
				return CELL_EKRESOURCE;
			}

			mutex->recursive_count++;

			return CELL_OK;
		}

		return CELL_EDEADLK;
	}

	// lock immediately if not locked
	if (!mutex->owner)
	{
		mutex->owner = ppu.shared_from_this();

		return CELL_OK;
	}

	// add waiter; protocol is ignored in current implementation
	sleep_queue_entry_t waiter(ppu, mutex->sq);

	while (!ppu.unsignal())
	{
		CHECK_EMU_STATUS;

		if (timeout)
		{
			const u64 passed = get_system_time() - start_time;

			if (passed >= timeout)
			{
				return CELL_ETIMEDOUT;
			}

			ppu.cv.wait_for(lv2_lock, std::chrono::microseconds(timeout - passed));
		}
		else
		{
			ppu.cv.wait(lv2_lock);
		}
	}

	// new owner must be set when unlocked
	if (mutex->owner.get() != &ppu)
	{
		throw EXCEPTION("Unexpected mutex owner");
	}

	return CELL_OK;
}

s32 sys_mutex_trylock(PPUThread& ppu, u32 mutex_id)
{
	sys_mutex.Log("sys_mutex_trylock(mutex_id=0x%x)", mutex_id);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	// check current ownership
	if (mutex->owner.get() == &ppu)
	{
		if (mutex->recursive)
		{
			if (mutex->recursive_count == 0xffffffffu)
			{
				return CELL_EKRESOURCE;
			}

			mutex->recursive_count++;

			return CELL_OK;
		}

		return CELL_EDEADLK;
	}

	if (mutex->owner)
	{
		return CELL_EBUSY;
	}

	// own the mutex if free
	mutex->owner = ppu.shared_from_this();

	return CELL_OK;
}

s32 sys_mutex_unlock(PPUThread& ppu, u32 mutex_id)
{
	sys_mutex.Log("sys_mutex_unlock(mutex_id=0x%x)", mutex_id);

	LV2_LOCK;

	const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(mutex_id);

	if (!mutex)
	{
		return CELL_ESRCH;
	}

	// check current ownership
	if (mutex->owner.get() != &ppu)
	{
		return CELL_EPERM;
	}

	if (mutex->recursive_count)
	{
		if (!mutex->recursive)
		{
			throw EXCEPTION("Unexpected recursive_count");
		}
		
		mutex->recursive_count--;
	}
	else
	{
		mutex->unlock(lv2_lock);
	}

	return CELL_OK;
}
