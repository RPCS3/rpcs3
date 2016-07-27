#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_ppu_thread.h"

#include <thread>

logs::channel sys_ppu_thread("sys_ppu_thread", logs::level::notice);

void _sys_ppu_thread_exit(ppu_thread& ppu, u64 errorcode)
{
	sys_ppu_thread.warning("_sys_ppu_thread_exit(errorcode=0x%llx)", errorcode);

	// TODO: shall sys_mutex objects be unlocked?

	LV2_LOCK;

	ppu.state += cpu_state::exit;

	// Delete detached thread
	if (!ppu.is_joinable)
	{
		idm::remove<ppu_thread>(ppu.id);
	}

	// Throw if this syscall was not called directly by the SC instruction (hack)
	if (ppu.lr == 0 || ppu.gpr[11] != 41)
	{
		throw cpu_state::exit;
	}
}

void sys_ppu_thread_yield()
{
	sys_ppu_thread.trace("sys_ppu_thread_yield()");

	std::this_thread::yield();
}

s32 sys_ppu_thread_join(ppu_thread& ppu, u32 thread_id, vm::ptr<u64> vptr)
{
	sys_ppu_thread.warning("sys_ppu_thread_join(thread_id=0x%x, vptr=*0x%x)", thread_id, vptr);

	LV2_LOCK;

	const auto thread = idm::get<ppu_thread>(thread_id);

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
	while (!(thread->state & cpu_state::exit))
	{
		CHECK_EMU_STATUS;

		get_current_thread_cv().wait_for(lv2_lock, 1ms);
	}

	// get exit status from the register
	if (vptr) *vptr = thread->gpr[3];

	// cleanup
	idm::remove<ppu_thread>(thread->id);

	return CELL_OK;
}

s32 sys_ppu_thread_detach(u32 thread_id)
{
	sys_ppu_thread.warning("sys_ppu_thread_detach(thread_id=0x%x)", thread_id);

	LV2_LOCK;

	const auto thread = idm::get<ppu_thread>(thread_id);

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

void sys_ppu_thread_get_join_state(ppu_thread& ppu, vm::ptr<s32> isjoinable)
{
	sys_ppu_thread.warning("sys_ppu_thread_get_join_state(isjoinable=*0x%x)", isjoinable);

	LV2_LOCK;

	*isjoinable = ppu.is_joinable;
}

s32 sys_ppu_thread_set_priority(u32 thread_id, s32 prio)
{
	sys_ppu_thread.trace("sys_ppu_thread_set_priority(thread_id=0x%x, prio=%d)", thread_id, prio);

	LV2_LOCK;

	const auto thread = idm::get<ppu_thread>(thread_id);

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
	sys_ppu_thread.trace("sys_ppu_thread_get_priority(thread_id=0x%x, priop=*0x%x)", thread_id, priop);

	LV2_LOCK;

	const auto thread = idm::get<ppu_thread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	*priop = thread->prio;

	return CELL_OK;
}

s32 sys_ppu_thread_get_stack_information(ppu_thread& ppu, vm::ptr<sys_ppu_thread_stack_t> sp)
{
	sys_ppu_thread.trace("sys_ppu_thread_get_stack_information(sp=*0x%x)", sp);

	sp->pst_addr = ppu.stack_addr;
	sp->pst_size = ppu.stack_size;

	return CELL_OK;
}

s32 sys_ppu_thread_stop(u32 thread_id)
{
	sys_ppu_thread.todo("sys_ppu_thread_stop(thread_id=0x%x)", thread_id);

	LV2_LOCK;

	const auto thread = idm::get<ppu_thread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	//t->Stop();

	return CELL_OK;
}

s32 sys_ppu_thread_restart(u32 thread_id)
{
	sys_ppu_thread.todo("sys_ppu_thread_restart(thread_id=0x%x)", thread_id);

	LV2_LOCK;

	const auto thread = idm::get<ppu_thread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	//t->Stop();
	//t->Run();

	return CELL_OK;
}

s32 _sys_ppu_thread_create(vm::ptr<u64> thread_id, vm::ptr<ppu_thread_param_t> param, u64 arg, u64 unk, s32 prio, u32 stacksize, u64 flags, vm::cptr<char> threadname)
{
	sys_ppu_thread.warning("_sys_ppu_thread_create(thread_id=*0x%x, param=*0x%x, arg=0x%llx, unk=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname=*0x%x)",
		thread_id, param, arg, unk, prio, stacksize, flags, threadname);

	LV2_LOCK;

	if (prio < 0 || prio > 3071)
	{
		return CELL_EINVAL;
	}

	if ((flags & 3) == 3) // Check two flags: joinable + interrupt not allowed
	{
		return CELL_EPERM;
	}

	const auto ppu = idm::make_ptr<ppu_thread>(threadname ? threadname.get_ptr() : "", prio, stacksize);

	ppu->is_joinable = (flags & SYS_PPU_THREAD_CREATE_JOINABLE) != 0;
	ppu->gpr[13] = param->tls.value();
	
	if ((flags & SYS_PPU_THREAD_CREATE_INTERRUPT) == 0)
	{
		// Initialize thread entry point
		ppu->cmd_list
		({
			{ ppu_cmd::set_args, 2 }, arg, unk, // Actually unknown
			{ ppu_cmd::lle_call, param->entry.value() },
		});
	}
	else
	{
		// Save entry for further use
		ppu->gpr[2] = param->entry.value();
	}

	*thread_id = ppu->id;

	return CELL_OK;
}

s32 sys_ppu_thread_start(u32 thread_id)
{
	sys_ppu_thread.warning("sys_ppu_thread_start(thread_id=0x%x)", thread_id);

	LV2_LOCK;

	const auto thread = idm::get<ppu_thread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	thread->run();

	return CELL_OK;
}

s32 sys_ppu_thread_rename(u32 thread_id, vm::cptr<char> name)
{
	sys_ppu_thread.todo("sys_ppu_thread_rename(thread_id=0x%x, name=*0x%x)", thread_id, name);

	LV2_LOCK;

	const auto thread = idm::get<ppu_thread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}
