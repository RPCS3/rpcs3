#include "stdafx.h"
#if 0

void cellPrint_init();
Module cellPrint(0xf02a, cellPrint_init);

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

int cellPrintLoadAsync()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintLoadAsync2()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintUnloadAsync()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintGetStatus()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintOpenConfig()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintGetPrintableArea()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintStartJob()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintEndJob()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintCancelJob()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintStartPage()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintEndPage()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

int cellPrintSendBand()
{
	UNIMPLEMENTED_FUNC(cellPrint);
	return CELL_OK;
}

void cellPrint_init()
{
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
}
#endif
