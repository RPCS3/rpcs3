#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellPrint.h"

extern Module cellPrint;

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

s32 cellPrintLoadAsync(vm::ptr<CellPrintCallback> function, vm::ptr<u32> userdata, vm::cptr<CellPrintLoadParam> param, u32 container)
{
	cellPrint.Todo("cellPrintLoadAsync(function=*0x%x, userdata=*0x%x, param=*0x%x, container=%d)", function, userdata, param, container);
	return CELL_OK;
}

s32 cellPrintLoadAsync2(vm::ptr<CellPrintCallback> function, vm::ptr<u32> userdata, vm::cptr<CellPrintLoadParam> param)
{
	cellPrint.Todo("cellPrintLoadAsync2(function=*0x%x, userdata=*0x%x, param=*0x%x)", function, userdata, param);
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
