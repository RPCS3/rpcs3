#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_process.h"

#include "Emu/RSX/GCM.h"
#include "Emu/RSX/gcm_enums.h"
#include "cellResc.h"
#include "cellVideoOut.h"

#include "util/asm.hpp"

void cellGcmSetSecondVFrequency(u32 freq);
void cellGcmSetVBlankHandler(vm::ptr<void(u32)> handler);
void cellGcmSetSecondVHandler(vm::ptr<void(u32)> handler);
u32 cellGcmGetFlipStatus();
u64 cellGcmGetLastFlipTime();
u32 cellGcmGetTiledPitchSize(u32 size);
void cellGcmResetFlipStatus();
void cellGcmSetFlipHandler(vm::ptr<void(u32)> handler);
void cellGcmSetFlipMode(u32 mode);
u32 cellGcmGetLabelAddress(u8 index);
error_code cellGcmSetPrepareFlip(ppu_thread& ppu, vm::ptr<CellGcmContextData> ctxt, u32 id);
error_code cellGcmAddressToOffset(u32 address, vm::ptr<u32> offset);
error_code cellGcmSetDisplayBuffer(u8 id, u32 offset, u32 pitch, u32 width, u32 height);

error_code cellVideoOutConfigure(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option, u32 waitForEvent);

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

u32 get_dst_index_by_buffer_mode(u32 buffer_mode)
{
	switch (buffer_mode)
	{
	case CELL_RESC_720x480: return 0;
	case CELL_RESC_720x576: return 1;
	case CELL_RESC_1280x720: return 2;
	case CELL_RESC_1920x1080: return 3;
	default: fmt::throw_exception("unexpected buffer mode 0x%x", buffer_mode);
	}
}

void get_buffer_dimensions(u32 bufferMode, u32& width, u32& height)
{
	switch (bufferMode)
	{
	case CELL_RESC_720x480:
		width  = 720;
		height = 480;
		break;
	case CELL_RESC_720x576:
		width  = 720;
		height = 576;
		break;
	case CELL_RESC_1280x720:
		width  = 1280;
		height = 720;
		break;
	case CELL_RESC_1920x1080:
		width  = 1920;
		height = 1080;
		break;
	default:
		width  = 0;
		height = 0;
		break;
	}
}

u32 FUN_00007c60(u32 buffer_mode)
{
	switch (buffer_mode)
	{
	case CELL_RESC_720x480: return 4;
	case CELL_RESC_720x576: return 5;
	case CELL_RESC_1280x720: return 2;
	case CELL_RESC_1920x1080: return 1;
	default: fmt::throw_exception("unexpected buffer mode 0x%x", buffer_mode);
	}
}

u32 FUN_00007c98(u32 format)
{
	return (format == CELL_GCM_SURFACE_F_W16Z16Y16X16) ? 2 : 0;
}

u32 get_color_buffers_count(u32 bufferMode, u32 palTemporalMode)
{
	if (bufferMode == CELL_RESC_720x576)
	{
		if (palTemporalMode >= CELL_RESC_PAL_60_INTERPOLATE &&
			palTemporalMode <= CELL_RESC_PAL_60_INTERPOLATE_DROP_FLEXIBLE)
		{
			return 6;
		}
		else if (palTemporalMode == CELL_RESC_PAL_60_DROP)
		{
			return 3;
		}
	}

	return 2;
}

s32 get_color_buffer_size(u32 bufferMode, const CellRescDsts& dsts)
{
	u32 width = 0, height = 0;
	get_buffer_dimensions(bufferMode, width, height);

	return dsts.pitch * utils::align(height, dsts.heightAlign);
}

s32 get_max_color_buffer_size()
{
	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	s32 size = 0;

	for (u32 bufferMode : {CELL_RESC_720x480, CELL_RESC_720x576, CELL_RESC_1280x720, CELL_RESC_1920x1080})
	{
		if ((bufferMode & resc_manager.config.supportModes) == 0) continue;

		const u32 dst_index = get_dst_index_by_buffer_mode(bufferMode);
		const s32 buffer_size = get_color_buffer_size(bufferMode, ::at32(resc_manager.dsts, dst_index));
		const s32 num_buffers = get_color_buffers_count(bufferMode, resc_manager.config.palTemporalMode);

		size = std::max(size, buffer_size * num_buffers);
	}

	return size;
}

u8 get_texture_format(u8 colorFormat, u8 type)
{
	u8 texture_format = 0xff;

	if (colorFormat == CELL_GCM_SURFACE_A8R8G8B8)
	{
		texture_format = CELL_GCM_TEXTURE_A8R8G8B8;
	}
	else if (colorFormat == CELL_GCM_SURFACE_F_W16Z16Y16X16)
	{
		texture_format = CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;
	}
	else
	{
		return 0xff;
	}

	if (type == CELL_GCM_SURFACE_PITCH)
	{
		texture_format |= CELL_GCM_TEXTURE_LN;
	}
	else if (type != CELL_GCM_SURFACE_SWIZZLE)
	{
		return 0xff;
	}

	return texture_format;
}

void set_vblank_handler(vm::ptr<CellRescHandler> handler)
{
	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (resc_manager.is_initialized && resc_manager.bufferMode != 0)
	{
		if (resc_manager.bufferMode != CELL_RESC_720x576 || resc_manager.config.palTemporalMode == CELL_RESC_PAL_50)
		{
			cellGcmSetVBlankHandler(handler);
			// TODO: something = 0;
			return;
		}

		if (resc_manager.config.palTemporalMode == CELL_RESC_PAL_60_FOR_HSYNC)
		{
			cellGcmSetSecondVHandler(handler);
			// TODO: something = 0;
			return;
		}
	}

	// TODO: some_handler = handler;
	return;
}

void set_flip_handler(vm::ptr<CellRescHandler> handler)
{
	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (resc_manager.is_initialized && resc_manager.bufferMode != 0)
	{
		if (resc_manager.bufferMode != CELL_RESC_720x576 ||
			resc_manager.config.palTemporalMode == CELL_RESC_PAL_50 ||
			resc_manager.config.palTemporalMode == CELL_RESC_PAL_60_FOR_HSYNC)
		{
			cellGcmSetFlipHandler(handler);
			// TODO: something = 0;
		}
	}

	// TODO: some_handler = handler;
	return;
}

void fill_vertex_array(f32 src_width = 1.0f, f32 src_height = 1.0f, f32 dst_width = 1.0f, f32 dst_height = 1.0f)
{
	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	const f32 center_x = src_width * 0.5f;
	const f32 center_y = src_height * 0.5f;

	const f32 half_texel_x = center_x / resc_manager.horizontal;
	const f32 half_texel_y = center_y / resc_manager.vertical;

	f32 tex_coord_min_x = center_x - half_texel_x;
	f32 tex_coord_max_x = center_x + half_texel_x;

	f32 tex_coord_min_y = center_y - half_texel_y;
	f32 tex_coord_max_y = center_y + half_texel_y;

	if (resc_manager.bufferMode == CELL_RESC_720x480 || resc_manager.bufferMode == CELL_RESC_720x576)
	{
		if (resc_manager.config.ratioMode == CELL_RESC_LETTERBOX)
		{
			f32 letterbox_offset_y = half_texel_y * 4.0f / 3.0f;

			if (resc_manager.config.size != 28)
			{
				letterbox_offset_y /= 1.062091588973999f;
			}

			tex_coord_min_y = center_y - letterbox_offset_y;
			tex_coord_max_y = center_y + letterbox_offset_y;
		}
		else if (resc_manager.config.ratioMode == CELL_RESC_PANSCAN)
		{
			const f32 panscan_offset_x = half_texel_x * 3.0f / 4.0f;

			tex_coord_min_x = center_x - panscan_offset_x;
			tex_coord_max_x = center_x + panscan_offset_x;
		}
	}

	f32* vertexArray = reinterpret_cast<f32*>(resc_manager.vertexArray.get_ptr());

	vertexArray[0] = -1.0f;
	vertexArray[1] = 1.0f;
	vertexArray[2] = tex_coord_min_x;
	vertexArray[3] = tex_coord_min_y;
	vertexArray[4] = 0;
	vertexArray[5] = 0;

	vertexArray[6] = 1.0f;
	vertexArray[7] = 1.0f;
	vertexArray[8] = tex_coord_max_x;
	vertexArray[9] = tex_coord_min_y;
	vertexArray[10] = dst_width;
	vertexArray[11] = 0;

	vertexArray[12] = 1.0f;
	vertexArray[13] = -1.0f;
	vertexArray[14] = tex_coord_max_x;
	vertexArray[15] = tex_coord_max_y;
	vertexArray[16] = dst_width;
	vertexArray[17] = dst_height;

	vertexArray[18] = -1.0f;
	vertexArray[19] = -1.0f;
	vertexArray[20] = tex_coord_min_x;
	vertexArray[21] = tex_coord_max_y;
	vertexArray[22] = 0;
	vertexArray[23] = dst_height;

	resc_manager.field_0x140 = 4;
}


void fill_vertex_array_from_src(s32 index)
{
	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (resc_manager.recreateVertexArray.exchange(false))
	{
		resc_manager.src_width = resc_manager.srcs[index].width;
		resc_manager.src_height = resc_manager.srcs[index].height;
	}
	else
	{
		if (resc_manager.src_width == resc_manager.srcs[index].width &&
			resc_manager.src_height == resc_manager.srcs[index].height)
		{
			return;
		}

		resc_manager.src_width = resc_manager.srcs[index].width;
		resc_manager.src_height = resc_manager.srcs[index].height;
	}

	fill_vertex_array(resc_manager.src_width, resc_manager.src_height, resc_manager.width, resc_manager.height);
}

void init_config()
{
	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	// TODO: reverse engineer the rest

	resc_manager.bufferMode = 0;
	resc_manager.tableLength = 1;

	for (u32 i = 0; i < SRC_BUFFER_NUM; i++)
	{
		resc_manager.srcs[i].format = 0;
		resc_manager.srcs[i].pitch = 0;
		resc_manager.srcs[i].width = 0;
		resc_manager.srcs[i].height = 0;
		resc_manager.srcs[i].offset = 0;
	}

	resc_manager.dsts[0].format = CELL_RESC_SURFACE_A8R8G8B8;
	resc_manager.dsts[0].heightAlign = 8;
	resc_manager.dsts[0].pitch = cellGcmGetTiledPitchSize(2880);

	resc_manager.dsts[1].format = CELL_RESC_SURFACE_A8R8G8B8;
	resc_manager.dsts[1].heightAlign = 8;
	resc_manager.dsts[1].pitch = cellGcmGetTiledPitchSize(2880);

	resc_manager.dsts[2].format = CELL_RESC_SURFACE_A8R8G8B8;
	resc_manager.dsts[2].heightAlign = 8;
	resc_manager.dsts[2].pitch = cellGcmGetTiledPitchSize(5120);

	resc_manager.dsts[3].format = CELL_RESC_SURFACE_A8R8G8B8;
	resc_manager.dsts[3].heightAlign = 8;
	resc_manager.dsts[3].pitch = cellGcmGetTiledPitchSize(7680);

	resc_manager.colorBuffers = vm::null;
	resc_manager.vertexArray = vm::null;
	resc_manager.fragmentShader = vm::null;
	resc_manager.table = 0;

	resc_manager.field_0x110 = 0;

	resc_manager.width = 0;
	resc_manager.height = 0;
	resc_manager.pitch = 0;

	resc_manager.src_width = 0;
	resc_manager.src_height = 0;

	resc_manager.bufferSize = 0;

	for (u32 i = 0; i < MAX_DST_BUFFER_NUM; i++)
	{
		resc_manager.bufferOffsets[i] = 0;
	}

	resc_manager.field_0x140 = 0;

	resc_manager.depth = 32;
	resc_manager.horizontal = 1.0f;
	resc_manager.vertical = 1.0f;

	resc_manager.is_initialized = false;
	resc_manager.recreateVertexArray = false;

	for (s32 i = 0; i < 15; i++)
	{
		resc_manager.field_0x152[i] = 0xff;
	}

	resc_manager.field_0x168 = 0;

	resc_manager.lastFlipTime = 0;

	resc_manager.field_0x1b8 = 0;
	resc_manager.field_0x1c0 = 0;
	resc_manager.field_0x1c8 = 4;
	resc_manager.field_0x1cc = 5;
	resc_manager.field_0x1d0 = 2;
	resc_manager.field_0x1d4 = 0;
	resc_manager.field_0x1e8 = 1;
	resc_manager.field_0x1f0 = 0;
	resc_manager.field_0x1f8 = 0;

	resc_manager.flipStatus = 1;

	resc_manager.palInterpolateDropFlexRatio = 0.0f;
}

error_code cellRescInit(vm::cptr<CellRescInitConfig> initConfig)
{
	cellResc.todo("cellRescInit(initConfig=*0x%x)", initConfig);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_REINITIALIZED;
	}

	if (!initConfig)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	switch (initConfig->size)
	{
	case 20:
		break;
	case 24:
		if (initConfig->interlaceMode > CELL_RESC_INTERLACE_FILTER)
		{
			return CELL_RESC_ERROR_BAD_ARGUMENT;
		}
		break;
	case 28:
		if (initConfig->interlaceMode > CELL_RESC_2X3_QUINCUNX_ALT ||
			initConfig->flipMode > CELL_RESC_DISPLAY_HSYNC)
		{
			return CELL_RESC_ERROR_BAD_ARGUMENT;
		}
		break;
	default:
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	if ((initConfig->resourcePolicy & 0xfffffffc) != 0 ||
		(initConfig->supportModes & 0xf) == 0 ||
		(initConfig->ratioMode > CELL_RESC_PANSCAN) ||
		(initConfig->palTemporalMode > CELL_RESC_PAL_60_FOR_HSYNC))
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	init_config();

	std::memset(&resc_manager.config, 0, sizeof(resc_manager.config));
	std::memcpy(&resc_manager.config, initConfig.get_ptr(), initConfig->size);

	for (u32 i = 0; i < 5; i++)
	{
		if (!resc_manager.fragmentShaders[i])
		{
			// resc_manager.fragmentShaders[i] = TODO;

			if (!resc_manager.fragmentShaders[i])
			{
				return CELL_RESC_ERROR_LESS_MEMORY;
			}
		}
	}

	resc_manager.usedFragmentShader = resc_manager.fragmentShaders[1];

	// TODO

	resc_manager.is_initialized = true;

	return CELL_OK;
}

void cellRescExit()
{
	cellResc.todo("cellRescExit()");

	auto& resc_manager = g_fxo->get<cell_resc_manager>();
	if (!resc_manager.is_initialized) return;

	if (resc_manager.bufferMode == CELL_RESC_720x576)
	{
		if (resc_manager.config.palTemporalMode != CELL_RESC_PAL_50)
		{
			cellGcmSetSecondVFrequency(3);
			cellGcmSetVBlankHandler(vm::null);
			cellGcmSetSecondVHandler(vm::null);

			if (resc_manager.config.palTemporalMode >= CELL_RESC_PAL_60_INTERPOLATE)
			{
				// TODO
			}
		}
	}

	resc_manager.is_initialized = false;
}

error_code cellRescVideoOutResolutionId2RescBufferMode(u32 resolutionId, vm::ptr<u32> bufferMode)
{
	cellResc.trace("cellRescVideoOutResolutionId2RescBufferMode(resolutionId=0x%x, bufferMode=*0x%x)", resolutionId, bufferMode);

	if (!bufferMode)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	switch (resolutionId)
	{
	case CELL_VIDEO_OUT_RESOLUTION_1080: *bufferMode = CELL_RESC_1920x1080; break;
	case CELL_VIDEO_OUT_RESOLUTION_720: *bufferMode = CELL_RESC_1280x720; break;
	case CELL_VIDEO_OUT_RESOLUTION_480: *bufferMode = CELL_RESC_720x480; break;
	case CELL_VIDEO_OUT_RESOLUTION_576: *bufferMode = CELL_RESC_720x576; break;
	default: return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellRescSetDsts(u32 bufferMode, vm::cptr<CellRescDsts> dsts)
{
	cellResc.notice("cellRescSetDsts(bufferMode=0x%x, dsts=*0x%x)", bufferMode, dsts);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (!dsts)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	switch (bufferMode)
	{
	case CELL_RESC_720x480:
	case CELL_RESC_720x576:
	case CELL_RESC_1280x720:
	case CELL_RESC_1920x1080:
		break;
	default:
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	const u32 index = get_dst_index_by_buffer_mode(bufferMode);

	resc_manager.dsts[index].format = dsts->format;
	resc_manager.dsts[index].pitch = dsts->pitch;
	resc_manager.dsts[index].heightAlign = dsts->heightAlign;
	return CELL_OK;
}

error_code cellRescSetDisplayMode(u32 bufferMode)
{
	cellResc.todo("cellRescSetDisplayMode(bufferMode=0x%x)", bufferMode);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if ((bufferMode != CELL_RESC_720x480 && bufferMode != CELL_RESC_720x576 && bufferMode != CELL_RESC_1280x720 && bufferMode != CELL_RESC_1920x1080) ||
		!(resc_manager.config.supportModes & bufferMode))
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	resc_manager.bufferMode = bufferMode;

	if (bufferMode == CELL_RESC_720x576)
	{
		const u32 pal_mode  = resc_manager.config.palTemporalMode;
		const u32 flip_mode = resc_manager.config.flipMode;

		switch (pal_mode)
		{
		case CELL_RESC_PAL_60_DROP:
		case CELL_RESC_PAL_60_INTERPOLATE:
		case CELL_RESC_PAL_60_INTERPOLATE_30_DROP:
		case CELL_RESC_PAL_60_INTERPOLATE_DROP_FLEXIBLE:
		{
			if (flip_mode == CELL_RESC_DISPLAY_HSYNC)
			{
				return CELL_RESC_ERROR_BAD_COMBINATION;
			}
			break;
		}
		case CELL_RESC_PAL_60_FOR_HSYNC:
		{
			if (flip_mode == CELL_RESC_DISPLAY_VSYNC)
			{
				return CELL_RESC_ERROR_BAD_COMBINATION;
			}
			break;
		}
		default:
			break;
		}
	}

	const u32 dst_index = get_dst_index_by_buffer_mode(bufferMode);
	resc_manager.activeDst = dst_index;

	get_buffer_dimensions(resc_manager.bufferMode, resc_manager.width, resc_manager.height);

	const u32 pitch = resc_manager.dsts[resc_manager.activeDst].pitch;
	const u32 heightAlign = resc_manager.dsts[resc_manager.activeDst].heightAlign;

	resc_manager.pitch = pitch;
	resc_manager.bufferSize = pitch * utils::align(resc_manager.height, heightAlign);

	if (resc_manager.bufferMode == CELL_RESC_720x576 &&
		resc_manager.config.palTemporalMode >= CELL_RESC_PAL_60_INTERPOLATE &&
		resc_manager.config.palTemporalMode <= CELL_RESC_PAL_60_INTERPOLATE_DROP_FLEXIBLE)
	{
		if (resc_manager.config.interlaceMode == CELL_RESC_INTERLACE_FILTER)
		{
			resc_manager.usedFragmentShader = resc_manager.fragmentShaders[4];
		}
		else
		{
			resc_manager.usedFragmentShader = resc_manager.fragmentShaders[2];
		}
	}
	else if (resc_manager.config.interlaceMode == CELL_RESC_INTERLACE_FILTER)
	{
		resc_manager.usedFragmentShader = resc_manager.fragmentShaders[3];
	}
	else
	{
		resc_manager.usedFragmentShader = resc_manager.fragmentShaders[1];
	}

	// TODO: is the return value of these functions used
	FUN_00007c60(resc_manager.bufferMode);
	FUN_00007c98(resc_manager.dsts[resc_manager.activeDst].format);

	vm::ptr<CellVideoOutConfiguration> videoOutConfig = vm::make_var<CellVideoOutConfiguration>({});
	std::memset(videoOutConfig.get_ptr(), 0, sizeof(CellVideoOutConfiguration));
	cellVideoOutConfigure(0, videoOutConfig, vm::null, 0);

	if (resc_manager.bufferMode == CELL_RESC_720x576)
	{
		switch (resc_manager.config.palTemporalMode)
		{
		case CELL_RESC_PAL_60_DROP:
		{
			*vm::get_super_ptr<u32>(cellGcmGetLabelAddress(0x11)) = 0;
			*vm::get_super_ptr<u32>(cellGcmGetLabelAddress(0x12)) = 0;
			*vm::get_super_ptr<u32>(cellGcmGetLabelAddress(0x13)) = 0;
			cellGcmSetSecondVFrequency(1);
			cellGcmSetVBlankHandler(vm::null);
			//cellGcmSetSecondVHandler(); // TODO
			cellGcmSetFlipHandler(vm::null);
			break;
		}
		case CELL_RESC_PAL_60_INTERPOLATE:
		case CELL_RESC_PAL_60_INTERPOLATE_30_DROP:
		case CELL_RESC_PAL_60_INTERPOLATE_DROP_FLEXIBLE:
		{
			// TODO

			*vm::get_super_ptr<u32>(cellGcmGetLabelAddress(0x11)) = 0;
			*vm::get_super_ptr<u32>(cellGcmGetLabelAddress(0x12)) = 0;
			*vm::get_super_ptr<u32>(cellGcmGetLabelAddress(0x13)) = 0;
			cellGcmSetSecondVFrequency(1);
			// cellGcmSetVBlankHandler(); // TODO
			// cellGcmSetSecondVHandler(); // TODO
			cellGcmSetFlipHandler(vm::null);
			break;
		}
		case CELL_RESC_PAL_60_FOR_HSYNC:
		{
			cellGcmSetSecondVFrequency(1);
			cellGcmSetVBlankHandler(vm::null);
			break;
		}
		default:
			break;
		}
	}

	if (vm::ptr<CellRescHandler> handler = vm::null) // TODO
	{
		set_vblank_handler(handler);
	}

	if (vm::ptr<CellRescHandler> handler = vm::null) // TODO
	{
		set_flip_handler(handler);
	}

	cellGcmSetFlipMode(resc_manager.config.flipMode == CELL_RESC_DISPLAY_VSYNC ? CELL_GCM_DISPLAY_VSYNC : CELL_GCM_DISPLAY_HSYNC);

	return CELL_OK;
}

error_code cellRescAdjustAspectRatio(f32 horizontal, f32 vertical)
{
	cellResc.notice("cellRescAdjustAspectRatio(horizontal=%f, vertical=%f)", horizontal, vertical);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (horizontal < 0.5f || horizontal > 2.0f || vertical < 0.5f || vertical > 2.0f)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	resc_manager.horizontal = horizontal;
	resc_manager.vertical = vertical;

	if (!resc_manager.vertexArray)
	{
		return CELL_OK;
	}

	if (resc_manager.config.interlaceMode == CELL_RESC_INTERLACE_FILTER)
	{
		resc_manager.recreateVertexArray = true;
		return CELL_OK;
	}

	fill_vertex_array();
	return CELL_OK;
}

error_code cellRescSetPalInterpolateDropFlexRatio(f32 ratio)
{
	cellResc.notice("cellRescSetPalInterpolateDropFlexRatio(ratio=%f)", ratio);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (ratio < 0.0f || ratio > 1.0f)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	resc_manager.palInterpolateDropFlexRatio = ratio;

	return CELL_OK;
}

error_code cellRescGetBufferSize(vm::ptr<s32> colorBuffers, vm::ptr<s32> vertexArray, vm::ptr<s32> fragmentShader)
{
	cellResc.notice("cellRescGetBufferSize(colorBuffers=*0x%x, vertexArray=*0x%x, fragmentShader=*0x%x)", colorBuffers, vertexArray, fragmentShader);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	vm::var<s32> sdk_version;
	if (error_code error = sys_process_get_sdk_version(sys_process_getpid(), sdk_version); error != CELL_OK)
	{
		return CELL_RESC_ERROR_x308;
	}

	s32 colorBuffersSize = 0;
	s32 fragmentShaderSize = 0;

	if ((resc_manager.config.resourcePolicy & 1) == 0)
	{
		colorBuffersSize = get_max_color_buffer_size();
		fragmentShaderSize = 768;

		if (*sdk_version < 0x280000)
		{
			if (resc_manager.config.size == 24 || resc_manager.config.size == 28)
			{
				if (resc_manager.config.interlaceMode == CELL_RESC_INTERLACE_FILTER)
				{
					if (resc_manager.bufferMode == CELL_RESC_720x576 && resc_manager.config.palTemporalMode != CELL_RESC_PAL_50)
					{
						fragmentShaderSize = 640;
					}
					else
					{
						fragmentShaderSize = 512;
					}
				}
				else if ((resc_manager.config.supportModes & 2) != 0 &&
				          resc_manager.config.palTemporalMode >= CELL_RESC_PAL_60_INTERPOLATE &&
				          resc_manager.config.palTemporalMode <= CELL_RESC_PAL_60_INTERPOLATE_DROP_FLEXIBLE)
				{
					fragmentShaderSize = 96;
				}
				else
				{
					fragmentShaderSize = 16;
				}
			}
			else if (resc_manager.config.size == 20)
			{
				fragmentShaderSize = resc_manager.usedFragmentShader->size;
			}
			else
			{
				fragmentShaderSize = 0;
			}
		}
	}
	else
	{
		colorBuffersSize = resc_manager.bufferSize * get_color_buffers_count(resc_manager.bufferMode, resc_manager.config.palTemporalMode);
		fragmentShaderSize = resc_manager.usedFragmentShader->size;
	}

	if (colorBuffers)
	{
		*colorBuffers = colorBuffersSize;
	}

	if (vertexArray)
	{
		*vertexArray = 384;
	}

	if (fragmentShader)
	{
		*fragmentShader = fragmentShaderSize;
	}

	return CELL_OK;
}

s32 cellRescGetNumColorBuffers(u32 dstMode, u32 palTemporalMode, u32 reserved)
{
	cellResc.trace("cellRescGetNumColorBuffers(dstMode=0x%x, palTemporalMode=0x%x, reserved=%d)", dstMode, palTemporalMode, reserved);

	if (reserved != 0)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return get_color_buffers_count(dstMode, palTemporalMode);
}

error_code cellRescGcmSurface2RescSrc(vm::cptr<CellGcmSurface> gcmSurface, vm::ptr<CellRescSrc> rescSrc)
{
	cellResc.notice("cellRescGcmSurface2RescSrc(gcmSurface=*0x%x, rescSrc=*0x%x)", gcmSurface, rescSrc);

	if (!gcmSurface || !rescSrc)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	const u8 texture_format = get_texture_format(gcmSurface->colorFormat, gcmSurface->type);

	std::memset(rescSrc.get_ptr(), 0, sizeof(CellRescSrc));

	short factor_width = 1;
	short factor_height = 1;

	if (resc_manager.config.size == 24 || resc_manager.config.size == 28)
	{
		const u8 antialias = gcmSurface->antialias;
		if (antialias == CELL_GCM_SURFACE_DIAGONAL_CENTERED_2)
		{
			factor_width = 2;
		}
		else if (antialias == CELL_GCM_SURFACE_SQUARE_CENTERED_4 || antialias == CELL_GCM_SURFACE_SQUARE_ROTATED_4)
		{
			factor_width = 2;
			factor_height = 2;
		}
	}

	rescSrc->format = texture_format;
	rescSrc->pitch = gcmSurface->colorPitch[0];
	rescSrc->width = gcmSurface->width * factor_width;
	rescSrc->offset = gcmSurface->colorOffset[0];
	rescSrc->height = gcmSurface->height * factor_height;

	return CELL_OK;
}

error_code cellRescSetSrc(s32 idx, vm::cptr<CellRescSrc> src)
{
	cellResc.notice("cellRescSetSrc(idx=0x%x, src=*0x%x)", idx, src);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (idx >= SRC_BUFFER_NUM || !src || !src->width || src->width > 4096 || !src->height || src->height > 4096)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	const u32 texture_format = src->format & 0x9f;
	if (texture_format != CELL_GCM_TEXTURE_A8R8G8B8 && texture_format != CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	resc_manager.srcs[idx] = *src;

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

	// TODO

	return CELL_OK;
}

void cellRescSetWaitFlip(ppu_thread& ppu, vm::ptr<CellGcmContextData> con)
{
	cellResc.notice("cellRescSetWaitFlip(con=*0x%x)", con);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	vm::bptr<u32> current = con->current;

	if (current + 4 > con->end)
	{
		if (con->callback(ppu, con, 4))
		{
			return;
		}

		current = con->current;
	}

	if (resc_manager.bufferMode == CELL_RESC_720x576 &&
		resc_manager.config.palTemporalMode != CELL_RESC_PAL_50 &&
		resc_manager.config.palTemporalMode != CELL_RESC_PAL_60_FOR_HSYNC)
	{
		current[0] = 0x40064;
		current[1] = 0x10 * ((resc_manager.field_0x1dc == 1) ? 0x13 : 0x12);
		current[2] = 0x40068;
		current[3] = 0;
	}
	else
	{
		current[0] = 0x40064;
		current[1] = 0x10;
		current[2] = 0x40068;
		current[3] = 0;
	}

	con->current = current + 4;
}

error_code cellRescSetBufferAddress(vm::ptr<void> colorBuffers, vm::ptr<void> vertexArray, vm::ptr<void> fragmentShader)
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

	resc_manager.colorBuffers = colorBuffers;
	resc_manager.vertexArray = vertexArray;
	resc_manager.fragmentShader = fragmentShader;

	vm::var<u32> offset;
	cellGcmAddressToOffset(colorBuffers.addr(), offset);

	u32 color_buffers = get_color_buffers_count(resc_manager.bufferMode, resc_manager.config.palTemporalMode);

	for (u32 i = 0; i < color_buffers; i++)
	{
		resc_manager.bufferOffsets[i] = *offset + i * resc_manager.bufferSize;
	}

	for (u32 i = 0; true; i++)
	{
		color_buffers = get_color_buffers_count(resc_manager.bufferMode, resc_manager.config.palTemporalMode);

		if (i >= color_buffers)
		{
			if (resc_manager.config.interlaceMode != CELL_RESC_INTERLACE_FILTER)
			{
				fill_vertex_array();
			}

			if (resc_manager.usedFragmentShader->unk_1 != 0)
			{
				if (resc_manager.usedFragmentShader->size != 0)
				{
					std::memcpy(resc_manager.fragmentShader.get_ptr(), resc_manager.usedFragmentShader->data.get_ptr(), resc_manager.usedFragmentShader->size);

					vm::var<u32> fs_offset;
					cellGcmAddressToOffset(resc_manager.fragmentShader.addr(), fs_offset);
					resc_manager.usedFragmentShader->offset = *fs_offset;
				}
			}

			// TODO
		}

		const error_code error = cellGcmSetDisplayBuffer(i, resc_manager.bufferOffsets[i], resc_manager.pitch, resc_manager.width, resc_manager.height);
		if (error != CELL_OK)
		{
			// Something is called here before the return. Not sure if there's a NOP here or something wasn't decompiled correctly.
			return error;
		}
	}

	return CELL_OK;
}

void cellRescSetFlipHandler(vm::ptr<CellRescHandler> handler)
{
	cellResc.todo("cellRescSetFlipHandler(handler=*0x%x)", handler);

	set_flip_handler(handler);
}

void cellRescSetVBlankHandler(vm::ptr<CellRescHandler> handler)
{
	cellResc.todo("cellRescSetVBlankHandler(handler=*0x%x)", handler);

	set_vblank_handler(handler);
}

void cellRescResetFlipStatus()
{
	cellResc.trace("cellRescResetFlipStatus()");

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (resc_manager.bufferMode == CELL_RESC_720x576 &&
		resc_manager.config.palTemporalMode != CELL_RESC_PAL_50 &&
		resc_manager.config.palTemporalMode != CELL_RESC_PAL_60_FOR_HSYNC)
	{
		resc_manager.flipStatus = 0;
		return;
	}

	cellGcmResetFlipStatus();
}

u32 cellRescGetFlipStatus()
{
	cellResc.trace("cellRescGetFlipStatus()");

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (resc_manager.bufferMode == CELL_RESC_720x576 &&
		resc_manager.config.palTemporalMode != CELL_RESC_PAL_50 &&
		resc_manager.config.palTemporalMode != CELL_RESC_PAL_60_FOR_HSYNC)
	{
		return resc_manager.flipStatus ^ 1;
	}

	return cellGcmGetFlipStatus();
}

u64 cellRescGetLastFlipTime()
{
	cellResc.trace("cellRescGetLastFlipTime()");

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (resc_manager.bufferMode == CELL_RESC_720x576 &&
		resc_manager.config.palTemporalMode != CELL_RESC_PAL_50 &&
		resc_manager.config.palTemporalMode != CELL_RESC_PAL_60_FOR_HSYNC)
	{
		return resc_manager.lastFlipTime;
	}

	return cellGcmGetLastFlipTime();
}

s32 cellRescGetRegisterCount()
{
	cellResc.todo("cellRescGetRegisterCount()");
	// TODO
	return 0;
}

void cellRescSetRegisterCount(s32 regCount)
{
	cellResc.todo("cellRescSetRegisterCount(regCount=0x%x)", regCount);
	// TODO
}

error_code cellRescCreateInterlaceTable(vm::ptr<void> ea_addr, f32 srcH, CellRescTableElement depth, s32 length)
{
	cellResc.todo("cellRescCreateInterlaceTable(ea_addr=*0x%x, srcH=%f, depth=0x%x, length=%d)", ea_addr, srcH, +depth, length);

	auto& resc_manager = g_fxo->get<cell_resc_manager>();

	if (!resc_manager.is_initialized)
	{
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (!ea_addr || srcH <= 0.0f || static_cast<s32>(depth) < CELL_RESC_ELEMENT_FLOAT || length > 1)
	{
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	if (resc_manager.height == 0)
	{
		return CELL_RESC_ERROR_BAD_COMBINATION;
	}

	// TODO

	resc_manager.depth = depth;
	resc_manager.table = ea_addr.addr();
	resc_manager.tableLength = length;

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
