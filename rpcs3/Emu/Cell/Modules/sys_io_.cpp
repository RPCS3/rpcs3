#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(sys_io);

extern void cellPad_init();
extern void cellKb_init();
extern void cellMouse_init();

struct libio_sys_config
{
	shared_mutex mtx;
	s32 init_ctr = 0;
	u32 stack_addr = 0;

	~libio_sys_config() noexcept
	{
		if (stack_addr)
		{
			vm::dealloc_verbose_nothrow(stack_addr, vm::stack);
		}
	}
};

// Only exists internally (has no name)
extern void libio_sys_config_init()
{
	auto cfg = g_fxo->get<libio_sys_config>();

	std::lock_guard lock(cfg->mtx);

	if (cfg->init_ctr++ == 0)
	{
		// Belongs to "_cfg_evt_hndlr" thread (8k stack)
		cfg->stack_addr = verify(HERE, vm::alloc(0x2000, vm::stack, 4096));
	}
}

extern void libio_sys_config_end()
{
	auto cfg = g_fxo->get<libio_sys_config>();

	std::lock_guard lock(cfg->mtx);

	if (cfg->init_ctr-- == 1)
	{
		verify(HERE), vm::dealloc(std::exchange(cfg->stack_addr, 0), vm::stack);
	}
}

error_code sys_config_start()
{
	sys_io.todo("sys_config_start()");

	return CELL_OK;
}

error_code sys_config_stop()
{
	sys_io.todo("sys_config_stop()");
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
	//cellPad_init();
	//cellKb_init();
	//cellMouse_init();

	REG_FUNC(sys_io, sys_config_start);
	REG_FUNC(sys_io, sys_config_stop);
	REG_FUNC(sys_io, sys_config_add_service_listener);
	REG_FUNC(sys_io, sys_config_remove_service_listener);
	REG_FUNC(sys_io, sys_config_register_io_error_handler);
	REG_FUNC(sys_io, sys_config_register_service);
	REG_FUNC(sys_io, sys_config_unregister_io_error_handler);
	REG_FUNC(sys_io, sys_config_unregister_service);
});
