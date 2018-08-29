#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellLibprof);

s32 cellUserTraceInit()
{
	UNIMPLEMENTED_FUNC(cellLibprof);
	return CELL_OK;
}

s32 cellUserTraceRegister()
{
	UNIMPLEMENTED_FUNC(cellLibprof);
	return CELL_OK;
}

s32 cellUserTraceUnregister()
{
	UNIMPLEMENTED_FUNC(cellLibprof);
	return CELL_OK;
}

s32 cellUserTraceTerminate()
{
	UNIMPLEMENTED_FUNC(cellLibprof);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellLibprof)("cellLibprof", []()
{
	REG_FUNC(cellLibprof, cellUserTraceInit);
	REG_FUNC(cellLibprof, cellUserTraceRegister);
	REG_FUNC(cellLibprof, cellUserTraceUnregister);
	REG_FUNC(cellLibprof, cellUserTraceTerminate);
});
