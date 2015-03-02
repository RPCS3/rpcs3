#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sleep_queue.h"
#include "sys_time.h"
#include "sys_semaphore.h"

SysCallBase sys_semaphore("sys_semaphore");

u32 semaphore_create(s32 initial_count, s32 max_count, u32 protocol, u64 name_u64)
{
	std::shared_ptr<Semaphore> sem(new Semaphore(initial_count, max_count, protocol, name_u64));

	const u32 id = sys_semaphore.GetNewId(sem, TYPE_SEMAPHORE);
	sem->queue.set_full_name(fmt::Format("Semaphore(%d)", id));

	sys_semaphore.Notice("*** semaphore created [%s] (protocol=0x%x): id = %d", std::string((const char*)&name_u64, 8).c_str(), protocol, id);
	return id;
}

s32 sys_semaphore_create(vm::ptr<u32> sem, vm::ptr<sys_semaphore_attribute> attr, s32 initial_count, s32 max_count)
{
	sys_semaphore.Warning("sys_semaphore_create(sem_addr=0x%x, attr_addr=0x%x, initial_count=%d, max_count=%d)",
		sem.addr(), attr.addr(), initial_count, max_count);

	if (!sem)
	{
		sys_semaphore.Error("sys_semaphore_create(): invalid memory access (sem_addr=0x%x)", sem.addr());
		return CELL_EFAULT;
	}

	if (!attr)
	{
		sys_semaphore.Error("sys_semaphore_create(): An invalid argument value is specified (attr_addr=0x%x)", attr.addr());
		return CELL_EFAULT;
	}

	if (max_count <= 0 || initial_count > max_count || initial_count < 0)
	{
		sys_semaphore.Error("sys_semaphore_create(): invalid parameters (initial_count=%d, max_count=%d)", initial_count, max_count);
		return CELL_EINVAL;
	}

	switch (attr->protocol.data())
	{
	case se32(SYS_SYNC_FIFO): break;
	case se32(SYS_SYNC_PRIORITY): break;
	case se32(SYS_SYNC_PRIORITY_INHERIT): sys_semaphore.Todo("SYS_SYNC_PRIORITY_INHERIT"); break;
	case se32(SYS_SYNC_RETRY): sys_semaphore.Error("Invalid protocol (SYS_SYNC_RETRY)"); return CELL_EINVAL;
	default: sys_semaphore.Error("Unknown protocol attribute (0x%x)", attr->protocol); return CELL_EINVAL;
	}

	if (attr->pshared.data() != se32(0x200))
	{
		sys_semaphore.Error("Unknown pshared attribute (0x%x)", attr->pshared);
		return CELL_EINVAL;
	}

	*sem = semaphore_create(initial_count, max_count, attr->protocol, attr->name_u64);
	return CELL_OK;
}

s32 sys_semaphore_destroy(u32 sem_id)
{
	sys_semaphore.Warning("sys_semaphore_destroy(sem_id=%d)", sem_id);

	std::shared_ptr<Semaphore> sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}
	
	if (sem->queue.count()) // TODO: safely make object unusable
	{
		return CELL_EBUSY;
	}

	Emu.GetIdManager().RemoveID(sem_id);
	return CELL_OK;
}

s32 sys_semaphore_wait(u32 sem_id, u64 timeout)
{
	sys_semaphore.Log("sys_semaphore_wait(sem_id=%d, timeout=%lld)", sem_id, timeout);

	const u64 start_time = get_system_time();

	std::shared_ptr<Semaphore> sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	const u32 tid = GetCurrentPPUThread().GetId();
	s32 old_value;

	{
		sem->value.atomic_op_sync([&old_value](s32& value)
		{
			old_value = value;
			if (value > 0)
			{
				value--;
			}
		});

		if (old_value > 0)
		{
			return CELL_OK;
		}

		sem->queue.push(tid, sem->protocol);
	}
	

	while (true)
	{
		if (sem->queue.pop(tid, sem->protocol))
		{
			break;
		}

		assert(!sem->value.read_sync());
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (timeout && get_system_time() - start_time > timeout)
		{
			if (!sem->queue.invalidate(tid, sem->protocol))
			{
				if (sem->queue.pop(tid, sem->protocol))
				{
					return CELL_OK;
				}
				assert(!"sys_semaphore_wait() failed (timeout)");
			}
			return CELL_ETIMEDOUT;
		}

		if (Emu.IsStopped())
		{
			sys_semaphore.Warning("sys_semaphore_wait(%d) aborted", sem_id);
			return CELL_OK;
		}
	}

	return CELL_OK;
}

s32 sys_semaphore_trywait(u32 sem_id)
{
	sys_semaphore.Log("sys_semaphore_trywait(sem_id=%d)", sem_id);

	std::shared_ptr<Semaphore> sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	s32 old_value;

	sem->value.atomic_op_sync([&old_value](s32& value)
	{
		old_value = value;
		if (value > 0)
		{
			value--;
		}
	});

	if (old_value > 0)
	{
		return CELL_OK;
	}
	else
	{
		return CELL_EBUSY;
	}
}

s32 sys_semaphore_post(u32 sem_id, s32 count)
{
	sys_semaphore.Log("sys_semaphore_post(sem_id=%d, count=%d)", sem_id, count);

	std::shared_ptr<Semaphore> sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	if (count < 0)
	{
		return CELL_EINVAL;
	}

	if (count + sem->value.read_sync() - (s32)sem->queue.count() > sem->max)
	{
		return CELL_EBUSY;
	}

	while (count > 0)
	{
		if (Emu.IsStopped())
		{
			sys_semaphore.Warning("sys_semaphore_post(%d) aborted", sem_id);
			return CELL_OK;
		}

		if (u32 target = sem->queue.signal(sem->protocol))
		{
			count--;
		}
		else
		{
			sem->value.atomic_op([count](s32& value)
			{
				value += count;
			});
			count = 0;
		}
	}

	return CELL_OK;
}

s32 sys_semaphore_get_value(u32 sem_id, vm::ptr<s32> count)
{
	sys_semaphore.Log("sys_semaphore_get_value(sem_id=%d, count_addr=0x%x)", sem_id, count.addr());

	if (!count)
	{
		sys_semaphore.Error("sys_semaphore_get_value(): invalid memory access (addr=0x%x)", count.addr());
		return CELL_EFAULT;
	}

	std::shared_ptr<Semaphore> sem;
	if (!Emu.GetIdManager().GetIDData(sem_id, sem))
	{
		return CELL_ESRCH;
	}

	*count = sem->value.read_sync();
	return CELL_OK;
}
