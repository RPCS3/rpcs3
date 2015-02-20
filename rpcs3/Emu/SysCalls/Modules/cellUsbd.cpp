#include "stdafx.h"
#if 0

void cellUsbd_init();
Module cellUsbd(0x001c, cellUsbd_init);

// Return Codes
enum
{
	CELL_USBD_ERROR_NOT_INITIALIZED        = 0x80110001,
	CELL_USBD_ERROR_ALREADY_INITIALIZED    = 0x80110002,
	CELL_USBD_ERROR_NO_MEMORY              = 0x80110003,
	CELL_USBD_ERROR_INVALID_PARAM          = 0x80110004,
	CELL_USBD_ERROR_INVALID_TRANSFER_TYPE  = 0x80110005,
	CELL_USBD_ERROR_LDD_ALREADY_REGISTERED = 0x80110006,
	CELL_USBD_ERROR_LDD_NOT_ALLOCATED      = 0x80110007,
	CELL_USBD_ERROR_LDD_NOT_RELEASED       = 0x80110008,
	CELL_USBD_ERROR_LDD_NOT_FOUND          = 0x80110009,
	CELL_USBD_ERROR_DEVICE_NOT_FOUND       = 0x8011000a,
	CELL_USBD_ERROR_PIPE_NOT_ALLOCATED     = 0x8011000b,
	CELL_USBD_ERROR_PIPE_NOT_RELEASED      = 0x8011000c,
	CELL_USBD_ERROR_PIPE_NOT_FOUND         = 0x8011000d,
	CELL_USBD_ERROR_IOREQ_NOT_ALLOCATED    = 0x8011000e,
	CELL_USBD_ERROR_IOREQ_NOT_RELEASED     = 0x8011000f,
	CELL_USBD_ERROR_IOREQ_NOT_FOUND        = 0x80110010,
	CELL_USBD_ERROR_CANNOT_GET_DESCRIPTOR  = 0x80110011,
	CELL_USBD_ERROR_FATAL                  = 0x801100ff,
};

int cellUsbdInit()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdEnd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdSetThreadPriority()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdSetThreadPriority2()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdGetThreadPriority()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdRegisterLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdRegisterExtraLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdRegisterExtraLdd2()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdUnregisterLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdUnregisterExtraLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdOpenPipe()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdClosePipe()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdControlTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdBulkTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdInterruptTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdIsochronousTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdHSIsochronousTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdScanStaticDescriptor()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdGetDeviceSpeed()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdGetDeviceLocation()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdSetPrivateData()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdGetPrivateData()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdAllocateMemory()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

int cellUsbdFreeMemory()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

void cellUsbd_init()
{
	REG_FUNC(cellUsbd, cellUsbdInit);
	REG_FUNC(cellUsbd, cellUsbdEnd);

	REG_FUNC(cellUsbd, cellUsbdSetThreadPriority);
	REG_FUNC(cellUsbd, cellUsbdSetThreadPriority2);
	REG_FUNC(cellUsbd, cellUsbdGetThreadPriority);

	REG_FUNC(cellUsbd, cellUsbdRegisterLdd);
	REG_FUNC(cellUsbd, cellUsbdRegisterExtraLdd);
	REG_FUNC(cellUsbd, cellUsbdRegisterExtraLdd2);
	REG_FUNC(cellUsbd, cellUsbdUnregisterLdd);
	REG_FUNC(cellUsbd, cellUsbdUnregisterExtraLdd);

	REG_FUNC(cellUsbd, cellUsbdOpenPipe);
	REG_FUNC(cellUsbd, cellUsbdClosePipe);

	REG_FUNC(cellUsbd, cellUsbdControlTransfer);
	REG_FUNC(cellUsbd, cellUsbdBulkTransfer);
	REG_FUNC(cellUsbd, cellUsbdInterruptTransfer);
	REG_FUNC(cellUsbd, cellUsbdIsochronousTransfer);
	REG_FUNC(cellUsbd, cellUsbdHSIsochronousTransfer);

	REG_FUNC(cellUsbd, cellUsbdScanStaticDescriptor);
	REG_FUNC(cellUsbd, cellUsbdGetDeviceSpeed);
	REG_FUNC(cellUsbd, cellUsbdGetDeviceLocation);

	REG_FUNC(cellUsbd, cellUsbdSetPrivateData);
	REG_FUNC(cellUsbd, cellUsbdGetPrivateData);

	REG_FUNC(cellUsbd, cellUsbdAllocateMemory);
	REG_FUNC(cellUsbd, cellUsbdFreeMemory);
}
#endif
