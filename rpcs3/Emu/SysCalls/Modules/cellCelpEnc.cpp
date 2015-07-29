#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellCelpEnc;

// Return Codes
enum
{
	CELL_CELPENC_ERROR_FAILED      = 0x80614001,
	CELL_CELPENC_ERROR_SEQ         = 0x80614002,
	CELL_CELPENC_ERROR_ARG         = 0x80614003,
	CELL_CELPENC_ERROR_CORE_FAILED = 0x80614081,
	CELL_CELPENC_ERROR_CORE_SEQ    = 0x80614082,
	CELL_CELPENC_ERROR_CORE_ARG    = 0x80614083,
};

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

Module cellCelpEnc("cellCelpEnc", []()
{
	REG_FUNC(cellCelpEnc, cellCelpEncQueryAttr);
	REG_FUNC(cellCelpEnc, cellCelpEncOpen);
	REG_FUNC(cellCelpEnc, cellCelpEncOpenEx);
	REG_FUNC(cellCelpEnc, cellCelpEncClose);
	REG_FUNC(cellCelpEnc, cellCelpEncStart);
	REG_FUNC(cellCelpEnc, cellCelpEncEnd);
	REG_FUNC(cellCelpEnc, cellCelpEncEncodeFrame);
	REG_FUNC(cellCelpEnc, cellCelpEncWaitForOutput);
	REG_FUNC(cellCelpEnc, cellCelpEncGetAu);
});
