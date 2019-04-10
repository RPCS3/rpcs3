#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(libad_async);

s32 sceAdAsyncOpenContext()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

s32 sceAdAsyncConnectContext()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

s32 sceAdAsyncSpaceOpen()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

s32 sceAdAsyncFlushReports()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

s32 sceAdAsyncSpaceClose()
{
	UNIMPLEMENTED_FUNC(libad_async);
	return CELL_OK;
}

s32 sceAdAsyncCloseContext()
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
