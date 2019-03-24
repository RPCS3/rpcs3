#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellPesmUtility);

s32 cellPesmCloseDevice()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmEncryptSample()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmEncryptSample2()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmEndMovieRec()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmFinalize()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmFinalize2()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmGetSinf()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmInitEntry()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmInitEntry2()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmInitialize()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmLoadAsync()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmOpenDevice()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmPrepareRec()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmStartMovieRec()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

s32 cellPesmUnloadAsync()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPesmUtility)("cellPesmUtility", []()
{
	REG_FUNC(cellPesmUtility, cellPesmInitialize);
	REG_FUNC(cellPesmUtility, cellPesmFinalize);
	REG_FUNC(cellPesmUtility, cellPesmLoadAsync);
	REG_FUNC(cellPesmUtility, cellPesmOpenDevice);
	REG_FUNC(cellPesmUtility, cellPesmEncryptSample);
	REG_FUNC(cellPesmUtility, cellPesmUnloadAsync);
	REG_FUNC(cellPesmUtility, cellPesmGetSinf);
	REG_FUNC(cellPesmUtility, cellPesmStartMovieRec);
	REG_FUNC(cellPesmUtility, cellPesmInitEntry);
	REG_FUNC(cellPesmUtility, cellPesmEndMovieRec);
	REG_FUNC(cellPesmUtility, cellPesmEncryptSample2);
	REG_FUNC(cellPesmUtility, cellPesmFinalize2);
	REG_FUNC(cellPesmUtility, cellPesmCloseDevice);
	REG_FUNC(cellPesmUtility, cellPesmInitEntry2);
	REG_FUNC(cellPesmUtility, cellPesmPrepareRec);
});
