#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_ppu_thread.h"
#include "Emu/Cell/Modules/sysPrxForUser.h"

LOG_CHANNEL(sys_io);

extern void cellPad_init();
extern void cellKb_init();
extern void cellMouse_init();

struct libio_sys_config
{
	shared_mutex mtx;
	s32 init_ctr = 0;
	u32 ppu_id = 0;
	u32 queue_id = 0;

	~libio_sys_config() noexcept
	{
	}
};

extern void cellPad_NotifyStateChange(u32 index, u32 state);

void config_event_entry(ppu_thread& ppu)
{
	auto& cfg = g_fxo->get<libio_sys_config>();

	// Ensure awake
	ppu.check_state();

	while (!sys_event_queue_receive(ppu, cfg.queue_id, vm::null, 0))
	{
		if (ppu.is_stopped())
		{
			return;
		}

		// Some delay
		thread_ctrl::wait_for(10000);

		// Wakeup
		ppu.check_state();

		const u64 arg1 = ppu.gpr[5];
		const u64 arg2 = ppu.gpr[6];
		const u64 arg3 = ppu.gpr[7];

		// TODO: Reverse-engineer proper event system

		if (arg1 == 1)
		{
			cellPad_NotifyStateChange(arg2, arg3);
		}
	}

	ppu_execute<&sys_ppu_thread_exit>(ppu, 0);
}

extern void send_sys_io_connect_event(u32 index, u32 state)
{
	auto& cfg = g_fxo->get<libio_sys_config>();

	std::lock_guard lock(cfg.mtx);

	if (cfg.init_ctr)
	{
		if (auto port = idm::get<lv2_obj, lv2_event_queue>(cfg.queue_id))
		{
			port->send(0, 1, index, state);
		}
	}
}

error_code sys_config_start(ppu_thread& ppu)
{
	sys_io.warning("sys_config_start()");

	auto& cfg = g_fxo->get<libio_sys_config>();

	std::lock_guard lock(cfg.mtx);

	if (cfg.init_ctr++ == 0)
	{
		// Run thread
		vm::var<u64> _tid;
		vm::var<u32> queue_id;
		vm::var<char[]> _name = vm::make_str("_cfg_evt_hndlr");

		vm::var<sys_event_queue_attribute_t> attr;
		attr->protocol = SYS_SYNC_PRIORITY;
		attr->type = SYS_PPU_QUEUE;
		attr->name_u64 = 0;

		ensure(CELL_OK == sys_event_queue_create(ppu, queue_id, attr, 0, 0x20));
		ensure(CELL_OK == ppu_execute<&sys_ppu_thread_create>(ppu, +_tid, g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(config_event_entry)), 0, 512, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, +_name));

		cfg.ppu_id = static_cast<u32>(*_tid);
		cfg.queue_id = *queue_id;
	}

	return CELL_OK;
}

error_code sys_config_stop(ppu_thread& ppu)
{
	sys_io.warning("sys_config_stop()");

	auto& cfg = g_fxo->get<libio_sys_config>();

	std::lock_guard lock(cfg.mtx);

	if (cfg.init_ctr && cfg.init_ctr-- == 1)
	{
		ensure(CELL_OK == sys_event_queue_destroy(ppu, cfg.queue_id, SYS_EVENT_QUEUE_DESTROY_FORCE));
		ensure(CELL_OK == sys_ppu_thread_join(ppu, cfg.ppu_id, +vm::var<u64>{}));
	}
	else
	{
		// TODO: Unknown error
	}

	return CELL_OK;
}

error_code sys_config_add_service_listener()
{
	sys_io.todo("sys_config_add_service_listener()");
	return CELL_OK;
}

error_code sys_config_remove_service_listener()
{
	sys_io.todo("sys_config_remove_service_listener()");
	return CELL_OK;
}

error_code sys_config_register_io_error_handler()
{
	sys_io.todo("sys_config_register_io_error_handler()");
	return CELL_OK;
}

error_code sys_config_register_service()
{
	sys_io.todo("sys_config_register_service()");
	return CELL_OK;
}

error_code sys_config_unregister_io_error_handler()
{
	sys_io.todo("sys_config_unregister_io_error_handler()");
	return CELL_OK;
}

error_code sys_config_unregister_service()
{
	sys_io.todo("sys_config_unregister_service()");
	return CELL_OK;
}


DECLARE(ppu_module_manager::sys_io)("sys_io", []()
{
	cellPad_init();
	cellKb_init();
	cellMouse_init();

	REG_FUNC(sys_io, sys_config_start);
	REG_FUNC(sys_io, sys_config_stop);
	REG_FUNC(sys_io, sys_config_add_service_listener);
	REG_FUNC(sys_io, sys_config_remove_service_listener);
	REG_FUNC(sys_io, sys_config_register_io_error_handler);
	REG_FUNC(sys_io, sys_config_register_service);
	REG_FUNC(sys_io, sys_config_unregister_io_error_handler);
	REG_FUNC(sys_io, sys_config_unregister_service);

	REG_HIDDEN_FUNC(config_event_entry);
});
