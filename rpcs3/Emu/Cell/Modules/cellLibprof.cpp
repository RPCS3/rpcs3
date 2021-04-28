#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellLibprof);

error_code cellUserTraceInit()
{
	UNIMPLEMENTED_FUNC(cellLibprof);
	return CELL_OK;
}

error_code cellUserTraceRegister()
{
	UNIMPLEMENTED_FUNC(cellLibprof);
	return CELL_OK;
}

error_code cellUserTraceUnregister()
{
	UNIMPLEMENTED_FUNC(cellLibprof);
	return CELL_OK;
}

error_code cellUserTraceTerminate()
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
