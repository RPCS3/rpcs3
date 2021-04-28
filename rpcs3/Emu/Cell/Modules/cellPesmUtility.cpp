#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellPesmUtility);

error_code cellPesmCloseDevice()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmEncryptSample()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmEncryptSample2()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmEndMovieRec()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmFinalize()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmFinalize2()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmGetSinf()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmInitEntry()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmInitEntry2()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmInitialize()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmLoadAsync()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmOpenDevice()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmPrepareRec()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmStartMovieRec()
{
	UNIMPLEMENTED_FUNC(cellPesmUtility);
	return CELL_OK;
}

error_code cellPesmUnloadAsync()
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
