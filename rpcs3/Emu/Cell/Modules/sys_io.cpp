#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel sys_io("sys_io", logs::level::notice);

extern void cellPad_init();
extern void cellKb_init();
extern void cellMouse_init();


s32 sys_config_start()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sys_config_stop()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sys_config_add_service_listener()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sys_config_remove_service_listener()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sys_config_register_io_error_handler()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sys_config_register_service()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sys_config_unregister_io_error_handler()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sys_config_unregister_service()
{
	fmt::throw_exception("Unimplemented" HERE);
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
