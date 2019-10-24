#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(sys_io);

extern void cellPad_init();
extern void cellKb_init();
extern void cellMouse_init();


s32 sys_config_start()
{
	sys_io.todo("sys_config_start()");
	return CELL_OK;
}

s32 sys_config_stop()
{
	sys_io.todo("sys_config_stop()");
	return CELL_OK;
}

s32 sys_config_add_service_listener()
{
	sys_io.todo("sys_config_add_service_listener()");
	return CELL_OK;
}

s32 sys_config_remove_service_listener()
{
	sys_io.todo("sys_config_remove_service_listener()");
	return CELL_OK;
}

s32 sys_config_register_io_error_handler()
{
	sys_io.todo("sys_config_register_io_error_handler()");
	return CELL_OK;
}

s32 sys_config_register_service()
{
	sys_io.todo("sys_config_register_service()");
	return CELL_OK;
}

s32 sys_config_unregister_io_error_handler()
{
	sys_io.todo("sys_config_unregister_io_error_handler()");
	return CELL_OK;
}

s32 sys_config_unregister_service()
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
});
