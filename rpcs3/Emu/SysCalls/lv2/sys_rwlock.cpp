#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sleep_queue.h"
#include "sys_time.h"
#include "sys_rwlock.h"

SysCallBase sys_rwlock("sys_rwlock");

s32 sys_rwlock_create(vm::ptr<u32> rw_lock_id, vm::ptr<sys_rwlock_attribute_t> attr)
{
	sys_rwlock.Warning("sys_rwlock_create(rw_lock_id_addr=0x%x, attr_addr=0x%x)", rw_lock_id.addr(), attr.addr());

	if (!attr)
	{
		sys_rwlock.Error("sys_rwlock_create(): null attr address");
		return CELL_EFAULT;
	}

	switch (attr->protocol.data())
	{
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_RETRY): sys_rwlock.Error("Invalid protocol (SYS_SYNC_RETRY)"); return CELL_EINVAL;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_rwlock.Todo("SYS_SYNC_PRIORITY_INHERIT"); break;
	case se32(SYS_SYNC_FIFO): break;
	default: sys_rwlock.Error("Unknown protocol (0x%x)", attr->protocol); return CELL_EINVAL;
	}

	if (attr->pshared.data() != se32(0x200))
	{
		sys_rwlock.Error("Unknown pshared attribute (0x%x)", attr->pshared);
		return CELL_EINVAL;
	}

	std::shared_ptr<RWLock> rw(new RWLock(attr->protocol, attr->name_u64));
	const u32 id = Emu.GetIdManager().GetNewID(rw, TYPE_RWLOCK);
	*rw_lock_id = id;
	rw->wqueue.set_full_name(fmt::Format("Rwlock(%d)", id));

	sys_rwlock.Warning("*** rwlock created [%s] (protocol=0x%x): id = %d", std::string(attr->name, 8).c_str(), rw->protocol, id);
	return CELL_OK;
}

s32 sys_rwlock_destroy(u32 rw_lock_id)
{
	sys_rwlock.Warning("sys_rwlock_destroy(rw_lock_id=%d)", rw_lock_id);

	std::shared_ptr<RWLock> rw;
	if (!Emu.GetIdManager().GetIDData(rw_lock_id, rw))
	{
		return CELL_ESRCH;
	}

	if (!rw->sync.compare_and_swap_test({ 0, 0 }, { ~0u, ~0u })) // check if locked and make unusable
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(rw_lock_id);
	return CELL_OK;
}

s32 sys_rwlock_rlock(u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.Log("sys_rwlock_rlock(rw_lock_id=%d, timeout=%lld)", rw_lock_id, timeout);

	const u64 start_time = get_system_time();

	std::shared_ptr<RWLock> rw;
	if (!Emu.GetIdManager().GetIDData(rw_lock_id, rw))
	{
		return CELL_ESRCH;
	}

	while (true)
	{
		bool succeeded;
		rw->sync.atomic_op_sync([&succeeded](RWLock::sync_var_t& sync)
		{
			assert(~sync.readers);
			if ((succeeded = !sync.writer))
			{
				sync.readers++;
			}
		});

		if (succeeded)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (timeout && get_system_time() - start_time > timeout)
		{
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_rwlock.Warning("sys_rwlock_rlock(id=%d) aborted", rw_lock_id);
			return CELL_OK;
		}
	}

	return CELL_OK;
}

s32 sys_rwlock_tryrlock(u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_tryrlock(rw_lock_id=%d)", rw_lock_id);

	std::shared_ptr<RWLock> rw;
	if (!Emu.GetIdManager().GetIDData(rw_lock_id, rw))
	{
		return CELL_ESRCH;
	}

	bool succeeded;
	rw->sync.atomic_op_sync([&succeeded](RWLock::sync_var_t& sync)
	{
		assert(~sync.readers);
		if ((succeeded = !sync.writer))
		{
			sync.readers++;
		}
	});

	if (succeeded)
	{
		return CELL_OK;
	}

	return CELL_EBUSY;
}

s32 sys_rwlock_runlock(u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_runlock(rw_lock_id=%d)", rw_lock_id);

	std::shared_ptr<RWLock> rw;
	if (!Emu.GetIdManager().GetIDData(rw_lock_id, rw))
	{
		return CELL_ESRCH;
	}

	bool succeeded;
	rw->sync.atomic_op_sync([&succeeded](RWLock::sync_var_t& sync)
	{
		if ((succeeded = sync.readers != 0))
		{
			assert(!sync.writer);
			sync.readers--;
		}
	});

	if (succeeded)
	{
		return CELL_OK;
	}

	return CELL_EPERM;
}

s32 sys_rwlock_wlock(PPUThread& CPU, u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.Log("sys_rwlock_wlock(rw_lock_id=%d, timeout=%lld)", rw_lock_id, timeout);

	const u64 start_time = get_system_time();

	std::shared_ptr<RWLock> rw;
	if (!Emu.GetIdManager().GetIDData(rw_lock_id, rw))
	{
		return CELL_ESRCH;
	}

	const u32 tid = CPU.GetId();

	if (rw->sync.compare_and_swap_test({ 0, 0 }, { 0, tid }))
	{
		return CELL_OK;
	}

	if (rw->sync.read_relaxed().writer == tid)
	{
		return CELL_EDEADLK;
	}

	rw->wqueue.push(tid, rw->protocol);

	while (true)
	{
		auto old_sync = rw->sync.compare_and_swap({ 0, 0 }, { 0, tid });
		if (!old_sync.readers && (!old_sync.writer || old_sync.writer == tid))
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (timeout && get_system_time() - start_time > timeout)
		{
			if (!rw->wqueue.invalidate(tid, rw->protocol))
			{
				assert(!"sys_rwlock_wlock() failed (timeout)");
			}
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_rwlock.Warning("sys_rwlock_wlock(id=%d) aborted", rw_lock_id);
			return CELL_OK;
		}
	}

	if (!rw->wqueue.invalidate(tid, rw->protocol) && !rw->wqueue.pop(tid, rw->protocol))
	{
		assert(!"sys_rwlock_wlock() failed (locking)");
	}
	return CELL_OK;
}

s32 sys_rwlock_trywlock(PPUThread& CPU, u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_trywlock(rw_lock_id=%d)", rw_lock_id);

	std::shared_ptr<RWLock> rw;
	if (!Emu.GetIdManager().GetIDData(rw_lock_id, rw))
	{
		return CELL_ESRCH;
	}

	const u32 tid = CPU.GetId();

	if (rw->sync.compare_and_swap_test({ 0, 0 }, { 0, tid }))
	{
		return CELL_OK;
	}

	if (rw->sync.read_relaxed().writer == tid)
	{
		return CELL_EDEADLK;
	}

	return CELL_EBUSY;
}

s32 sys_rwlock_wunlock(PPUThread& CPU, u32 rw_lock_id)
{
	sys_rwlock.Log("sys_rwlock_wunlock(rw_lock_id=%d)", rw_lock_id);

	std::shared_ptr<RWLock> rw;
	if (!Emu.GetIdManager().GetIDData(rw_lock_id, rw))
	{
		return CELL_ESRCH;
	}

	const u32 tid = CPU.GetId();
	const u32 target = rw->wqueue.signal(rw->protocol);

	if (rw->sync.compare_and_swap_test({ 0, tid }, { 0, target }))
	{
		if (!target)
		{
			// TODO: signal readers
		}
		return CELL_OK;
	}

	return CELL_EPERM;
}
