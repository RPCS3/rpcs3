#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellCelpEnc.h"

LOG_CHANNEL(cellCelpEnc);

s32 cellCelpEncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

s32 cellCelpEncOpen()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

s32 cellCelpEncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

s32 cellCelpEncOpenExt()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

s32 cellCelpEncClose()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

s32 cellCelpEncStart()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

s32 cellCelpEncEnd()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

s32 cellCelpEncEncodeFrame()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

s32 cellCelpEncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

s32 cellCelpEncGetAu()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellCelpEnc)("cellCelpEnc", []()
{
	REG_FUNC(cellCelpEnc, cellCelpEncQueryAttr);
	REG_FUNC(cellCelpEnc, cellCelpEncOpen);
	REG_FUNC(cellCelpEnc, cellCelpEncOpenEx);
	REG_FUNC(cellCelpEnc, cellCelpEncOpenExt);
	REG_FUNC(cellCelpEnc, cellCelpEncClose);
	REG_FUNC(cellCelpEnc, cellCelpEncStart);
	REG_FUNC(cellCelpEnc, cellCelpEncEnd);
	REG_FUNC(cellCelpEnc, cellCelpEncEncodeFrame);
	REG_FUNC(cellCelpEnc, cellCelpEncWaitForOutput);
	REG_FUNC(cellCelpEnc, cellCelpEncGetAu);
});
