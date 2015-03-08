#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/Modules/sysPrxForUser.h"
#include "sleep_queue.h"
#include "sys_time.h"
#include "sys_lwmutex.h"
#include "sys_lwcond.h"

SysCallBase sys_lwcond("sys_lwcond");

s32 lwcond_create(sys_lwcond_t& lwcond, sys_lwmutex_t& lwmutex, u64 name_u64)
{
	const u32 addr = vm::get_addr(&lwmutex);

	std::shared_ptr<Lwcond> lw(new Lwcond(name_u64, addr));

	const u32 id = Emu.GetIdManager().GetNewID(lw, TYPE_LWCOND);

	lw->queue.set_full_name(fmt::Format("Lwcond(%d, addr=0x%x)", id, lw->addr));
	lwcond.lwmutex.set(addr);
	lwcond.lwcond_queue = id;

	sys_lwcond.Warning("*** lwcond created [%s] (lwmutex_addr=0x%x): id = %d", std::string((const char*)&name_u64, 8).c_str(), addr, id);
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

	std::shared_ptr<Lwcond> lw;
	if (!Emu.GetIdManager().GetIDData(id, lw))
	{
		return CELL_ESRCH;
	}

	if (lw->queue.count()) // TODO: safely make object unusable
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(id);
	return CELL_OK;
}

s32 sys_lwcond_signal(vm::ptr<sys_lwcond_t> lwcond)
{
	sys_lwcond.Log("sys_lwcond_signal(lwcond_addr=0x%x)", lwcond.addr());

	std::shared_ptr<Lwcond> lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	auto mutex = lwcond->lwmutex.to_le();

	if (u32 target = lw->queue.signal(mutex->attribute))
	{
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

	std::shared_ptr<Lwcond> lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	auto mutex = lwcond->lwmutex.to_le();

	while (u32 target = lw->queue.signal(mutex->attribute))
	{
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

	std::shared_ptr<Lwcond> lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	if (!Emu.GetIdManager().CheckID(ppu_thread_id))
	{
		return CELL_ESRCH;
	}

	if (!lw->queue.signal_selected(ppu_thread_id))
	{
		return CELL_EPERM;
	}

	return CELL_OK;
}

s32 sys_lwcond_wait(PPUThread& CPU, vm::ptr<sys_lwcond_t> lwcond, u64 timeout)
{
	sys_lwcond.Log("sys_lwcond_wait(lwcond_addr=0x%x, timeout=%lld)", lwcond.addr(), timeout);

	const u64 start_time = get_system_time();

	std::shared_ptr<Lwcond> lw;
	if (!Emu.GetIdManager().GetIDData((u32)lwcond->lwcond_queue, lw))
	{
		return CELL_ESRCH;
	}

	auto mutex = lwcond->lwmutex.to_le();
	u32 tid_le = CPU.GetId();
	auto tid = be_t<u32>::make(tid_le);

	std::shared_ptr<sleep_queue_t> sq;
	if (!Emu.GetIdManager().GetIDData((u32)mutex->sleep_queue, sq))
	{
		sys_lwcond.Warning("sys_lwcond_wait(id=%d): associated mutex had invalid sleep queue (%d)",
			(u32)lwcond->lwcond_queue, (u32)mutex->sleep_queue);
		return CELL_ESRCH;
	}

	if (mutex->owner.read_sync() != tid)
	{
		return CELL_EPERM;
	}

	lw->queue.push(tid_le, mutex->attribute);

	auto old_recursive = mutex->recursive_count;
	mutex->recursive_count = 0;

	auto target = be_t<u32>::make(sq->signal(mutex->attribute));
	if (!mutex->owner.compare_and_swap_test(tid, target))
	{
		assert(!"sys_lwcond_wait(): mutex unlocking failed");
	}

	bool signaled = false;
	while (true)
	{
		if ((signaled = signaled || lw->queue.pop(tid, mutex->attribute))) // check signaled threads
		{
			s32 res = sys_lwmutex_lock(CPU, mutex, timeout ? get_system_time() - start_time : 0); // this is bad
			if (res == CELL_OK)
			{
				break;
			}

			switch (res)
			{
			case static_cast<int>(CELL_EDEADLK):
			{
				sys_lwcond.Error("sys_lwcond_wait(id=%d): associated mutex was locked", (u32)lwcond->lwcond_queue);
				return CELL_OK; // mutex not locked (but already locked in the incorrect way)
			}
			case static_cast<int>(CELL_ESRCH):
			{
				sys_lwcond.Error("sys_lwcond_wait(id=%d): associated mutex not found (%d)", (u32)lwcond->lwcond_queue, (u32)mutex->sleep_queue);
				return CELL_ESRCH; // mutex not locked
			}
			case static_cast<int>(CELL_ETIMEDOUT):
			{
				return CELL_ETIMEDOUT; // mutex not locked
			}
			case static_cast<int>(CELL_EINVAL):
			{
				sys_lwcond.Error("sys_lwcond_wait(id=%d): invalid associated mutex (%d)", (u32)lwcond->lwcond_queue, (u32)mutex->sleep_queue);
				return CELL_EINVAL; // mutex not locked
			}
			default:
			{
				sys_lwcond.Error("sys_lwcond_wait(id=%d): mutex->lock() returned 0x%x", (u32)lwcond->lwcond_queue, res);
				return CELL_EINVAL; // mutex not locked
			}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (timeout && get_system_time() - start_time > timeout)
		{
			if (!lw->queue.invalidate(tid_le, mutex->attribute))
			{
				assert(!"sys_lwcond_wait() failed (timeout)");
			}
			return CELL_ETIMEDOUT; // mutex not locked
		}

		if (Emu.IsStopped())
		{
			sys_lwcond.Warning("sys_lwcond_wait(id=%d) aborted", (u32)lwcond->lwcond_queue);
			return CELL_OK;
		}
	}

	mutex->recursive_count = old_recursive;
	return CELL_OK;
}
