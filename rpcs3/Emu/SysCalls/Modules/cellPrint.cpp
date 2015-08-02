#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellPrint;

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

s32 cellSysutilPrintInit()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellSysutilPrintShutdown()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintLoadAsync()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintLoadAsync2()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintUnloadAsync()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintGetStatus()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintOpenConfig()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintGetPrintableArea()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintStartJob()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintEndJob()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintCancelJob()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintStartPage()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintEndPage()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

s32 cellPrintSendBand()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

Module cellPrint("cellPrint", []()
{
	REG_FUNC(cellPrint, cellSysutilPrintInit);
	REG_FUNC(cellPrint, cellSysutilPrintShutdown);

	REG_FUNC(cellPrint, cellPrintLoadAsync);
	REG_FUNC(cellPrint, cellPrintLoadAsync2);
	REG_FUNC(cellPrint, cellPrintUnloadAsync);
	REG_FUNC(cellPrint, cellPrintGetStatus);
	REG_FUNC(cellPrint, cellPrintOpenConfig);
	REG_FUNC(cellPrint, cellPrintGetPrintableArea);
	REG_FUNC(cellPrint, cellPrintStartJob);
	REG_FUNC(cellPrint, cellPrintEndJob);
	REG_FUNC(cellPrint, cellPrintCancelJob);
	REG_FUNC(cellPrint, cellPrintStartPage);
	REG_FUNC(cellPrint, cellPrintEndPage);
	REG_FUNC(cellPrint, cellPrintSendBand);
});
