#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellNetAoi);

s32 cellNetAoiDeletePeer()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

s32 cellNetAoiInit()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

s32 cellNetAoiGetPspTitleId()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

s32 cellNetAoiTerm()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

s32 cellNetAoiStop()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

s32 cellNetAoiGetRemotePeerInfo()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

s32 cellNetAoiStart()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

s32 cellNetAoiGetLocalInfo()
{
	UNIMPLEMENTED_FUNC(cellNetAoi);
	return CELL_OK;
}

s32 cellNetAoiAddPeer()
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
