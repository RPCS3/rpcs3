#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellUsbd.AddFunc(0xd0e766fe, cellUsbdInit);
	cellUsbd.AddFunc(0x35f22ac3, cellUsbdEnd);

	cellUsbd.AddFunc(0xc24af1d7, cellUsbdSetThreadPriority);
	cellUsbd.AddFunc(0x5c832bd7, cellUsbdSetThreadPriority2);
	cellUsbd.AddFunc(0xd5263dea, cellUsbdGetThreadPriority);

	cellUsbd.AddFunc(0x359befba, cellUsbdRegisterLdd);
	cellUsbd.AddFunc(0x7fe92c54, cellUsbdRegisterExtraLdd);
	cellUsbd.AddFunc(0xbd554bcb, cellUsbdRegisterExtraLdd2);
	cellUsbd.AddFunc(0x64951ac7, cellUsbdUnregisterLdd);
	cellUsbd.AddFunc(0x90460081, cellUsbdUnregisterExtraLdd);

	cellUsbd.AddFunc(0x254289ac, cellUsbdOpenPipe);
	cellUsbd.AddFunc(0x9763e962, cellUsbdClosePipe);

	cellUsbd.AddFunc(0x97cf128e, cellUsbdControlTransfer);
	cellUsbd.AddFunc(0xac77eb78, cellUsbdBulkTransfer);
	cellUsbd.AddFunc(0x0f411262, cellUsbdInterruptTransfer);
	cellUsbd.AddFunc(0xde58c4c2, cellUsbdIsochronousTransfer);
	cellUsbd.AddFunc(0x7a1b6eab, cellUsbdHSIsochronousTransfer);

	cellUsbd.AddFunc(0x2fb08e1e, cellUsbdScanStaticDescriptor);
	cellUsbd.AddFunc(0xbdbd2428, cellUsbdGetDeviceSpeed);
	cellUsbd.AddFunc(0xdb819e03, cellUsbdGetDeviceLocation);

	cellUsbd.AddFunc(0x63bfdb97, cellUsbdSetPrivateData);
	cellUsbd.AddFunc(0x5de3af36, cellUsbdGetPrivateData);

	cellUsbd.AddFunc(0x074dbb39, cellUsbdAllocateMemory);
	cellUsbd.AddFunc(0x4e456e81, cellUsbdFreeMemory);
}