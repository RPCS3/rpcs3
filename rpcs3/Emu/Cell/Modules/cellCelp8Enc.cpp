#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellCelp8Enc.h"

LOG_CHANNEL(cellCelp8Enc);


s32 cellCelp8EncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

s32 cellCelp8EncOpen()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

s32 cellCelp8EncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

s32 cellCelp8EncClose()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

s32 cellCelp8EncStart()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

s32 cellCelp8EncEnd()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

s32 cellCelp8EncEncodeFrame()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

s32 cellCelp8EncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

s32 cellCelp8EncGetAu()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellCelp8Enc)("cellCelp8Enc", []()
{
	REG_FUNC(cellCelp8Enc, cellCelp8EncQueryAttr);
	REG_FUNC(cellCelp8Enc, cellCelp8EncOpen);
	REG_FUNC(cellCelp8Enc, cellCelp8EncOpenEx);
	REG_FUNC(cellCelp8Enc, cellCelp8EncClose);
	REG_FUNC(cellCelp8Enc, cellCelp8EncStart);
	REG_FUNC(cellCelp8Enc, cellCelp8EncEnd);
	REG_FUNC(cellCelp8Enc, cellCelp8EncEncodeFrame);
	REG_FUNC(cellCelp8Enc, cellCelp8EncWaitForOutput);
	REG_FUNC(cellCelp8Enc, cellCelp8EncGetAu);
});
