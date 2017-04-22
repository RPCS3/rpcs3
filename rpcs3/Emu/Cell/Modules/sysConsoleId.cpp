#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel sysConsoleId("sysConsoleId", logs::level::notice);

s32 sys_get_console_id()
{
    UNIMPLEMENTED_FUNC(sysConsoleId);
	return CELL_OK;
}

DECLARE(ppu_module_manager::sysConsoleId)("sysConsoleId", []()
{
	REG_FUNC(sysConsoleId, sys_get_console_id);
});
