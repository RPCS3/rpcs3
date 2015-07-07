#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/IdManager.h"
#include "Emu/DbgCommand.h"

#include "Emu/Cell/PPUThread.h"
#include "sys_mutex.h"
#include "sys_ppu_thread.h"

SysCallBase sys_ppu_thread("sys_ppu_thread");

void _sys_ppu_thread_exit(PPUThread& ppu, u64 errorcode)
{
	sys_ppu_thread.Log("_sys_ppu_thread_exit(errorcode=0x%llx)", errorcode);

	LV2_LOCK;

	// get all sys_mutex objects
	for (auto& _id : Emu.GetIdManager().get_data<lv2_mutex_t>())
	{
		const auto mutex = std::static_pointer_cast<lv2_mutex_t>(_id.data);

		// unlock mutex if locked by this thread
		if (mutex->owner.get() == &ppu)
		{
			mutex->owner.reset();
			
			if (mutex->sq.size())
			{
				mutex->owner = mutex->sq.front();
				mutex->owner->Signal();
			}
		}
	}

	if (!ppu.is_joinable)
	{
		const u32 id = ppu.GetId();

		CallAfter([id]()
		{
			Emu.GetIdManager().remove<PPUThread>(id);
		});
	}

	ppu.Exit();
}

void sys_ppu_thread_yield()
{
	sys_ppu_thread.Log("sys_ppu_thread_yield()");

	std::this_thread::yield();
}

s32 sys_ppu_thread_join(PPUThread& ppu, u32 thread_id, vm::ptr<u64> vptr)
{
	sys_ppu_thread.Warning("sys_ppu_thread_join(thread_id=0x%x, vptr=*0x%x)", thread_id, vptr);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<PPUThread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (!thread->is_joinable || thread->is_joining)
	{
		return CELL_EINVAL;
	}

	if (&ppu == thread.get())
	{
		return CELL_EDEADLK;
	}

	// mark joining
	thread->is_joining = true;

	// join thread
	while (thread->IsActive())
	{
		CHECK_EMU_STATUS;

		ppu.cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	// get exit status from the register
	*vptr = thread->GPR[3];

	// cleanup
	Emu.GetIdManager().remove<PPUThread>(thread->GetId());

	return CELL_OK;
}

s32 sys_ppu_thread_detach(u32 thread_id)
{
	sys_ppu_thread.Warning("sys_ppu_thread_detach(thread_id=0x%x)", thread_id);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<PPUThread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (!thread->is_joinable)
	{
		return CELL_EINVAL;
	}

	if (thread->is_joining)
	{
		return CELL_EBUSY;
	}

	// "detach"
	thread->is_joinable = false;

	return CELL_OK;
}

void sys_ppu_thread_get_join_state(PPUThread& ppu, vm::ptr<s32> isjoinable)
{
	sys_ppu_thread.Warning("sys_ppu_thread_get_join_state(isjoinable=*0x%x)", isjoinable);

	LV2_LOCK;

	*isjoinable = ppu.is_joinable;
}

s32 sys_ppu_thread_set_priority(u32 thread_id, s32 prio)
{
	sys_ppu_thread.Log("sys_ppu_thread_set_priority(thread_id=0x%x, prio=%d)", thread_id, prio);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<PPUThread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (prio < 0 || prio > 3071)
	{
		return CELL_EINVAL;
	}

	thread->prio = prio;

	return CELL_OK;
}

s32 sys_ppu_thread_get_priority(u32 thread_id, vm::ptr<s32> priop)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_priority(thread_id=0x%x, priop=*0x%x)", thread_id, priop);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<PPUThread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	*priop = thread->prio;

	return CELL_OK;
}

s32 sys_ppu_thread_get_stack_information(PPUThread& ppu, vm::ptr<sys_ppu_thread_stack_t> sp)
{
	sys_ppu_thread.Log("sys_ppu_thread_get_stack_information(sp=*0x%x)", sp);

	sp->pst_addr = ppu.stack_addr;
	sp->pst_size = ppu.stack_size;

	return CELL_OK;
}

s32 sys_ppu_thread_stop(u32 thread_id)
{
	sys_ppu_thread.Todo("sys_ppu_thread_stop(thread_id=0x%x)", thread_id);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<PPUThread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	//t->Stop();

	return CELL_OK;
}

s32 sys_ppu_thread_restart(u32 thread_id)
{
	sys_ppu_thread.Todo("sys_ppu_thread_restart(thread_id=0x%x)", thread_id);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<PPUThread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	//t->Stop();
	//t->Run();

	return CELL_OK;
}

u32 ppu_thread_create(u32 entry, u64 arg, s32 prio, u32 stacksize, bool is_joinable, bool is_interrupt, std::string name, std::function<void(PPUThread&)> task)
{
	const auto ppu = Emu.GetIdManager().make_ptr<PPUThread>(name);

	ppu->prio = prio;
	ppu->stack_size = stacksize < 0x4000 ? 0x4000 : stacksize; // (hack) adjust minimal stack size
	ppu->custom_task = task;
	ppu->Run();

	if (entry)
	{
		ppu->PC = vm::read32(entry);
		ppu->GPR[2] = vm::read32(entry + 4); // rtoc
	}

	if (!is_interrupt)
	{
		ppu->GPR[3] = arg;
		ppu->Exec();
	}

	return ppu->GetId();
}

s32 _sys_ppu_thread_create(vm::ptr<u64> thread_id, vm::ptr<ppu_thread_param_t> param, u64 arg, u64 unk, s32 prio, u32 stacksize, u64 flags, vm::cptr<char> threadname)
{
	sys_ppu_thread.Warning("_sys_ppu_thread_create(thread_id=*0x%x, param=*0x%x, arg=0x%llx, unk=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname=*0x%x)",
		thread_id, param, arg, unk, prio, stacksize, flags, threadname);

	LV2_LOCK;

	if (prio < 0 || prio > 3071)
	{
		return CELL_EINVAL;
	}

	const bool is_joinable = flags & SYS_PPU_THREAD_CREATE_JOINABLE;
	const bool is_interrupt = flags & SYS_PPU_THREAD_CREATE_INTERRUPT;

	if (is_joinable && is_interrupt)
	{
		return CELL_EPERM;
	}

	const auto ppu = Emu.GetIdManager().make_ptr<PPUThread>(threadname ? threadname.get_ptr() : "");

	ppu->prio = prio;
	ppu->stack_size = stacksize < 0x4000 ? 0x4000 : stacksize; // (hack) adjust minimal stack size
	ppu->Run();

	ppu->PC = vm::read32(param->entry);
	ppu->GPR[2] = vm::read32(param->entry + 4); // rtoc
	ppu->GPR[3] = arg;
	ppu->GPR[4] = unk; // actually unknown

	ppu->is_joinable = is_joinable;

	if (u32 tls = param->tls) // hack
	{
		ppu->GPR[13] = tls;
	}

	*thread_id = ppu->GetId();

	return CELL_OK;
}

s32 sys_ppu_thread_start(u32 thread_id)
{
	sys_ppu_thread.Warning("sys_ppu_thread_start(thread_id=0x%x)", thread_id);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<PPUThread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	thread->Exec();

	return CELL_OK;
}

s32 sys_ppu_thread_rename(u32 thread_id, vm::cptr<char> name)
{
	sys_ppu_thread.Todo("sys_ppu_thread_rename(thread_id=0x%x, name=*0x%x)", thread_id, name);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<PPUThread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}
