#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_lwmutex.h"
#include "sys_lwcond.h"

SysCallBase sys_lwcond("sys_lwcond");

s32 lwcond_create(sys_lwcond_t& lwcond, sys_lwmutex_t& lwmutex, u64 name_u64)
{
	u32 id = sys_lwcond.GetNewId(new Lwcond(name_u64), TYPE_LWCOND);
	u32 addr = Memory.RealToVirtualAddr(&lwmutex);
	lwcond.lwmutex.set(be_t<u32>::make(addr));
	lwcond.lwcond_queue = id;

	std::string name((const char*)&name_u64, 8);

	sys_lwcond.Warning("*** lwcond created [%s] (lwmutex_addr=0x%x): id = %d",
		name.c_str(), addr, id);

	Emu.GetSyncPrimManager().AddSyncPrimData(TYPE_LWCOND, id, name);

	return CELL_OK;
}

s32 sys_lwcond_create(vm::ptr<sys_lwcond_t> lwcond, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwcond_attribute_t> attr)
{
	sys_lwcond.Log("sys_lwcond_create(lwcond_addr=0x%x, lwmutex_addr=0x%x, attr_addr=0x%x)",
		lwcond.addr(), lwmutex.addr(), attr.addr());

	return lwcond_create(*lwcond, *lwmutex, attr->name_u64);
}

s32 sys_lwcond_destroy(vm::ptr<sys_lwcond_t> lwcond)
{
	sys_lwcond.Warning("sys_lwcond_destroy(lwcond_addr=0x%x)", lwcond.addr());

	u32 id = lwcond->lwcond_queue;

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData(id, lw))
	{
		return CELL_ESRCH;
	}

	if (!lw->m_queue.finalize())
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(id);
	Emu.GetSyncPrimManager().EraseSyncPrimData(TYPE_LWCOND, id);
	return CELL_OK;
}

s32 sys_lwcond_signal(vm::ptr<sys_lwcond_t> lwcond)
{
	sys_lwcond.Log("sys_lwcond_signal(lwcond_addr=0x%x)", lwcond.addr());

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	auto mutex = vm::ptr<sys_lwmutex_t>::make(lwcond->lwmutex.addr());

	if (u32 target = (mutex->attribute.ToBE() == se32(SYS_SYNC_PRIORITY) ? lw->m_queue.pop_prio() : lw->m_queue.pop()))
	{
		lw->signal.lock(target);

		if (Emu.IsStopped())
		{
			sys_lwcond.Warning("sys_lwcond_signal(id=%d) aborted", (u32)lwcond->lwcond_queue);
			return CELL_OK;
		}
	}

	return CELL_OK;
}

s32 sys_lwcond_signal_all(vm::ptr<sys_lwcond_t> lwcond)
{
	sys_lwcond.Log("sys_lwcond_signal_all(lwcond_addr=0x%x)", lwcond.addr());

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	auto mutex = vm::ptr<sys_lwmutex_t>::make(lwcond->lwmutex.addr());

	while (u32 target = (mutex->attribute.ToBE() == se32(SYS_SYNC_PRIORITY) ? lw->m_queue.pop_prio() : lw->m_queue.pop()))
	{
		lw->signal.lock(target);

		if (Emu.IsStopped())
		{
			sys_lwcond.Warning("sys_lwcond_signal_all(id=%d) aborted", (u32)lwcond->lwcond_queue);
			return CELL_OK;
		}
	}

	return CELL_OK;
}

s32 sys_lwcond_signal_to(vm::ptr<sys_lwcond_t> lwcond, u32 ppu_thread_id)
{
	sys_lwcond.Log("sys_lwcond_signal_to(lwcond_addr=0x%x, ppu_thread_id=%d)", lwcond.addr(), ppu_thread_id);

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	if (!Emu.GetIdManager().CheckID(ppu_thread_id))
	{
		return CELL_ESRCH;
	}

	if (!lw->m_queue.invalidate(ppu_thread_id))
	{
		return CELL_EPERM;
	}

	u32 target = ppu_thread_id;
	{
		lw->signal.lock(target);

		if (Emu.IsStopped())
		{
			sys_lwcond.Warning("sys_lwcond_signal_to(id=%d, to=%d) aborted", (u32)lwcond->lwcond_queue, ppu_thread_id);
			return CELL_OK;
		}
	}

	return CELL_OK;
}

s32 sys_lwcond_wait(vm::ptr<sys_lwcond_t> lwcond, u64 timeout)
{
	sys_lwcond.Log("sys_lwcond_wait(lwcond_addr=0x%x, timeout=%lld)", lwcond.addr(), timeout);

	Lwcond* lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	auto mutex = vm::ptr<sys_lwmutex_t>::make(lwcond->lwmutex.addr());
	u32 tid_le = GetCurrentPPUThread().GetId();
	be_t<u32> tid = be_t<u32>::make(tid_le);

	SleepQueue* sq = nullptr;
	Emu.GetIdManager().GetIDData((u32)mutex->sleep_queue, sq);

	if (mutex->mutex.GetOwner() != tid)
	{
		sys_lwcond.Warning("sys_lwcond_wait(id=%d) failed (EPERM)", (u32)lwcond->lwcond_queue);
		return CELL_EPERM; // caller must own this lwmutex
	}

	lw->m_queue.push(tid_le);

	if (mutex->recursive_count.ToBE() != se32(1))
	{
		sys_lwcond.Warning("sys_lwcond_wait(id=%d): associated mutex had wrong recursive value (%d)",
			(u32)lwcond->lwcond_queue, (u32)mutex->recursive_count);
	}
	mutex->recursive_count = 0;

	if (sq)
	{
		mutex->mutex.unlock(tid, be_t<u32>::make(mutex->attribute.ToBE() == se32(SYS_SYNC_PRIORITY) ? sq->pop_prio() : sq->pop()));
	}
	else if (mutex->attribute.ToBE() == se32(SYS_SYNC_RETRY))
	{
		mutex->mutex.unlock(tid); // SYS_SYNC_RETRY
	}
	else
	{
		sys_lwcond.Warning("sys_lwcond_wait(id=%d): associated mutex had invalid sleep queue (%d)",
			(u32)lwcond->lwcond_queue, (u32)mutex->sleep_queue);
	}

	u64 counter = 0;
	const u64 max_counter = timeout ? (timeout / 1000) : ~0;

	while (true)
	{
		if (lw->signal.unlock(tid, tid) == SMR_OK)
		{
			switch (mutex->lock(tid, 0))
			{
			case CELL_OK: break;
			case static_cast<int>(CELL_EDEADLK): sys_lwcond.Warning("sys_lwcond_wait(id=%d): associated mutex was locked",
								   (u32)lwcond->lwcond_queue); return CELL_OK;
			case static_cast<int>(CELL_ESRCH): sys_lwcond.Warning("sys_lwcond_wait(id=%d): associated mutex not found (%d)",
								 (u32)lwcond->lwcond_queue, (u32)mutex->sleep_queue); return CELL_ESRCH;
			case static_cast<int>(CELL_EINVAL): goto abort;
			}

			mutex->recursive_count = 1;
			lw->signal.unlock(tid);
			return CELL_OK;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (counter++ > max_counter)
		{
			lw->m_queue.invalidate(tid_le);
			return CELL_ETIMEDOUT;
		}
		if (Emu.IsStopped())
		{
			goto abort;
		}
	}

abort:
	sys_lwcond.Warning("sys_lwcond_wait(id=%d) aborted", (u32)lwcond->lwcond_queue);
	return CELL_OK;
}
