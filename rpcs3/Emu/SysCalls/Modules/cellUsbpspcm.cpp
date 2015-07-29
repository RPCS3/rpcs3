#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellUsbPspcm;

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

s32 cellUsbPspcmInit()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmEnd()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmCalcPoolSize()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmRegister()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmUnregister()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmGetAddr()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmBind()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmBindAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmWaitBindAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmPollBindAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmCancelBind()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmClose()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmSend()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmSendAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmWaitSendAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmPollSendAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmRecv()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmRecvAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmWaitRecvAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmPollRecvAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmReset()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmResetAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmWaitResetAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmPollResetAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmWaitData()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmPollData()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

s32 cellUsbPspcmCancelWaitData()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

Module cellUsbPspcm("cellUsbPspcm", []()
{
	REG_FUNC(cellUsbPspcm, cellUsbPspcmInit);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmEnd);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmCalcPoolSize);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmRegister);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmUnregister);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmGetAddr);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmBind);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmBindAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmWaitBindAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmPollBindAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmCancelBind);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmClose);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmSend);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmSendAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmWaitSendAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmPollSendAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmRecv);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmRecvAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmWaitRecvAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmPollRecvAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmReset);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmResetAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmWaitResetAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmPollResetAsync);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmWaitData);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmPollData);
	REG_FUNC(cellUsbPspcm, cellUsbPspcmCancelWaitData);
});
