#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellUsbpspcm.AddFunc(0x657fcd36, cellUsbPspcmInit);
	cellUsbpspcm.AddFunc(0x0f7b3b6d, cellUsbPspcmEnd);
	cellUsbpspcm.AddFunc(0xf20df7fc, cellUsbPspcmCalcPoolSize);
	cellUsbpspcm.AddFunc(0xe3fbf64d, cellUsbPspcmRegister);
	cellUsbpspcm.AddFunc(0x7ff72b42, cellUsbPspcmUnregister);
	cellUsbpspcm.AddFunc(0x97670a90, cellUsbPspcmGetAddr);
	cellUsbpspcm.AddFunc(0xabe090e3, cellUsbPspcmBind);
	cellUsbpspcm.AddFunc(0x17f42197, cellUsbPspcmBindAsync);
	cellUsbpspcm.AddFunc(0x4abe830e, cellUsbPspcmWaitBindAsync);
	cellUsbpspcm.AddFunc(0x01a4cde0, cellUsbPspcmPollBindAsync);
	cellUsbpspcm.AddFunc(0xa4a5ddb4, cellUsbPspcmCancelBind);
	cellUsbpspcm.AddFunc(0xfa07d320, cellUsbPspcmClose);
	cellUsbpspcm.AddFunc(0x7277d7c3, cellUsbPspcmSend);
	cellUsbpspcm.AddFunc(0x4af23efa, cellUsbPspcmSendAsync);
	cellUsbpspcm.AddFunc(0x3caddf6c, cellUsbPspcmWaitSendAsync);
	cellUsbpspcm.AddFunc(0x7f0a3eaf, cellUsbPspcmPollSendAsync);
	cellUsbpspcm.AddFunc(0xf9883d3b, cellUsbPspcmRecv);
	cellUsbpspcm.AddFunc(0x02955295, cellUsbPspcmRecvAsync);
	cellUsbpspcm.AddFunc(0x461dc8cc, cellUsbPspcmWaitRecvAsync);
	cellUsbpspcm.AddFunc(0x7b249315, cellUsbPspcmPollRecvAsync);
	cellUsbpspcm.AddFunc(0xe68a65ac, cellUsbPspcmReset);
	cellUsbpspcm.AddFunc(0x4ef182dd, cellUsbPspcmResetAsync);
	cellUsbpspcm.AddFunc(0xe840f449, cellUsbPspcmWaitResetAsync);
	cellUsbpspcm.AddFunc(0x3f22403e, cellUsbPspcmPollResetAsync);
	cellUsbpspcm.AddFunc(0xdb864d11, cellUsbPspcmWaitData);
	cellUsbpspcm.AddFunc(0x816799dd, cellUsbPspcmPollData);
	cellUsbpspcm.AddFunc(0xe76e79ab, cellUsbPspcmCancelWaitData);
}