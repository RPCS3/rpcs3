#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel sysBdMediaId("sysBdMediaId", logs::level::notice);

s32 sys_get_bd_media_id()
{
	UNIMPLEMENTED_FUNC(sysBdMediaId);
	return CELL_OK;
}

DECLARE(ppu_module_manager::sysBdMediaId)("sysBdMediaId", []()
{
	REG_FUNC(sysBdMediaId, sys_get_bd_media_id);
});
