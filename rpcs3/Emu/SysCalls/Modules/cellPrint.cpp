#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellPrint.AddFunc(0xc9c3ef14, cellPrintLoadAsync);
	cellPrint.AddFunc(0xf0865182, cellPrintLoadAsync2);
	cellPrint.AddFunc(0xeb51aa38, cellPrintUnloadAsync);
	cellPrint.AddFunc(0x6802dfb5, cellPrintGetStatus);
	cellPrint.AddFunc(0xf9a53f35, cellPrintOpenConfig);
	cellPrint.AddFunc(0x6e952645, cellPrintGetPrintableArea);
	cellPrint.AddFunc(0x795b12b3, cellPrintStartJob);
	cellPrint.AddFunc(0xc04a7d42, cellPrintEndJob);
	cellPrint.AddFunc(0x293d9e9c, cellPrintCancelJob);
	cellPrint.AddFunc(0x865acf74, cellPrintStartPage);
	cellPrint.AddFunc(0x0d44f661, cellPrintEndPage);
	cellPrint.AddFunc(0x0a373522, cellPrintSendBand);
}