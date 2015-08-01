#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"

extern Module sys_io;

extern void cellPad_init();
extern void cellKb_init();
extern void cellMouse_init();


s32 sys_config_start()
{
	throw EXCEPTION("");
}

s32 sys_config_stop()
{
	throw EXCEPTION("");
}

s32 sys_config_add_service_listener()
{
	throw EXCEPTION("");
}

s32 sys_config_remove_service_listener()
{
	throw EXCEPTION("");
}

s32 sys_config_register_service()
{
	throw EXCEPTION("");
}

s32 sys_config_unregister_service()
{
	throw EXCEPTION("");
}


Module sys_io("sys_io", []()
{
	cellPad_init();
	cellKb_init();
	cellMouse_init();

	REG_FUNC(sys_io, sys_config_start);
	REG_FUNC(sys_io, sys_config_stop);
	REG_FUNC(sys_io, sys_config_add_service_listener);
	REG_FUNC(sys_io, sys_config_remove_service_listener);
	REG_FUNC(sys_io, sys_config_register_service);
	REG_FUNC(sys_io, sys_config_unregister_service);
});
