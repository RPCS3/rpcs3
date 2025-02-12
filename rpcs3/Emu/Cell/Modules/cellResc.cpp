#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/RSX/GCM.h"
#include "Emu/RSX/gcm_enums.h"
#include "cellResc.h"
#include "cellVideoOut.h"

LOG_CHANNEL(cellResc);

template <>
void fmt_class_string<CellRescError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellRescError value)
	{
		switch (value)
		{
		STR_CASE(CELL_RESC_ERROR_NOT_INITIALIZED);
		STR_CASE(CELL_RESC_ERROR_REINITIALIZED);
		STR_CASE(CELL_RESC_ERROR_BAD_ALIGNMENT);
		STR_CASE(CELL_RESC_ERROR_BAD_ARGUMENT);
		STR_CASE(CELL_RESC_ERROR_LESS_MEMORY);
		STR_CASE(CELL_RESC_ERROR_GCM_FLIP_QUE_FULL);
		STR_CASE(CELL_RESC_ERROR_BAD_COMBINATION);
		STR_CASE(CELL_RESC_ERROR_x308);
		}

		return unknown;
	});
}

error_code cellRescInit(vm::cptr<CellRescInitConfig> initConfig)
{
	cellResc.todo("cellRescInit(initConfig=*0x%x)", initConfig);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_REINITIALIZED;
	}

	if (!initConfig || initConfig->size > 28) // TODO: more checks
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	resc_manager.config =
	{
		initConfig->size,
		initConfig->resourcePolicy,
		initConfig->supportModes,
		initConfig->ratioMode,
		initConfig->palTemporalMode,
		initConfig->interlaceMode,
		initConfig->flipMode
	};
	resc_manager.is_initialized = true;

	return CELL_OK;
}

void cellRescExit()
{
	cellResc.todo("cellRescExit()");

	auto& resc_manager = g_fxo->get<cell_resc_manager>();
	resc_manager.is_initialized = false;
}

error_code cellRescVideoOutResolutionId2RescBufferMode(u32 resolutionId, vm::cptr<u32> bufferMode)
{
	cellResc.todo("cellRescVideoOutResolutionId2RescBufferMode(resolutionId=%d, bufferMode=*0x%x)", resolutionId, bufferMode);

	if (!bufferMode || !resolutionId || resolutionId > CELL_VIDEO_OUT_RESOLUTION_576)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellRescSetDsts(u32 bufferMode, vm::cptr<CellRescDsts> dsts)
{
	cellResc.todo("cellRescSetDsts(bufferMode=%d, dsts=*0x%x)", bufferMode, dsts);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (!dsts || !bufferMode || bufferMode > CELL_RESC_1920x1080) // TODO: is the bufferMode check correct?
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellRescSetDisplayMode(u32 bufferMode)
{
	cellResc.todo("cellRescSetDisplayMode(bufferMode=%d)", bufferMode);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (!bufferMode || bufferMode > CELL_RESC_1920x1080 || !(resc_manager.config.support_modes & bufferMode)) // TODO: is the bufferMode check correct?
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	if (bufferMode == CELL_RESC_720x576)
	{
		const u32 pal_mode  = resc_manager.config.pal_temporal_mode;
		const u32 flip_mode = resc_manager.config.flip_mode;

		// Check if palTemporalMode is any INTERPOLATE mode or CELL_RESC_PAL_60_DROP
		if ((pal_mode - CELL_RESC_PAL_60_INTERPOLATE) <= CELL_RESC_PAL_60_INTERPOLATE || pal_mode == CELL_RESC_PAL_60_DROP)
		{
			if (flip_mode == CELL_RESC_DISPLAY_HSYNC)
			{
				return CELL_RESC_ERROR_BAD_COMBINATION;
			}
		}

		if (pal_mode == CELL_RESC_PAL_60_FOR_HSYNC)
		{
			if (flip_mode == CELL_RESC_DISPLAY_VSYNC)
			{
				return CELL_RESC_ERROR_BAD_COMBINATION;
			}
		}
	}

	resc_manager.buffer_mode = bufferMode;

	return CELL_OK;
}

error_code cellRescAdjustAspectRatio(f32 horizontal, f32 vertical)
{
	cellResc.todo("cellRescAdjustAspectRatio(horizontal=%f, vertical=%f)", horizontal, vertical);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (horizontal < 0.5f || horizontal > 2.0f || vertical < 0.5f || vertical > 2.0f)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellRescSetPalInterpolateDropFlexRatio(f32 ratio)
{
	cellResc.todo("cellRescSetPalInterpolateDropFlexRatio(ratio=%f)", ratio);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (ratio < 0.0f || ratio > 1.0f)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellRescGetBufferSize(vm::ptr<s32> colorBuffers, vm::ptr<s32> vertexArray, vm::ptr<s32> fragmentShader)
{
	cellResc.todo("cellRescGetBufferSize(colorBuffers=*0x%x, vertexArray=*0x%x, fragmentShader=*0x%x)", colorBuffers, vertexArray, fragmentShader);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	//if (something)
	//{
	//	return CELL_RESC_ERROR_x308;
	//}

	//if (colorBuffers)
	//{
	//	colorBuffers = something
	//}

	//if (vertexArray)
	//{
	//	vertexArray = something
	//}

	//if (fragmentShader)
	//{
	//	fragmentShader = something
	//}

	return CELL_OK;
}

s32 cellRescGetNumColorBuffers(u32 dstMode, u32 palTemporalMode, s32 reserved)
{
	cellResc.todo("cellRescGetNumColorBuffers(dstMode=%d, palTemporalMode=%d, reserved=%d)", dstMode, palTemporalMode, reserved);

	if (reserved != 0)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	if (dstMode == CELL_RESC_720x576)
	{
		// Check if palTemporalMode is any INTERPOLATE mode
		if ((palTemporalMode - CELL_RESC_PAL_60_INTERPOLATE) <= CELL_RESC_PAL_60_INTERPOLATE)
		{
			return 6;
		}

		if (palTemporalMode == CELL_RESC_PAL_60_DROP)
		{
			return 3;
		}
	}

	return 2;
}

error_code cellRescGcmSurface2RescSrc(vm::cptr<CellGcmSurface> gcmSurface, vm::cptr<CellRescSrc> rescSrc)
{
	cellResc.todo("cellRescGcmSurface2RescSrc(gcmSurface=*0x%x, rescSrc=*0x%x)", gcmSurface, rescSrc);

	if (!gcmSurface || !rescSrc)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellRescSetSrc(s32 idx, vm::cptr<CellRescSrc> src)
{
	cellResc.todo("cellRescSetSrc(idx=0x%x, src=*0x%x)", idx, src);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (idx >= SRC_BUFFER_NUM || !src || !src->width || !src->height || src->height > 4096) // TODO: is this correct?
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellRescSetConvertAndFlip(vm::ptr<CellGcmContextData> con, s32 idx)
{
	cellResc.todo("cellRescSetConvertAndFlip(con=*0x%x, idx=0x%x)", con, idx);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (idx >= SRC_BUFFER_NUM)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellRescSetWaitFlip(vm::ptr<CellGcmContextData> con)
{
	cellResc.todo("cellRescSetWaitFlip(con=*0x%x)", con);

	return CELL_OK;
}

error_code cellRescSetBufferAddress(vm::cptr<u32> colorBuffers, vm::cptr<u32> vertexArray, vm::cptr<u32> fragmentShader)
{
	cellResc.todo("cellRescSetBufferAddress(colorBuffers=*0x%x, vertexArray=*0x%x, fragmentShader=*0x%x)", colorBuffers, vertexArray, fragmentShader);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (!colorBuffers || !vertexArray || !fragmentShader)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	if (!colorBuffers.aligned(128) || !vertexArray.aligned(4) || !fragmentShader.aligned(64)) // clrlwi with 25, 30, 26
	{
		return CELL_RESC_ERROR_BAD_ALIGNMENT;
	}

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

error_code cellRescCreateInterlaceTable(vm::ptr<void> ea_addr, f32 srcH, CellRescTableElement depth, s32 length)
{
	cellResc.todo("cellRescCreateInterlaceTable(ea_addr=0x%x, srcH=%f, depth=%d, length=%d)", ea_addr, srcH, +depth, length);

	if (!ea_addr || srcH <= 0.0f || depth > CELL_RESC_ELEMENT_FLOAT || length <= 0) // TODO: srcH check correct?
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.buffer_mode)
	{
		return CELL_RESC_ERROR_BAD_COMBINATION;
	}

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
