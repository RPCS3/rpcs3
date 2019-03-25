#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(libad_core);

s32 sceAdGetAssetInfo()
{
	UNIMPLEMENTED_FUNC(libad_core);
	return CELL_OK;
}

s32 sceAdGetSpaceInfo()
{
	UNIMPLEMENTED_FUNC(libad_core);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libad_core)("libad_core", []()
{
	REG_FUNC(libad_core, sceAdGetAssetInfo);
	REG_FUNC(libad_core, sceAdGetSpaceInfo);
});
