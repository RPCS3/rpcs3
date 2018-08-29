#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellJpgEnc.h"

LOG_CHANNEL(cellJpgEnc);


s32 cellJpgEncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncOpen()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncClose()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncWaitForInput()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncEncodePicture()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncEncodePicture2()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncGetStreamInfo()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncReset()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellJpgEnc)("cellJpgEnc", []()
{
	REG_FUNC(cellJpgEnc, cellJpgEncQueryAttr);
	REG_FUNC(cellJpgEnc, cellJpgEncOpen);
	REG_FUNC(cellJpgEnc, cellJpgEncOpenEx);
	REG_FUNC(cellJpgEnc, cellJpgEncClose);
	REG_FUNC(cellJpgEnc, cellJpgEncWaitForInput);
	REG_FUNC(cellJpgEnc, cellJpgEncEncodePicture);
	REG_FUNC(cellJpgEnc, cellJpgEncEncodePicture2);
	REG_FUNC(cellJpgEnc, cellJpgEncWaitForOutput);
	REG_FUNC(cellJpgEnc, cellJpgEncGetStreamInfo);
	REG_FUNC(cellJpgEnc, cellJpgEncReset);
});
