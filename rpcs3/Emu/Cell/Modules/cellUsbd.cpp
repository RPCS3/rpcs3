#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellUsbd.h"

LOG_CHANNEL(cellUsbd);

template<>
void fmt_class_string<CellUsbdError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_USBD_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_USBD_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_USBD_ERROR_NO_MEMORY);
			STR_CASE(CELL_USBD_ERROR_INVALID_PARAM);
			STR_CASE(CELL_USBD_ERROR_INVALID_TRANSFER_TYPE);
			STR_CASE(CELL_USBD_ERROR_LDD_ALREADY_REGISTERED);
			STR_CASE(CELL_USBD_ERROR_LDD_NOT_ALLOCATED);
			STR_CASE(CELL_USBD_ERROR_LDD_NOT_RELEASED);
			STR_CASE(CELL_USBD_ERROR_LDD_NOT_FOUND);
			STR_CASE(CELL_USBD_ERROR_DEVICE_NOT_FOUND);
			STR_CASE(CELL_USBD_ERROR_PIPE_NOT_ALLOCATED);
			STR_CASE(CELL_USBD_ERROR_PIPE_NOT_RELEASED);
			STR_CASE(CELL_USBD_ERROR_PIPE_NOT_FOUND);
			STR_CASE(CELL_USBD_ERROR_IOREQ_NOT_ALLOCATED);
			STR_CASE(CELL_USBD_ERROR_IOREQ_NOT_RELEASED);
			STR_CASE(CELL_USBD_ERROR_IOREQ_NOT_FOUND);
			STR_CASE(CELL_USBD_ERROR_CANNOT_GET_DESCRIPTOR);
			STR_CASE(CELL_USBD_ERROR_FATAL);
		}

		return unknown;
	});
}

error_code cellUsbdInit()
{
	cellUsbd.warning("cellUsbdInit()");

	return CELL_OK;
}

error_code cellUsbdEnd()
{
	cellUsbd.warning("cellUsbdEnd()");

	return CELL_OK;
}

error_code cellUsbdSetThreadPriority()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdSetThreadPriority2()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdGetThreadPriority()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdRegisterLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdRegisterCompositeLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdRegisterExtraLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdRegisterExtraLdd2()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdUnregisterLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdUnregisterCompositeLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdUnregisterExtraLdd()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdOpenPipe()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdClosePipe()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdControlTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdBulkTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdInterruptTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdIsochronousTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdHSIsochronousTransfer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdScanStaticDescriptor()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdGetDeviceSpeed()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdGetDeviceLocation()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdSetPrivateData()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdGetPrivateData()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdAllocateMemory()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdAllocateMemoryFromContainer()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdAllocateSharedMemory()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdFreeMemory()
{
	UNIMPLEMENTED_FUNC(cellUsbd);
	return CELL_OK;
}

error_code cellUsbdResetDevice()
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
	REG_FUNC(cellUsbd, cellUsbdAllocateMemoryFromContainer);
	REG_FUNC(cellUsbd, cellUsbdAllocateSharedMemory);
	REG_FUNC(cellUsbd, cellUsbdFreeMemory);

	REG_FUNC(cellUsbd, cellUsbdResetDevice);
});
