#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/RSX/GCM.h"
#include "cellResc.h"

LOG_CHANNEL(cellResc);

s32 cellRescInit(vm::ptr<CellRescInitConfig> initConfig)
{
	cellResc.todo("cellRescInit(initConfig=*0x%x)", initConfig);

	return CELL_OK;
}

void cellRescExit()
{
	cellResc.todo("cellRescExit()");
}

s32 cellRescVideoOutResolutionId2RescBufferMode(u32 resolutionId, vm::ptr<u32> bufferMode)
{
	cellResc.todo("cellRescVideoOutResolutionId2RescBufferMode(resolutionId=%d, bufferMode=*0x%x)", resolutionId, bufferMode);

	return CELL_OK;
}

s32 cellRescSetDsts(u32 dstsMode, vm::ptr<CellRescDsts> dsts)
{
	cellResc.todo("cellRescSetDsts(dstsMode=%d, dsts=*0x%x)", dstsMode, dsts);

	return CELL_OK;
}

s32 cellRescSetDisplayMode(u32 displayMode)
{
	cellResc.todo("cellRescSetDisplayMode(displayMode=%d)", displayMode);

	return CELL_OK;
}

s32 cellRescAdjustAspectRatio(f32 horizontal, f32 vertical)
{
	cellResc.todo("cellRescAdjustAspectRatio(horizontal=%f, vertical=%f)", horizontal, vertical);

	return CELL_OK;
}

s32 cellRescSetPalInterpolateDropFlexRatio(f32 ratio)
{
	cellResc.todo("cellRescSetPalInterpolateDropFlexRatio(ratio=%f)", ratio);

	return CELL_OK;
}

s32 cellRescGetBufferSize(vm::ptr<u32> colorBuffers, vm::ptr<u32> vertexArray, vm::ptr<u32> fragmentShader)
{
	cellResc.todo("cellRescGetBufferSize(colorBuffers=*0x%x, vertexArray=*0x%x, fragmentShader=*0x%x)", colorBuffers, vertexArray, fragmentShader);

	return CELL_OK;
}

s32 cellRescGetNumColorBuffers(u32 dstMode, u32 palTemporalMode, u32 reserved)
{
	cellResc.todo("cellRescGetNumColorBuffers(dstMode=%d, palTemporalMode=%d, reserved=%d)", dstMode, palTemporalMode, reserved);

	return 2;
}

s32 cellRescGcmSurface2RescSrc(vm::ptr<CellGcmSurface> gcmSurface, vm::ptr<CellRescSrc> rescSrc)
{
	cellResc.todo("cellRescGcmSurface2RescSrc(gcmSurface=*0x%x, rescSrc=*0x%x)", gcmSurface, rescSrc);

	return CELL_OK;
}

s32 cellRescSetSrc(s32 idx, vm::ptr<CellRescSrc> src)
{
	cellResc.todo("cellRescSetSrc(idx=0x%x, src=*0x%x)", idx, src);

	return CELL_OK;
}

s32 cellRescSetConvertAndFlip(ppu_thread& ppu, vm::ptr<CellGcmContextData> cntxt, s32 idx)
{
	cellResc.todo("cellRescSetConvertAndFlip(cntxt=*0x%x, idx=0x%x)", cntxt, idx);

	return CELL_OK;
}

s32 cellRescSetWaitFlip()
{
	cellResc.todo("cellRescSetWaitFlip()");

	return CELL_OK;
}

s32 cellRescSetBufferAddress(vm::ptr<u32> colorBuffers, vm::ptr<u32> vertexArray, vm::ptr<u32> fragmentShader)
{
	cellResc.todo("cellRescSetBufferAddress(colorBuffers=*0x%x, vertexArray=*0x%x, fragmentShader=*0x%x)", colorBuffers, vertexArray, fragmentShader);

	return CELL_OK;
}

void cellRescSetFlipHandler(vm::ptr<void(u32)> handler)
{
	cellResc.todo("cellRescSetFlipHandler(handler=*0x%x)", handler);
}

void cellRescResetFlipStatus()
{
	cellResc.todo("cellRescResetFlipStatus()");
}

s32 cellRescGetFlipStatus()
{
	cellResc.todo("cellRescGetFlipStatus()");

	return 0;
}

s32 cellRescGetRegisterCount()
{
	cellResc.todo("cellRescGetRegisterCount()");
	return 0;
}

u64 cellRescGetLastFlipTime()
{
	cellResc.todo("cellRescGetLastFlipTime()");

	return 0;
}

void cellRescSetRegisterCount(s32 regCount)
{
	cellResc.todo("cellRescSetRegisterCount(regCount=0x%x)", regCount);
}

void cellRescSetVBlankHandler(vm::ptr<void(u32)> handler)
{
	cellResc.todo("cellRescSetVBlankHandler(handler=*0x%x)", handler);
}

s32 cellRescCreateInterlaceTable(u32 ea_addr, f32 srcH, CellRescTableElement depth, s32 length)
{
	cellResc.todo("cellRescCreateInterlaceTable(ea_addr=0x%x, srcH=%f, depth=%d, length=%d)", ea_addr, srcH, +depth, length);

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellResc)("cellResc", []()
{
	REG_FUNC(cellResc, cellRescSetConvertAndFlip);
	REG_FUNC(cellResc, cellRescSetWaitFlip);
	REG_FUNC(cellResc, cellRescSetFlipHandler);
	REG_FUNC(cellResc, cellRescGcmSurface2RescSrc);
	REG_FUNC(cellResc, cellRescGetNumColorBuffers);
	REG_FUNC(cellResc, cellRescSetDsts);
	REG_FUNC(cellResc, cellRescResetFlipStatus);
	REG_FUNC(cellResc, cellRescSetPalInterpolateDropFlexRatio);
	REG_FUNC(cellResc, cellRescGetRegisterCount);
	REG_FUNC(cellResc, cellRescAdjustAspectRatio);
	REG_FUNC(cellResc, cellRescSetDisplayMode);
	REG_FUNC(cellResc, cellRescExit);
	REG_FUNC(cellResc, cellRescInit);
	REG_FUNC(cellResc, cellRescGetBufferSize);
	REG_FUNC(cellResc, cellRescGetLastFlipTime);
	REG_FUNC(cellResc, cellRescSetSrc);
	REG_FUNC(cellResc, cellRescSetRegisterCount);
	REG_FUNC(cellResc, cellRescSetBufferAddress);
	REG_FUNC(cellResc, cellRescGetFlipStatus);
	REG_FUNC(cellResc, cellRescVideoOutResolutionId2RescBufferMode);
	REG_FUNC(cellResc, cellRescSetVBlankHandler);
	REG_FUNC(cellResc, cellRescCreateInterlaceTable);
});
