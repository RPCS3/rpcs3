#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellDtcpIpUtility);

error_code cellDtcpIpRead()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpFinalize()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpActivate()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpOpen()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpCheckActivation()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpInitialize()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpGetDecryptedData()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpStopSequence()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpSeek()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpStartSequence()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpSetEncryptedData()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpClose()
{
	UNIMPLEMENTED_FUNC(cellDtcpIpUtility);
	return CELL_OK;
}

error_code cellDtcpIpSuspendActivationForDebug()
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
