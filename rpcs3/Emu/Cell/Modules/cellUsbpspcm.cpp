#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellUsbPspcm);

// Return Codes
enum CellUsbpspcmError : u32
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

template<>
void fmt_class_string<CellUsbpspcmError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_USBPSPCM_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_USBPSPCM_ERROR_ALREADY);
			STR_CASE(CELL_USBPSPCM_ERROR_INVALID);
			STR_CASE(CELL_USBPSPCM_ERROR_NO_MEMORY);
			STR_CASE(CELL_USBPSPCM_ERROR_BUSY);
			STR_CASE(CELL_USBPSPCM_ERROR_INPROGRESS);
			STR_CASE(CELL_USBPSPCM_ERROR_NO_SPACE);
			STR_CASE(CELL_USBPSPCM_ERROR_CANCELED);
			STR_CASE(CELL_USBPSPCM_ERROR_RESETTING);
			STR_CASE(CELL_USBPSPCM_ERROR_RESET_END);
			STR_CASE(CELL_USBPSPCM_ERROR_CLOSED);
			STR_CASE(CELL_USBPSPCM_ERROR_NO_DATA);
		}

		return unknown;
	});
}

error_code cellUsbPspcmInit()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmEnd()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmCalcPoolSize()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmRegister()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmUnregister()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmGetAddr()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmBind()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmBindAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmWaitBindAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmPollBindAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmCancelBind()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmClose()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmSend()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmSendAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmWaitSendAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmPollSendAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmRecv()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmRecvAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmWaitRecvAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmPollRecvAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmReset()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmResetAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmWaitResetAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmPollResetAsync()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmWaitData()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmPollData()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

error_code cellUsbPspcmCancelWaitData()
{
	UNIMPLEMENTED_FUNC(cellUsbPspcm);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellUsbPspcm)("cellUsbPspcm", []()
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
