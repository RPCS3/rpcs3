#include "stdafx.h"
#include "sys_ppu_thread.h"

#include "Emu/IdManager.h"
#include "Emu/perf_meter.hpp"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Memory/vm_locking.h"
#include "sys_event.h"
#include "sys_process.h"
#include "sys_mmapper.h"
#include "sys_memory.h"

#include "util/asm.hpp"

LOG_CHANNEL(sys_ppu_thread);

// Simple structure to cleanup previous thread, because can't remove its own thread
struct ppu_thread_cleaner
{
	atomic_t<u32> old_id = 0;

	void clean(u32 new_id)
	{
		if (old_id || new_id) [[likely]]
		{
			if (u32 id = old_id.exchange(new_id)) [[likely]]
			{
				if (!idm::remove<named_thread<ppu_thread>>(id)) [[unlikely]]
				{
					sys_ppu_thread.fatal("Failed to remove detached thread 0x%x", id);
				}
			}
		}
	}
};

bool ppu_thread_exit(ppu_thread& ppu)
{
	ppu.state += cpu_flag::exit + cpu_flag::wait;
	
	// Deallocate Stack Area
	ensure(vm::dealloc(ppu.stack_addr, vm::stack) == ppu.stack_size);

	if (const auto dct = g_fxo->get<lv2_memory_container>())
	{
		dct->used -= ppu.stack_size;
	}

	perf_log.notice("Perf stats for STCX reload: successs %u, failure %u", ppu.last_succ, ppu.last_fail);
	return false;
}

void _sys_ppu_thread_exit(ppu_thread& ppu, u64 errorcode)
{
	ppu.state += cpu_flag::wait;

	// Need to wait until the current writer finish
	if (ppu.state & cpu_flag::memory) vm::g_mutex.lock_unlock();

	sys_ppu_thread.trace("_sys_ppu_thread_exit(errorcode=0x%llx)", errorcode);

	ppu.state += cpu_flag::exit;

	ppu_join_status old_status;
	{
		std::lock_guard lock(id_manager::g_mutex);

		// Get joiner ID
		old_status = ppu.joiner.fetch_op([](ppu_join_status& status)
		{
			if (status == ppu_join_status::joinable)
			{
				// Joinable, not joined
				status = ppu_join_status::zombie;
				return;
			}

			// Set deleted thread status
			status = ppu_join_status::exited;
		});

		if (old_status >= ppu_join_status::max)
		{
			lv2_obj::append(idm::check_unlocked<named_thread<ppu_thread>>(static_cast<u32>(old_status)));
		}

		// Unqueue
		lv2_obj::sleep(ppu);

		// Remove suspend state (TODO)
		ppu.state -= cpu_flag::suspend;
	}

	g_fxo->get<ppu_thread_cleaner>()->clean(old_status == ppu_join_status::detached ? ppu.id : 0);

	if (old_status == ppu_join_status::joinable)
	{
		// Wait for termination
		ppu.joiner.wait(ppu_join_status::zombie);
	}

	ppu_thread_exit(ppu);
}

s32 sys_ppu_thread_yield(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.trace("sys_ppu_thread_yield()");

	// Return 0 on successful context switch, 1 otherwise
	return +!lv2_obj::yield(ppu);
}

error_code sys_ppu_thread_join(ppu_thread& ppu, u32 thread_id, vm::ptr<u64> vptr)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.trace("sys_ppu_thread_join(thread_id=0x%x, vptr=*0x%x)", thread_id, vptr);

	// Clean some detached thread (hack)
	g_fxo->get<ppu_thread_cleaner>()->clean(0);

	auto thread = idm::get<named_thread<ppu_thread>>(thread_id, [&](ppu_thread& thread) -> CellError
	{
		if (&ppu == &thread)
		{
			return CELL_EDEADLK;
		}

		CellError result = thread.joiner.atomic_op([&](ppu_join_status& value) -> CellError
		{
			if (value == ppu_join_status::zombie)
			{
				value = ppu_join_status::exited;
				return CELL_EAGAIN;
			}

			if (value == ppu_join_status::exited)
			{
				return CELL_ESRCH;
			}

			if (value >= ppu_join_status::max)
			{
				return CELL_EINVAL;
			}

			value = ppu_join_status{ppu.id};
			return {};
		});

		if (!result)
		{
			lv2_obj::sleep(ppu);
		}
		else if (result == CELL_EAGAIN)
		{
			thread.joiner.notify_one();
		}

		return result;
	});

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (thread.ret && thread.ret != CELL_EAGAIN)
	{
		return thread.ret;
	}

	// Wait for cleanup
	(*thread.ptr)();

	if (ppu.test_stopped())
	{
		return 0;
	}

	// Get the exit status from the register
	const u64 vret = thread->gpr[3];

	// Cleanup
	ensure(idm::remove_verify<named_thread<ppu_thread>>(thread_id, std::move(thread.ptr)));

	if (!vptr)
	{
		return not_an_error(CELL_EFAULT);
	}

	*vptr = vret;
	return CELL_OK;
}

error_code sys_ppu_thread_detach(ppu_thread& ppu, u32 thread_id)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.trace("sys_ppu_thread_detach(thread_id=0x%x)", thread_id);

	// Clean some detached thread (hack)
	g_fxo->get<ppu_thread_cleaner>()->clean(0);

	const auto thread = idm::check<named_thread<ppu_thread>>(thread_id, [&](ppu_thread& thread) -> CellError
	{
		CellError result = thread.joiner.atomic_op([](ppu_join_status& value) -> CellError
		{
			if (value == ppu_join_status::zombie)
			{
				value = ppu_join_status::exited;
				return CELL_EAGAIN;
			}

			if (value == ppu_join_status::exited)
			{
				return CELL_ESRCH;
			}

			if (value == ppu_join_status::detached)
			{
				return CELL_EINVAL;
			}

			if (value >= ppu_join_status::max)
			{
				return CELL_EBUSY;
			}

			value = ppu_join_status::detached;
			return {};
		});

		if (result == CELL_EAGAIN)
		{
			thread.joiner.notify_one();
		}

		return result;
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
		g_fxo->get<ppu_thread_cleaner>()->clean(thread_id);
		g_fxo->get<ppu_thread_cleaner>()->clean(0);
	}

	return CELL_OK;
}

error_code sys_ppu_thread_get_join_state(ppu_thread& ppu, vm::ptr<s32> isjoinable)
{
	sys_ppu_thread.trace("sys_ppu_thread_get_join_state(isjoinable=*0x%x)", isjoinable);

	if (!isjoinable)
	{
		return CELL_EFAULT;
	}

	*isjoinable = ppu.joiner != ppu_join_status::detached;
	return CELL_OK;
}

error_code sys_ppu_thread_set_priority(ppu_thread& ppu, u32 thread_id, s32 prio)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.trace("sys_ppu_thread_set_priority(thread_id=0x%x, prio=%d)", thread_id, prio);

	if (prio < (g_ps3_process_info.debug_or_root() ? -512 : 0) || prio > 3071)
	{
		return CELL_EINVAL;
	}

	// Clean some detached thread (hack)
	g_fxo->get<ppu_thread_cleaner>()->clean(0);

	const auto thread = idm::check<named_thread<ppu_thread>>(thread_id, [&](ppu_thread& thread)
	{
		if (thread.joiner == ppu_join_status::exited)
		{
			return false;
		}

		if (thread.prio != prio)
		{
			lv2_obj::set_priority(thread, prio);
		}

		return true;
	});

	if (!thread || !thread.ret)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_ppu_thread_get_priority(ppu_thread& ppu, u32 thread_id, vm::ptr<s32> priop)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.trace("sys_ppu_thread_get_priority(thread_id=0x%x, priop=*0x%x)", thread_id, priop);

	// Clean some detached thread (hack)
	g_fxo->get<ppu_thread_cleaner>()->clean(0);

	u32 prio;

	const auto thread = idm::check<named_thread<ppu_thread>>(thread_id, [&](ppu_thread& thread)
	{
		if (thread.joiner == ppu_join_status::exited)
		{
			return false;
		}

		prio = thread.prio;
		return true;
	});

	if (!thread || !thread.ret)
	{
		return CELL_ESRCH;
	}

	*priop = prio;
	return CELL_OK;
}

error_code sys_ppu_thread_get_stack_information(ppu_thread& ppu, vm::ptr<sys_ppu_thread_stack_t> sp)
{
	sys_ppu_thread.trace("sys_ppu_thread_get_stack_information(sp=*0x%x)", sp);

	sp->pst_addr = ppu.stack_addr;
	sp->pst_size = ppu.stack_size;

	return CELL_OK;
}

error_code sys_ppu_thread_stop(ppu_thread& ppu, u32 thread_id)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.todo("sys_ppu_thread_stop(thread_id=0x%x)", thread_id);

	if (!g_ps3_process_info.has_root_perm())
	{
		return CELL_ENOSYS;
	}

	const auto thread = idm::check<named_thread<ppu_thread>>(thread_id, [&](ppu_thread& thread)
	{
		return thread.joiner != ppu_join_status::exited;
	});

	if (!thread || !thread.ret)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_ppu_thread_restart(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.todo("sys_ppu_thread_restart()");

	if (!g_ps3_process_info.has_root_perm())
	{
		return CELL_ENOSYS;
	}

	return CELL_OK;
}

error_code _sys_ppu_thread_create(ppu_thread& ppu, vm::ptr<u64> thread_id, vm::ptr<ppu_thread_param_t> param, u64 arg, u64 unk, s32 prio, u32 _stacksz, u64 flags, vm::cptr<char> threadname)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.warning("_sys_ppu_thread_create(thread_id=*0x%x, param=*0x%x, arg=0x%llx, unk=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname=*0x%x)",
		thread_id, param, arg, unk, prio, _stacksz, flags, threadname);

	// thread_id is checked for null in stub -> CELL_ENOMEM
	// unk is set to 0 in sys_ppu_thread_create stub

	if (!param || !param->entry)
	{
		return CELL_EFAULT;
	}

	if (prio < (g_ps3_process_info.debug_or_root() ? -512 : 0) || prio > 3071)
	{
		return CELL_EINVAL;
	}

	if ((flags & 3) == 3) // Check two flags: joinable + interrupt not allowed
	{
		return CELL_EPERM;
	}

	const ppu_func_opd_t entry = param->entry.opd();
	const u32 tls = param->tls;

	// Clean some detached thread (hack)
	g_fxo->get<ppu_thread_cleaner>()->clean(0);

	// Compute actual stack size and allocate
	const u32 stack_size = utils::align<u32>(std::max<u32>(_stacksz, 4096), 4096);

	const auto dct = g_fxo->get<lv2_memory_container>();

	// Try to obtain "physical memory" from the default container
	if (!dct->take(stack_size))
	{
		return CELL_ENOMEM;
	}

	const vm::addr_t stack_base{vm::alloc(stack_size, vm::stack, 4096)};

	if (!stack_base)
	{
		dct->used -= stack_size;
		return CELL_ENOMEM;
	}

	std::string ppu_name;

	if (threadname)
	{
		constexpr u32 max_size = 27; // max size including null terminator
		const auto pname = threadname.get_ptr();
		ppu_name.assign(pname, std::find(pname, pname + max_size, '\0'));
	}

	const u32 tid = idm::import<named_thread<ppu_thread>>([&]()
	{
		const u32 tid = idm::last_id();

		std::string full_name = fmt::format("PPU[0x%x] ", tid);

		if (threadname)
		{
			if (!ppu_name.empty())
			{
				fmt::append(full_name, "%s ", ppu_name);
			}
		}

		ppu_thread_params p;
		p.stack_addr = stack_base;
		p.stack_size = stack_size;
		p.tls_addr = tls;
		p.entry = entry;
		p.arg0 = arg;
		p.arg1 = unk;

		return std::make_shared<named_thread<ppu_thread>>(full_name, p, ppu_name, prio, 1 - static_cast<int>(flags & 3));
	});

	if (!tid)
	{
		vm::dealloc(stack_base);
		dct->used -= stack_size;
		return CELL_EAGAIN;
	}

	*thread_id = tid;
	sys_ppu_thread.warning(u8"_sys_ppu_thread_create(): Thread “%s” created (id=0x%x, func=*0x%x, rtoc=0x%x, user-tls=0x%x)", ppu_name, tid, entry.addr, entry.rtoc, tls);
	return CELL_OK;
}

error_code sys_ppu_thread_start(ppu_thread& ppu, u32 thread_id)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.trace("sys_ppu_thread_start(thread_id=0x%x)", thread_id);

	const auto thread = idm::get<named_thread<ppu_thread>>(thread_id, [&](ppu_thread& thread) -> CellError
	{
		if (thread.joiner == ppu_join_status::exited)
		{
			return CELL_ESRCH;
		}

		if (!thread.state.test_and_reset(cpu_flag::stop))
		{
			// Already started
			return CELL_EBUSY;
		}

		lv2_obj::awake(&thread);

		thread.cmd_list
		({
			{ppu_cmd::opd_call, 0}, thread.entry_func
		});

		return {};
	});

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (thread.ret)
	{
		return thread.ret;
	}
	else
	{
		thread_ctrl::notify(*thread);

		// Dirty hack for sound: confirm the creation of _mxr000 event queue
		if (*thread->ppu_tname.load() == "_cellsurMixerMain"sv)
		{
			lv2_obj::sleep(ppu);

			while (!idm::select<lv2_obj, lv2_event_queue>([](u32, lv2_event_queue& eq)
			{
				//some games do not set event queue name, though key seems constant for them
				return (eq.name == "_mxr000\0"_u64) || (eq.key == 0x8000cafe02460300);
			}))
			{
				if (ppu.is_stopped())
				{
					return 0;
				}

				thread_ctrl::wait_for(50000);
			}

			if (ppu.test_stopped())
			{
				return 0;
			}
		}
	}

	return CELL_OK;
}

error_code sys_ppu_thread_rename(ppu_thread& ppu, u32 thread_id, vm::cptr<char> name)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.warning("sys_ppu_thread_rename(thread_id=0x%x, name=*0x%x)", thread_id, name);

	const auto thread = idm::get<named_thread<ppu_thread>>(thread_id, [&](ppu_thread& thread)
	{
		return thread.joiner != ppu_join_status::exited;
	});

	if (!thread || !thread.ret)
	{
		return CELL_ESRCH;
	}

	if (!name)
	{
		return CELL_EFAULT;
	}

	constexpr u32 max_size = 27; // max size including null terminator
	const auto pname = name.get_ptr();

	// Make valid name
	auto _name = make_single<std::string>(pname, std::find(pname, pname + max_size, '\0'));

	// thread_ctrl name is not changed (TODO)
	sys_ppu_thread.warning(u8"sys_ppu_thread_rename(): Thread renamed to “%s”", *_name);
	thread->ppu_tname.store(std::move(_name));
	return CELL_OK;
}

error_code sys_ppu_thread_recover_page_fault(ppu_thread& ppu, u32 thread_id)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.warning("sys_ppu_thread_recover_page_fault(thread_id=0x%x)", thread_id);

	const auto thread = idm::get<named_thread<ppu_thread>>(thread_id, [&](ppu_thread& thread)
	{
		return thread.joiner != ppu_join_status::exited;
	});

	if (!thread || !thread.ret)
	{
		return CELL_ESRCH;
	}

	return mmapper_thread_recover_page_fault(thread.ptr.get());
}

error_code sys_ppu_thread_get_page_fault_context(ppu_thread& ppu, u32 thread_id, vm::ptr<sys_ppu_thread_icontext_t> ctxt)
{
	ppu.state += cpu_flag::wait;

	sys_ppu_thread.todo("sys_ppu_thread_get_page_fault_context(thread_id=0x%x, ctxt=*0x%x)", thread_id, ctxt);

	const auto thread = idm::get<named_thread<ppu_thread>>(thread_id, [&](ppu_thread& thread)
	{
		return thread.joiner != ppu_join_status::exited;
	});

	if (!thread || !thread.ret)
	{
		return CELL_ESRCH;
	}

	// We can only get a context if the thread is being suspended for a page fault.
	auto pf_events = g_fxo->get<page_fault_event_entries>();
	reader_lock lock(pf_events->pf_mutex);

	const auto evt = pf_events->events.find(thread.ptr.get());
	if (evt == pf_events->events.end())
	{
		return CELL_EINVAL;
	}

	// TODO: Fill ctxt with proper information.

	return CELL_OK;
}
