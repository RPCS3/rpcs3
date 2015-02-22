#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellUsbd.h"

extern Module cellUsbd;

s32 cellUsbdInit()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdEnd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdSetThreadPriority()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdSetThreadPriority2()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdGetThreadPriority()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdRegisterLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdRegisterExtraLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdRegisterExtraLdd2()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdUnregisterLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdUnregisterExtraLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdOpenPipe()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdClosePipe()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdControlTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdBulkTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdInterruptTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdIsochronousTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdHSIsochronousTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdScanStaticDescriptor()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdGetDeviceSpeed()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdGetDeviceLocation()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdSetPrivateData()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdGetPrivateData()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdAllocateMemory()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

s32 cellUsbdFreeMemory()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

Module cellUsbd("cellUsbd", []()
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
});
