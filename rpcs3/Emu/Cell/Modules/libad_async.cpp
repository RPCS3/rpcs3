#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(libad_async);

error_code sceAdAsyncOpenContext()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

error_code sceAdAsyncConnectContext()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

error_code sceAdAsyncSpaceOpen()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

error_code sceAdAsyncFlushReports()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

error_code sceAdAsyncSpaceClose()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

error_code sceAdAsyncCloseContext()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

DECLARE(ppu_module_manager::libad_async)("libad_async", []()
{
	REG_FUNC(libad_async, sceAdAsyncOpenContext);
	REG_FUNC(libad_async, sceAdAsyncConnectContext);
	REG_FUNC(libad_async, sceAdAsyncSpaceOpen);
	REG_FUNC(libad_async, sceAdAsyncFlushReports);
	REG_FUNC(libad_async, sceAdAsyncSpaceClose);
	REG_FUNC(libad_async, sceAdAsyncCloseContext);
});
