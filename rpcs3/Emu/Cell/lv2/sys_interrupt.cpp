#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUOpcodes.h"
#include "sys_interrupt.h"

LOG_CHANNEL(sys_interrupt);

void lv2_int_serv::exec()
{
	thread->cmd_list
	({
		{ ppu_cmd::reset_stack, 0 },
		{ ppu_cmd::set_args, 2 }, arg1, arg2,
		{ ppu_cmd::lle_call, 2 },
		{ ppu_cmd::sleep, 0 }
	});

	thread_ctrl::notify(*thread);
}

void lv2_int_serv::join()
{
	// Enqueue _sys_ppu_thread_exit call
	thread->cmd_list
	({
		{ ppu_cmd::set_args, 1 }, u64{0},
		{ ppu_cmd::set_gpr, 11 }, u64{41},
		{ ppu_cmd::opcode, ppu_instructions::SC(0) },
	});

	thread_ctrl::notify(*thread);
	(*thread)();
}

error_code sys_interrupt_tag_destroy(u32 intrtag)
{
	sys_interrupt.warning("sys_interrupt_tag_destroy(intrtag=0x%x)", intrtag);

	const auto tag = idm::withdraw<lv2_obj, lv2_int_tag>(intrtag, [](lv2_int_tag& tag) -> CellError
	{
		if (!tag.handler.expired())
		{
			return CELL_EBUSY;
		}

		return {};
	});

	if (!tag)
	{
		return CELL_ESRCH;
	}

	if (tag.ret)
	{
		return tag.ret;
	}

	return CELL_OK;
}

error_code _sys_interrupt_thread_establish(vm::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg1, u64 arg2)
{
	sys_interrupt.warning("_sys_interrupt_thread_establish(ih=*0x%x, intrtag=0x%x, intrthread=0x%x, arg1=0x%llx, arg2=0x%llx)", ih, intrtag, intrthread, arg1, arg2);

	CellError error = CELL_EAGAIN;

	const u32 id = idm::import<lv2_obj, lv2_int_serv>([&]()
	{
		std::shared_ptr<lv2_int_serv> result;

		// Get interrupt tag
		const auto tag = idm::check_unlocked<lv2_obj, lv2_int_tag>(intrtag);

		if (!tag)
		{
			error = CELL_ESRCH;
			return result;
		}

		// Get interrupt thread
		const auto it = idm::get_unlocked<named_thread<ppu_thread>>(intrthread);

		if (!it)
		{
			error = CELL_ESRCH;
			return result;
		}

		// If interrupt thread is running, it's already established on another interrupt tag
		if (!(it->state & cpu_flag::stop))
		{
			error = CELL_EAGAIN;
			return result;
		}

		// It's unclear if multiple handlers can be established on single interrupt tag
		if (!tag->handler.expired())
		{
			error = CELL_ESTAT;
			return result;
		}

		result = std::make_shared<lv2_int_serv>(it, arg1, arg2);
		tag->handler = result;
		it->state -= cpu_flag::stop;
		thread_ctrl::notify(*it);
		return result;
	});

	if (id)
	{
		*ih = id;
		return CELL_OK;
	}

	return error;
}

error_code _sys_interrupt_thread_disestablish(ppu_thread& ppu, u32 ih, vm::ptr<u64> r13)
{
	sys_interrupt.warning("_sys_interrupt_thread_disestablish(ih=0x%x, r13=*0x%x)", ih, r13);

	const auto handler = idm::withdraw<lv2_obj, lv2_int_serv>(ih);

	if (!handler)
	{
		return CELL_ESRCH;
	}

	// Wait for sys_interrupt_thread_eoi() and destroy interrupt thread
	handler->join();

	// Save TLS base
	*r13 = handler->thread->gpr[13];

	return CELL_OK;
}

void sys_interrupt_thread_eoi(ppu_thread& ppu)
{
	vm::temporary_unlock(ppu);

	sys_interrupt.trace("sys_interrupt_thread_eoi()");

	ppu.state += cpu_flag::ret;
}
