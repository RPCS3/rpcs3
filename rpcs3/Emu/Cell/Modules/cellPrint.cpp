#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellPrint);

// Error Codes
enum
{
	CELL_PRINT_ERROR_INTERNAL            = 0x8002c401,
	CELL_PRINT_ERROR_NO_MEMORY           = 0x8002c402,
	CELL_PRINT_ERROR_PRINTER_NOT_FOUND   = 0x8002c403,
	CELL_PRINT_ERROR_INVALID_PARAM       = 0x8002c404,
	CELL_PRINT_ERROR_INVALID_FUNCTION    = 0x8002c405,
	CELL_PRINT_ERROR_NOT_SUPPORT         = 0x8002c406,
	CELL_PRINT_ERROR_OCCURRED            = 0x8002c407,
	CELL_PRINT_ERROR_CANCELED_BY_PRINTER = 0x8002c408,
};

struct CellPrintLoadParam
{
	be_t<u32> mode;
	u8 reserved[32];
};

struct CellPrintStatus
{
	be_t<s32> status;
	be_t<s32> errorStatus;
	be_t<s32> continueEnabled;
	u8 reserved[32];
};

using CellPrintCallback = void(s32 result, vm::ptr<void> userdata);

error_code cellSysutilPrintInit()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

error_code cellSysutilPrintShutdown()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

error_code cellPrintLoadAsync(vm::ptr<CellPrintCallback> function, vm::ptr<void> userdata, vm::cptr<CellPrintLoadParam> param, u32 container)
{
	cellPrint.todo("cellPrintLoadAsync(function=*0x%x, userdata=*0x%x, param=*0x%x, container=0x%x)", function, userdata, param, container);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		function(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPrintLoadAsync2(vm::ptr<CellPrintCallback> function, vm::ptr<void> userdata, vm::cptr<CellPrintLoadParam> param)
{
	cellPrint.todo("cellPrintLoadAsync2(function=*0x%x, userdata=*0x%x, param=*0x%x)", function, userdata, param);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		function(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPrintUnloadAsync(vm::ptr<CellPrintCallback> function, vm::ptr<void> userdata)
{
	cellPrint.todo("cellPrintUnloadAsync(function=*0x%x, userdata=*0x%x)", function, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		function(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPrintGetStatus(vm::ptr<CellPrintStatus> status)
{
	cellPrint.todo("cellPrintGetStatus(status=*0x%x)", status);
	return CELL_OK;
}

error_code cellPrintOpenConfig(vm::ptr<CellPrintCallback> function, vm::ptr<void> userdata)
{
	cellPrint.todo("cellPrintOpenConfig(function=*0x%x, userdata=*0x%x)", function, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		function(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPrintGetPrintableArea(vm::ptr<s32> pixelWidth, vm::ptr<s32> pixelHeight)
{
	cellPrint.todo("cellPrintGetPrintableArea(pixelWidth=*0x%x, pixelHeight=*0x%x)", pixelWidth, pixelHeight);
	return CELL_OK;
}

error_code cellPrintStartJob(s32 totalPage, s32 colorFormat)
{
	cellPrint.todo("cellPrintStartJob(totalPage=0x%x, colorFormat=0x%x)", totalPage, colorFormat);
	return CELL_OK;
}

error_code cellPrintEndJob()
{
	cellPrint.todo("cellPrintEndJob()");
	return CELL_OK;
}

error_code cellPrintCancelJob()
{
	cellPrint.todo("cellPrintCancelJob()");
	return CELL_OK;
}

error_code cellPrintStartPage()
{
	cellPrint.todo("cellPrintStartPage()");
	return CELL_OK;
}

error_code cellPrintEndPage()
{
	cellPrint.todo("cellPrintEndPage()");
	return CELL_OK;
}

error_code cellPrintSendBand(vm::cptr<u8> buff, s32 buffsize, vm::ptr<s32> sendsize)
{
	cellPrint.todo("cellPrintSendBand(buff=*0x%x, buffsize=0x%x, sendsize=*0x%x)", buff, buffsize, sendsize);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPrint)("cellPrintUtility", []()
{
	REG_FUNC(cellPrintUtility, cellSysutilPrintInit);
	REG_FUNC(cellPrintUtility, cellSysutilPrintShutdown);

	REG_FUNC(cellPrintUtility, cellPrintLoadAsync);
	REG_FUNC(cellPrintUtility, cellPrintLoadAsync2);
	REG_FUNC(cellPrintUtility, cellPrintUnloadAsync);
	REG_FUNC(cellPrintUtility, cellPrintGetStatus);
	REG_FUNC(cellPrintUtility, cellPrintOpenConfig);
	REG_FUNC(cellPrintUtility, cellPrintGetPrintableArea);
	REG_FUNC(cellPrintUtility, cellPrintStartJob);
	REG_FUNC(cellPrintUtility, cellPrintEndJob);
	REG_FUNC(cellPrintUtility, cellPrintCancelJob);
	REG_FUNC(cellPrintUtility, cellPrintStartPage);
	REG_FUNC(cellPrintUtility, cellPrintEndPage);
	REG_FUNC(cellPrintUtility, cellPrintSendBand);
});
