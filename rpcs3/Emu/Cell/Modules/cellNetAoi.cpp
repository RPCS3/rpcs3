#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellNetAoi);

error_code cellNetAoiDeletePeer()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

error_code cellNetAoiInit()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

error_code cellNetAoiGetPspTitleId()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

error_code cellNetAoiTerm()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

error_code cellNetAoiStop()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

error_code cellNetAoiGetRemotePeerInfo()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

error_code cellNetAoiStart()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

error_code cellNetAoiGetLocalInfo()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

error_code cellNetAoiAddPeer()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellNetAoi)("cellNetAoi", []()
{
	REG_FUNC(cellNetAoi, cellNetAoiDeletePeer);
	REG_FUNC(cellNetAoi, cellNetAoiInit);
	REG_FUNC(cellNetAoi, cellNetAoiGetPspTitleId);
	REG_FUNC(cellNetAoi, cellNetAoiTerm);
	REG_FUNC(cellNetAoi, cellNetAoiStop);
	REG_FUNC(cellNetAoi, cellNetAoiGetRemotePeerInfo);
	REG_FUNC(cellNetAoi, cellNetAoiStart);
	REG_FUNC(cellNetAoi, cellNetAoiGetLocalInfo);
	REG_FUNC(cellNetAoi, cellNetAoiAddPeer);
});
