#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_sync.h"
#include "sys_rwlock.h"

SysCallBase sys_rwlock("sys_rwlock");

extern u64 get_system_time();

void lv2_rwlock_t::notify_all(lv2_lock_t& lv2_lock)
{
	CHECK_LV2_LOCK(lv2_lock);

	// pick a new writer if possible; protocol is ignored in current implementation
	if (!readers && !writer && wsq.size())
	{
		writer = wsq.front();

		if (!writer->signal())
		{
			throw EXCEPTION("Writer already signaled");
		}

		return wsq.pop_front();
	}

	// wakeup all readers if possible
	if (!writer && !wsq.size())
	{
		readers += static_cast<u32>(rsq.size());

		for (auto& thread : rsq)
		{
			if (!thread->signal())
			{
				throw EXCEPTION("Reader already signaled");
			}
		}

		return rsq.clear();
	}
}

s32 sys_rwlock_create(vm::ptr<u32> rw_lock_id, vm::ptr<sys_rwlock_attribute_t> attr)
{
	sys_rwlock.Warning("sys_rwlock_create(rw_lock_id=*0x%x, attr=*0x%x)", rw_lock_id, attr);

	if (!rw_lock_id || !attr)
	{
		return CELL_EFAULT;
	}

	const u32 protocol = attr->protocol;

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_PRIORITY && protocol != SYS_SYNC_PRIORITY_INHERIT)
	{
		sys_rwlock.Error("sys_rwlock_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	if (attr->pshared != SYS_SYNC_NOT_PROCESS_SHARED || attr->ipc_key.data() || attr->flags.data())
	{
		sys_rwlock.Error("sys_rwlock_create(): unknown attributes (pshared=0x%x, ipc_key=0x%llx, flags=0x%x)", attr->pshared, attr->ipc_key, attr->flags);
		return CELL_EINVAL;
	}

	*rw_lock_id = idm::make<lv2_rwlock_t>(protocol, attr->name_u64);

	return CELL_OK;
}

s32 sys_rwlock_destroy(u32 rw_lock_id)
{
	sys_rwlock.Warning("sys_rwlock_destroy(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = idm::get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->readers || rwlock->writer || rwlock->rsq.size() || rwlock->wsq.size())
	{
		return CELL_EBUSY;
	}

	idm::remove<lv2_rwlock_t>(rw_lock_id);

	return CELL_OK;
}

s32 sys_rwlock_rlock(PPUThread& ppu, u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.Log("sys_rwlock_rlock(rw_lock_id=0x%x, timeout=0x%llx)", rw_lock_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto rwlock = idm::get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (!rwlock->writer && rwlock->wsq.empty())
	{
		if (!++rwlock->readers)
		{
			throw EXCEPTION("Too many readers");
		}

		return CELL_OK;
	}

	// add waiter; protocol is ignored in current implementation
	sleep_queue_entry_t waiter(ppu, rwlock->rsq);

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

	if (rwlock->writer || !rwlock->readers)
	{
		throw EXCEPTION("Unexpected");
	}

	return CELL_OK;
}

s32 sys_rwlock_tryrlock(u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_tryrlock(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = idm::get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->writer || rwlock->wsq.size())
	{
		return CELL_EBUSY;
	}

	if (!++rwlock->readers)
	{
		throw EXCEPTION("Too many readers");
	}

	return CELL_OK;
}

s32 sys_rwlock_runlock(u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_runlock(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = idm::get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (!rwlock->readers)
	{
		return CELL_EPERM;
	}

	if (!--rwlock->readers)
	{
		rwlock->notify_all(lv2_lock);
	}

	return CELL_OK;
}

s32 sys_rwlock_wlock(PPUThread& ppu, u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.Log("sys_rwlock_wlock(rw_lock_id=0x%x, timeout=0x%llx)", rw_lock_id, timeout);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto rwlock = idm::get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->writer.get() == &ppu)
	{
		return CELL_EDEADLK;
	}

	if (!rwlock->readers && !rwlock->writer)
	{
		rwlock->writer = ppu.shared_from_this();

		return CELL_OK;
	}

	// add waiter; protocol is ignored in current implementation
	sleep_queue_entry_t waiter(ppu, rwlock->wsq);

	while (!ppu.unsignal())
	{
		CHECK_EMU_STATUS;

		if (timeout)
		{
			const u64 passed = get_system_time() - start_time;

			if (passed >= timeout)
			{
				// if the last waiter quit the writer sleep queue, readers must acquire the lock
				if (!rwlock->writer && rwlock->wsq.size() == 1)
				{
					if (rwlock->wsq.front().get() != &ppu)
					{
						throw EXCEPTION("Unexpected");
					}

					rwlock->wsq.clear();
					rwlock->notify_all(lv2_lock);
				}

				return CELL_ETIMEDOUT;
			}

			ppu.cv.wait_for(lv2_lock, std::chrono::microseconds(timeout - passed));
		}
		else
		{
			ppu.cv.wait(lv2_lock);
		}
	}

	if (rwlock->readers || rwlock->writer.get() != &ppu)
	{
		throw EXCEPTION("Unexpected");
	}

	return CELL_OK;
}

s32 sys_rwlock_trywlock(PPUThread& ppu, u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_trywlock(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = idm::get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->writer.get() == &ppu)
	{
		return CELL_EDEADLK;
	}

	if (rwlock->readers || rwlock->writer || rwlock->wsq.size())
	{
		return CELL_EBUSY;
	}

	rwlock->writer = ppu.shared_from_this();

	return CELL_OK;
}

s32 sys_rwlock_wunlock(PPUThread& ppu, u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_wunlock(rw_lock_id=0x%x)", rw_lock_id);

	LV2_LOCK;

	const auto rwlock = idm::get<lv2_rwlock_t>(rw_lock_id);

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock->writer.get() != &ppu)
	{
		return CELL_EPERM;
	}

	rwlock->writer.reset();

	rwlock->notify_all(lv2_lock);

	return CELL_OK;
}
