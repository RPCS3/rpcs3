#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "cellUsbd.h"

logs::channel cellUsbd("cellUsbd");

s32 cellUsbdInit()
{
	cellUsbd.warning("cellUsbdInit()");

	return CELL_OK;
}

s32 cellUsbdEnd()
{
	cellUsbd.warning("cellUsbdEnd()");

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

s32 cellUsbdRegisterCompositeLdd()
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

s32 cellUsbdUnregisterCompositeLdd()
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

s32 cellUsbdResetDevice()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellUsbd)("cellUsbd", []()
{
	REG_FUNC(cellUsbd, cellUsbdInit);
	REG_FUNC(cellUsbd, cellUsbdEnd);

	REG_FUNC(cellUsbd, cellUsbdSetThreadPriority);
	REG_FUNC(cellUsbd, cellUsbdSetThreadPriority2);
	REG_FUNC(cellUsbd, cellUsbdGetThreadPriority);

	REG_FUNC(cellUsbd, cellUsbdRegisterLdd);
	REG_FUNC(cellUsbd, cellUsbdRegisterCompositeLdd);
	REG_FUNC(cellUsbd, cellUsbdRegisterExtraLdd);
	REG_FUNC(cellUsbd, cellUsbdRegisterExtraLdd2);
	REG_FUNC(cellUsbd, cellUsbdUnregisterLdd);
	REG_FUNC(cellUsbd, cellUsbdUnregisterCompositeLdd);
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

	REG_FUNC(cellUsbd, cellUsbdResetDevice);
});
