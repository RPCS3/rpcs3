#include "stdafx.h"
#if 0

void cellUsbpspcm_init();
Module cellUsbpspcm(0x0030, cellUsbpspcm_init);

// Return Codes
enum
{
	CELL_USBPSPCM_ERROR_NOT_INITIALIZED = 0x80110401,
	CELL_USBPSPCM_ERROR_ALREADY         = 0x80110402,
	CELL_USBPSPCM_ERROR_INVALID         = 0x80110403,
	CELL_USBPSPCM_ERROR_NO_MEMORY       = 0x80110404,
	CELL_USBPSPCM_ERROR_BUSY            = 0x80110405,
	CELL_USBPSPCM_ERROR_INPROGRESS      = 0x80110406,
	CELL_USBPSPCM_ERROR_NO_SPACE        = 0x80110407,
	CELL_USBPSPCM_ERROR_CANCELED        = 0x80110408,
	CELL_USBPSPCM_ERROR_RESETTING       = 0x80110409,
	CELL_USBPSPCM_ERROR_RESET_END       = 0x8011040A,
	CELL_USBPSPCM_ERROR_CLOSED          = 0x8011040B,
	CELL_USBPSPCM_ERROR_NO_DATA         = 0x8011040C,
};

int cellUsbPspcmInit()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmEnd()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmCalcPoolSize()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmRegister()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmUnregister()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmGetAddr()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmBind()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmBindAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmWaitBindAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmPollBindAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmCancelBind()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmClose()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmSend()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmSendAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmWaitSendAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmPollSendAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmRecv()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmRecvAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmWaitRecvAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmPollRecvAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmReset()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmResetAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmWaitResetAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmPollResetAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmWaitData()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmPollData()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

int cellUsbPspcmCancelWaitData()
{
	UNIMPLEMENTED_FUNC(cellUsbpspcm);
	return CELL_OK;
}

void cellUsbpspcm_init()
{
	REG_FUNC(cellUsbpspcm, cellUsbPspcmInit);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmEnd);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmCalcPoolSize);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmRegister);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmUnregister);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmGetAddr);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmBind);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmBindAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmWaitBindAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmPollBindAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmCancelBind);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmClose);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmSend);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmSendAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmWaitSendAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmPollSendAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmRecv);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmRecvAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmWaitRecvAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmPollRecvAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmReset);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmResetAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmWaitResetAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmPollResetAsync);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmWaitData);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmPollData);
	REG_FUNC(cellUsbpspcm, cellUsbPspcmCancelWaitData);
}
#endif
