#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellDtcpIpUtility);

s32 cellDtcpIpRead()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpFinalize()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpActivate()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpOpen()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpCheckActivation()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpInitialize()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpGetDecryptedData()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpStopSequence()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpSeek()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpStartSequence()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpSetEncryptedData()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpClose()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

s32 cellDtcpIpSuspendActivationForDebug()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellDtcpIpUtility)("cellDtcpIpUtility", []()
{
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpRead);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpFinalize);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpActivate);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpOpen);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpCheckActivation);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpInitialize);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpGetDecryptedData);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpStopSequence);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpSeek);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpStartSequence);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpSetEncryptedData);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpClose);
	REG_FUNC(cellDtcpIpUtility, cellDtcpIpSuspendActivationForDebug);
});
