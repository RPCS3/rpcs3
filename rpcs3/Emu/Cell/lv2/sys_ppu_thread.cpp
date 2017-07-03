#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_ppu_thread.h"
#include "sys_event.h"

namespace vm { using namespace ps3; }

logs::channel sys_ppu_thread("sys_ppu_thread");

void _sys_ppu_thread_exit(ppu_thread& ppu, u64 errorcode)
{
	vm::temporary_unlock(ppu);

	sys_ppu_thread.trace("_sys_ppu_thread_exit(errorcode=0x%llx)", errorcode);

	ppu.state += cpu_flag::exit;

	// Get joiner ID
	const u32 jid = ppu.joiner.fetch_op([](u32& value)
	{
		if (value == 0)
		{
			// Joinable, not joined
			value = -3;
		}
		else if (value != -1)
		{
			// Joinable, joined
			value = -2;
		}

		// Detached otherwise
	});

	if (jid == -1)
	{
		// Delete detached thread and unqueue
		idm::remove<ppu_thread>(ppu.id);
	}
	else if (jid != 0)
	{
		writer_lock lock(id_manager::g_mutex);

		// Schedule joiner and unqueue
		lv2_obj::awake(*idm::check_unlocked<ppu_thread>(jid), -2);
	}

	// Unqueue
	lv2_obj::sleep(ppu);

	// Remove suspend state (TODO)
	ppu.state -= cpu_flag::suspend;
}

void sys_ppu_thread_yield(ppu_thread& ppu)
{
	sys_ppu_thread.trace("sys_ppu_thread_yield()");

	lv2_obj::awake(ppu, -4);
}

error_code sys_ppu_thread_join(ppu_thread& ppu, u32 thread_id, vm::ptr<u64> vptr)
{
	vm::temporary_unlock(ppu);

	sys_ppu_thread.trace("sys_ppu_thread_join(thread_id=0x%x, vptr=*0x%x)", thread_id, vptr);

	const auto thread = idm::get<ppu_thread>(thread_id, [&](ppu_thread& thread) -> CellError
	{
		CellError result = thread.joiner.atomic_op([&](u32& value) -> CellError
		{
			if (value == -3)
			{
				value = -2;
				return CELL_EBUSY;
			}

			if (value == -2)
			{
				return CELL_ESRCH;
			}

			if (value)
			{
				return CELL_EINVAL;
			}

			// TODO: check precedence?
			if (&ppu == &thread)
			{
				return CELL_EDEADLK;
			}

			value = ppu.id;
			return {};
		});

		if (!result)
		{
			lv2_obj::sleep(ppu);
		}
		
		return result;
	});

	if (!thread)
	{
		return CELL_ESRCH;
	}
	
	if (thread.ret && thread.ret != CELL_EBUSY)
	{
		return thread.ret;
	}

	// Wait for cleanup
	thread->join();

	// Get the exit status from the register
	if (vptr)
	{
		ppu.test_state();
		*vptr = thread->gpr[3];
	}

	// Cleanup
	idm::remove<ppu_thread>(thread->id);
	return CELL_OK;
}

error_code sys_ppu_thread_detach(u32 thread_id)
{
	sys_ppu_thread.trace("sys_ppu_thread_detach(thread_id=0x%x)", thread_id);

	const auto thread = idm::check<ppu_thread>(thread_id, [&](ppu_thread& thread) -> CellError
	{
		return thread.joiner.atomic_op([&](u32& value) -> CellError
		{
			if (value == -3)
			{
				value = -2;
				return CELL_EAGAIN;
			}

			if (value == -2)
			{
				return CELL_ESRCH;
			}

			if (value == -1)
			{
				return CELL_EINVAL;
			}

			if (value)
			{
				return CELL_EBUSY;
			}

			value = -1;
			return {};
		});
	});

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (thread.ret && thread.ret != CELL_EAGAIN)
	{
		return thread.ret;
	}

	if (thread.ret == CELL_EAGAIN)
	{
		idm::remove<ppu_thread>(thread_id);
	}
	
	return CELL_OK;
}

void sys_ppu_thread_get_join_state(ppu_thread& ppu, vm::ptr<s32> isjoinable)
{
	sys_ppu_thread.trace("sys_ppu_thread_get_join_state(isjoinable=*0x%x)", isjoinable);

	*isjoinable = ppu.joiner != -1;
}

error_code sys_ppu_thread_set_priority(ppu_thread& ppu, u32 thread_id, s32 prio)
{
	sys_ppu_thread.trace("sys_ppu_thread_set_priority(thread_id=0x%x, prio=%d)", thread_id, prio);

	if (prio < 0 || prio > 3071)
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::check<ppu_thread>(thread_id, [&](ppu_thread& thread)
	{
		if (thread.prio != prio && thread.prio.exchange(prio) != prio)
		{
			lv2_obj::awake(thread, prio);
		}
	});

	if (!thread)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_ppu_thread_get_priority(u32 thread_id, vm::ptr<s32> priop)
{
	sys_ppu_thread.trace("sys_ppu_thread_get_priority(thread_id=0x%x, priop=*0x%x)", thread_id, priop);

	const auto thread = idm::check<ppu_thread>(thread_id, [&](ppu_thread& thread)
	{
		*priop = thread.prio;
	});

	if (!thread)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_ppu_thread_get_stack_information(ppu_thread& ppu, vm::ptr<sys_ppu_thread_stack_t> sp)
{
	sys_ppu_thread.trace("sys_ppu_thread_get_stack_information(sp=*0x%x)", sp);

	sp->pst_addr = ppu.stack_addr;
	sp->pst_size = ppu.stack_size;

	return CELL_OK;
}

error_code sys_ppu_thread_stop(u32 thread_id)
{
	sys_ppu_thread.todo("sys_ppu_thread_stop(thread_id=0x%x)", thread_id);

	const auto thread = idm::get<ppu_thread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_ppu_thread_restart(u32 thread_id)
{
	sys_ppu_thread.todo("sys_ppu_thread_restart(thread_id=0x%x)", thread_id);

	const auto thread = idm::get<ppu_thread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code _sys_ppu_thread_create(vm::ptr<u64> thread_id, vm::ptr<ppu_thread_param_t> param, u64 arg, u64 unk, s32 prio, u32 stacksize, u64 flags, vm::cptr<char> threadname)
{
	sys_ppu_thread.warning("_sys_ppu_thread_create(thread_id=*0x%x, param=*0x%x, arg=0x%llx, unk=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname=%s)",
		thread_id, param, arg, unk, prio, stacksize, flags, threadname);

	if (prio < 0 || prio > 3071)
	{
		return CELL_EINVAL;
	}

	if ((flags & 3) == 3) // Check two flags: joinable + interrupt not allowed
	{
		return CELL_EPERM;
	}

	const u32 tid = idm::import<ppu_thread>([&]()
	{
		auto ppu = std::make_shared<ppu_thread>(threadname ? threadname.get_ptr() : "", prio, stacksize);

		if ((flags & SYS_PPU_THREAD_CREATE_JOINABLE) != 0)
		{
			ppu->joiner = 0;
		}

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
			// Save entry for further use (workaround)
			ppu->gpr[2] = param->entry.value();
		}

		return ppu;
	});

	if (!tid)
	{
		return CELL_EAGAIN;
	}

	*thread_id = tid;
	return CELL_OK;
}

error_code sys_ppu_thread_start(ppu_thread& ppu, u32 thread_id)
{
	sys_ppu_thread.trace("sys_ppu_thread_start(thread_id=0x%x)", thread_id);

	const auto thread = idm::get<ppu_thread>(thread_id, [&](ppu_thread& thread)
	{
		lv2_obj::awake(thread, -2);
	});

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (!thread->state.test_and_reset(cpu_flag::stop))
	{
		// TODO: what happens there?
		return CELL_EPERM;
	}
	else
	{
		thread->notify();

		// Dirty hack for sound: confirm the creation of _mxr000 event queue
		if (thread->m_name == "_cellsurMixerMain")
		{
			lv2_obj::sleep(ppu);

			while (!idm::select<lv2_obj, lv2_event_queue>([](u32, lv2_event_queue& eq)
			{
				return eq.name == "_mxr000\0"_u64;
			}))
			{
				thread_ctrl::wait_for(50000);
			}

			ppu.test_state();
		}
	}

	return CELL_OK;
}

error_code sys_ppu_thread_rename(u32 thread_id, vm::cptr<char> name)
{
	sys_ppu_thread.todo("sys_ppu_thread_rename(thread_id=0x%x, name=%s)", thread_id, name);

	const auto thread = idm::get<ppu_thread>(thread_id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}
