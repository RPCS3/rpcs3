#include "stdafx.h"
#include "sys_interrupt.h"

#include "Emu/IdManager.h"
#include "Emu/System.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUOpcodes.h"

LOG_CHANNEL(sys_interrupt);

lv2_int_tag::lv2_int_tag() noexcept
	: lv2_obj(1)
	, id(idm::last_id())
{
}

lv2_int_tag::lv2_int_tag(utils::serial& ar) noexcept
	: lv2_obj(1)
	, id(idm::last_id())
	, handler([&]()
	{
		const u32 id = ar;

		auto ptr = idm::get_unlocked<lv2_obj, lv2_int_serv>(id);

		if (!ptr && id)
		{
			Emu.PostponeInitCode([id, &handler = this->handler]()
			{
				handler = ensure(idm::get_unlocked<lv2_obj, lv2_int_serv>(id));
			});
		}

		return ptr;
	}())
{
}

void lv2_int_tag::save(utils::serial& ar)
{
	ar(lv2_obj::check(handler) ? handler->id : 0);
}

lv2_int_serv::lv2_int_serv(shared_ptr<named_thread<ppu_thread>> thread, u64 arg1, u64 arg2) noexcept
	: lv2_obj(1)
	, id(idm::last_id())
	, thread(thread)
	, arg1(arg1)
	, arg2(arg2)
{
}

lv2_int_serv::lv2_int_serv(utils::serial& ar) noexcept
	: lv2_obj(1)
	, id(idm::last_id())
	, thread(idm::get_unlocked<named_thread<ppu_thread>>(ar))
	, arg1(ar)
	, arg2(ar)
{
}

void lv2_int_serv::save(utils::serial& ar)
{
	ar(thread && idm::check_unlocked<named_thread<ppu_thread>>(thread->id) ? thread->id : 0, arg1, arg2);
}

void ppu_interrupt_thread_entry(ppu_thread&, ppu_opcode_t, be_t<u32>*, struct ppu_intrp_func*);

void lv2_int_serv::exec() const
{
	thread->cmd_list
	({
		{ ppu_cmd::reset_stack, 0 },
		{ ppu_cmd::set_args, 2 }, arg1, arg2,
		{ ppu_cmd::entry_call, 0 },
		{ ppu_cmd::sleep, 0 },
		{ ppu_cmd::ptr_call, 0 },
		std::bit_cast<u64>(&ppu_interrupt_thread_entry)
	});
}

void ppu_thread_exit(ppu_thread&, ppu_opcode_t, be_t<u32>*, struct ppu_intrp_func*);

void lv2_int_serv::join() const
{
	thread->cmd_list
	({
		{ ppu_cmd::ptr_call, 0 },
		std::bit_cast<u64>(&ppu_thread_exit)
	});

	thread->cmd_notify.store(1);
	thread->cmd_notify.notify_one();
	(*thread)();

	idm::remove_verify<named_thread<ppu_thread>>(thread->id, thread);
}

error_code sys_interrupt_tag_destroy(ppu_thread& ppu, u32 intrtag)
{
	ppu.state += cpu_flag::wait;

	sys_interrupt.warning("sys_interrupt_tag_destroy(intrtag=0x%x)", intrtag);

	const auto tag = idm::withdraw<lv2_obj, lv2_int_tag>(intrtag, [](lv2_int_tag& tag) -> CellError
	{
		if (lv2_obj::check(tag.handler))
		{
			return CELL_EBUSY;
		}

		tag.exists.release(0);
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

error_code _sys_interrupt_thread_establish(ppu_thread& ppu, vm::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg1, u64 arg2)
{
	ppu.state += cpu_flag::wait;

	sys_interrupt.warning("_sys_interrupt_thread_establish(ih=*0x%x, intrtag=0x%x, intrthread=0x%x, arg1=0x%llx, arg2=0x%llx)", ih, intrtag, intrthread, arg1, arg2);

	CellError error = CELL_EAGAIN;

	const u32 id = idm::import<lv2_obj, lv2_int_serv>([&]()
	{
		shared_ptr<lv2_int_serv> result;

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
		if (cpu_flag::stop - it->state)
		{
			error = CELL_EAGAIN;
			return result;
		}

		// It's unclear if multiple handlers can be established on single interrupt tag
		if (lv2_obj::check(tag->handler))
		{
			error = CELL_ESTAT;
			return result;
		}

		result = make_shared<lv2_int_serv>(it, arg1, arg2);
		tag->handler = result;

		it->cmd_list
		({
			{ ppu_cmd::ptr_call, 0 },
			std::bit_cast<u64>(&ppu_interrupt_thread_entry)
		});

		it->state -= cpu_flag::stop;
		it->state.notify_one();

		return result;
	});

	if (id)
	{
		ppu.check_state();
		*ih = id;
		return CELL_OK;
	}

	return error;
}

error_code _sys_interrupt_thread_disestablish(ppu_thread& ppu, u32 ih, vm::ptr<u64> r13)
{
	ppu.state += cpu_flag::wait;

	sys_interrupt.warning("_sys_interrupt_thread_disestablish(ih=0x%x, r13=*0x%x)", ih, r13);

	const auto handler = idm::withdraw<lv2_obj, lv2_int_serv>(ih, [](lv2_obj& obj)
	{
		obj.exists.release(0);
	});

	if (!handler)
	{
		if (const auto thread = idm::withdraw<named_thread<ppu_thread>>(ih))
		{
			*r13 = thread->gpr[13];

			// It is detached from IDM now so join must be done explicitly now
			*thread = thread_state::finished;
			return CELL_OK;
		}

		return CELL_ESRCH;
	}

	lv2_obj::sleep(ppu);

	// Wait for sys_interrupt_thread_eoi() and destroy interrupt thread
	handler->join();

	// Save TLS base
	*r13 = handler->thread->gpr[13];

	return CELL_OK;
}

void sys_interrupt_thread_eoi(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_interrupt.trace("sys_interrupt_thread_eoi()");

	ppu.state += cpu_flag::ret;

	lv2_obj::sleep(ppu);

	ppu.interrupt_thread_executing = false;
}

void ppu_interrupt_thread_entry(ppu_thread& ppu, ppu_opcode_t, be_t<u32>*, struct ppu_intrp_func*)
{
	while (true)
	{
		shared_ptr<lv2_int_serv> serv = null_ptr;

		// Loop endlessly trying to invoke an interrupt if required
		idm::select<named_thread<spu_thread>>([&](u32, spu_thread& spu)
		{
			if (spu.get_type() != spu_type::threaded)
			{
				auto& ctrl = spu.int_ctrl[2];

				if (lv2_obj::check(ctrl.tag))
				{
					auto& handler = ctrl.tag->handler;

					if (lv2_obj::check(handler))
					{
						if (handler->thread.get() == &ppu)
						{
							if (spu.ch_out_intr_mbox.get_count() && ctrl.mask & SPU_INT2_STAT_MAILBOX_INT)
							{
								ctrl.stat |= SPU_INT2_STAT_MAILBOX_INT;
							}

							if (ctrl.mask & ctrl.stat)
							{
								ensure(!serv);
								serv = handler;
							}
						}
					}
				}
			}
		});

		if (serv)
		{
			// Queue interrupt, after the interrupt has finished the PPU returns to this loop
			serv->exec();
			return;
		}

		const auto state = +ppu.state;

		if (::is_stopped(state) || ppu.cmd_notify.exchange(0))
		{
			return;
		}

		thread_ctrl::wait_on(ppu.cmd_notify, 0);
	}
}
