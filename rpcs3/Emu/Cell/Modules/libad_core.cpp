#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(libad_core);

s32 sceAdOpenContext()
{
	UNIMPLEMENTED_FUNC(libad_core);
	return CELL_OK;
}

s32 sceAdFlushReports()
{
	UNIMPLEMENTED_FUNC(libad_core);
	return CELL_OK;
}

s32 sceAdGetAssetInfo()
{
	UNIMPLEMENTED_FUNC(libad_core);
	return CELL_OK;
}

s32 sceAdCloseContext()
{
	UNIMPLEMENTED_FUNC(libad_core);
	return CELL_OK;
}

s32 sceAdGetSpaceInfo()
{
	UNIMPLEMENTED_FUNC(libad_core);
	return CELL_OK;
}

s32 sceAdGetConnectionInfo()
{
	UNIMPLEMENTED_FUNC(libad_core);
	return CELL_OK;
}

s32 sceAdConnectContext()
{
	UNIMPLEMENTED_FUNC(libad_core);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libad_core)("libad_core", []()
{
	REG_FUNC(libad_core, sceAdOpenContext);
	REG_FUNC(libad_core, sceAdFlushReports);
	REG_FUNC(libad_core, sceAdGetAssetInfo);
	REG_FUNC(libad_core, sceAdCloseContext);
	REG_FUNC(libad_core, sceAdGetSpaceInfo);
	REG_FUNC(libad_core, sceAdGetConnectionInfo);
	REG_FUNC(libad_core, sceAdConnectContext);
});
