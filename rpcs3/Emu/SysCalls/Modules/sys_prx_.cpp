#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/SysCalls/lv2/sys_prx.h"
#include "sysPrxForUser.h"

extern Module sysPrxForUser;

s64 sys_prx_exitspawn_with_level()
{
	sysPrxForUser.Log("sys_prx_exitspawn_with_level()");
	return CELL_OK;
}

s32 sys_prx_load_module_list_on_memcontainer()
{
	throw EXCEPTION("");
}

void sysPrxForUser_sys_prx_init()
{
	// TODO: split syscalls and liblv2 functions

	REG_FUNC(sysPrxForUser, sys_prx_load_module);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_by_fd);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_on_memcontainer);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_on_memcontainer_by_fd);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_list);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_list_on_memcontainer);
	REG_FUNC(sysPrxForUser, sys_prx_start_module);
	REG_FUNC(sysPrxForUser, sys_prx_stop_module);
	REG_FUNC(sysPrxForUser, sys_prx_unload_module);
	REG_FUNC(sysPrxForUser, sys_prx_register_library);
	REG_FUNC(sysPrxForUser, sys_prx_unregister_library);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_list);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_info);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_id_by_name);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_id_by_address);
	REG_FUNC(sysPrxForUser, sys_prx_exitspawn_with_level);
	REG_FUNC(sysPrxForUser, sys_prx_get_my_module_id);
}
