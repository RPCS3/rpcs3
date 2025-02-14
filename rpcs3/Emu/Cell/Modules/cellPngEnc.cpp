#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "cellPngEnc.h"

LOG_CHANNEL(cellPngEnc);

template <>
void fmt_class_string<CellPngEncError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellPngEncError value)
	{
		switch (value)
		{
		STR_CASE(CELL_PNGENC_ERROR_ARG);
		STR_CASE(CELL_PNGENC_ERROR_SEQ);
		STR_CASE(CELL_PNGENC_ERROR_BUSY);
		STR_CASE(CELL_PNGENC_ERROR_EMPTY);
		STR_CASE(CELL_PNGENC_ERROR_RESET);
		STR_CASE(CELL_PNGENC_ERROR_FATAL);
		STR_CASE(CELL_PNGENC_ERROR_STREAM_ABORT);
		STR_CASE(CELL_PNGENC_ERROR_STREAM_SKIP);
		STR_CASE(CELL_PNGENC_ERROR_STREAM_OVERFLOW);
		STR_CASE(CELL_PNGENC_ERROR_STREAM_FILE_OPEN);
		}

		return unknown;
	});
}

struct png_encoder
{
	shared_mutex mutex;
	CellPngEncConfig config{};
	CellPngEncResource resource{};
	CellPngEncResourceEx resourceEx{};
};

bool check_config(vm::cptr<CellPngEncConfig> config)
{
	if (!config ||
		config->maxWidth == 0u || config->maxWidth > 1000000u ||
		config->maxHeight == 0u || config->maxHeight > 1000000u ||
		(config->maxBitDepth != 8u && config->maxBitDepth != 16u) ||
		static_cast<s32>(config->addMemSize) < 0 ||
		config->exParamNum != 0u)
	{
		return false;
	}

	return true;
}

u32 get_mem_size(vm::cptr<CellPngEncConfig> config)
{
	return config->addMemSize
		+ (config->enableSpu ? 0x78200 : 0x47a00)
		+ (config->maxBitDepth >> 1) * config->maxWidth * 7;
}


error_code cellPngEncQueryAttr(vm::cptr<CellPngEncConfig> config, vm::ptr<CellPngEncAttr> attr)
{
	cellPngEnc.todo("cellPngEncQueryAttr(config=*0x%x, attr=*0x%x)", config, attr);

	if (!attr || !check_config(config))
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	const u32 memsize = get_mem_size(config);
	attr->memSize = memsize + 0x1780;
	attr->cmdQueueDepth = 4;
	attr->versionLower = 0;
	attr->versionUpper = 0x270000;

	return CELL_OK;
}

error_code cellPngEncOpen(vm::cptr<CellPngEncConfig> config, vm::cptr<CellPngEncResource> resource, vm::ptr<u32> handle)
{
	cellPngEnc.todo("cellPngEncOpen(config=*0x%x, resource=*0x%x, handle=0x%x)", config, resource, handle);

	if (!handle || !check_config(config) ||
		!resource || !resource->memAddr || !resource->memSize ||
		resource->ppuThreadPriority < 0 || resource->ppuThreadPriority > 0xbff ||
		resource->spuThreadPriority < 0 || resource->ppuThreadPriority > 0xff)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	const u32 required_memsize = get_mem_size(config);

	if (resource->memSize < required_memsize + 0x1780U)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	auto& encoder = g_fxo->get<png_encoder>();
	{
		std::lock_guard lock(encoder.mutex);
		encoder.config = *config;
		encoder.resource = *resource;
	}

	return CELL_OK;
}

error_code cellPngEncOpenEx(vm::cptr<CellPngEncConfig> config, vm::cptr<CellPngEncResourceEx> resource, vm::ptr<u32> handle)
{
	cellPngEnc.todo("cellPngEncOpenEx(config=*0x%x, resourceEx=*0x%x, handle=0x%x)", config, resource, handle);

	if (!handle || !check_config(config) ||
		!resource || !resource->memAddr || !resource->memSize ||
		resource->ppuThreadPriority < 0 || resource->ppuThreadPriority > 0xbff ||
		resource->priority[0] > 15 || resource->priority[1] > 15 ||
		resource->priority[2] > 15 || resource->priority[3] > 15 ||
		resource->priority[4] > 15 || resource->priority[5] > 15 ||
		resource->priority[6] > 15 || resource->priority[7] > 15)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	const u32 required_memsize = get_mem_size(config);

	if (resource->memSize < required_memsize + 0x1780U)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	auto& encoder = g_fxo->get<png_encoder>();
	{
		std::lock_guard lock(encoder.mutex);
		encoder.config = *config;
		encoder.resourceEx = *resource;
	}

	return CELL_OK;
}

error_code cellPngEncClose(u32 handle)
{
	cellPngEnc.todo("cellPngEncClose(handle=0x%x)", handle);

	if (!handle)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	return CELL_OK;
}

error_code cellPngEncWaitForInput(u32 handle, b8 block)
{
	cellPngEnc.todo("cellPngEncWaitForInput(handle=0x%x, block=%d)", handle, block);

	if (!handle)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	return CELL_OK;
}

error_code cellPngEncEncodePicture(u32 handle, vm::cptr<CellPngEncPicture> picture, vm::cptr<CellPngEncEncodeParam> encodeParam, vm::cptr<CellPngEncOutputParam> outputParam)
{
	cellPngEnc.todo("cellPngEncEncodePicture(handle=0x%x, picture=*0x%x, encodeParam=*0x%x, outputParam=*0x%x)", handle, picture, encodeParam, outputParam);

	if (!handle || !picture || !picture->width || !picture->height ||
		(picture->packedPixel && picture->bitDepth >= 8) ||
		!picture->pictureAddr || picture->colorSpace > CELL_PNGENC_COLOR_SPACE_ARGB)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	auto& encoder = g_fxo->get<png_encoder>();
	{
		std::lock_guard lock(encoder.mutex);

		if (picture->width > encoder.config.maxWidth ||
			picture->height > encoder.config.maxHeight ||
			picture->bitDepth > encoder.config.maxBitDepth)
		{
			return CELL_PNGENC_ERROR_ARG;
		}
	}

	return CELL_OK;
}

error_code cellPngEncWaitForOutput(u32 handle, vm::ptr<u32> streamInfoNum, b8 block)
{
	cellPngEnc.todo("cellPngEncWaitForOutput(handle=0x%x, streamInfoNum=*0x%x, block=%d)", handle, streamInfoNum, block);

	if (!handle || !streamInfoNum)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	return CELL_OK;
}

error_code cellPngEncGetStreamInfo(u32 handle, vm::ptr<CellPngEncStreamInfo> streamInfo)
{
	cellPngEnc.todo("cellPngEncGetStreamInfo(handle=0x%x, streamInfo=*0x%x)", handle, streamInfo);

	if (!handle || !streamInfo)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	return CELL_OK;
}

error_code cellPngEncReset(u32 handle)
{
	cellPngEnc.todo("cellPngEncReset(handle=0x%x)", handle);

	if (!handle)
	{
		return CELL_PNGENC_ERROR_ARG;
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPngEnc)("cellPngEnc", []()
{
	REG_FUNC(cellPngEnc, cellPngEncQueryAttr);
	REG_FUNC(cellPngEnc, cellPngEncOpen);
	REG_FUNC(cellPngEnc, cellPngEncOpenEx);
	REG_FUNC(cellPngEnc, cellPngEncClose);
	REG_FUNC(cellPngEnc, cellPngEncWaitForInput);
	REG_FUNC(cellPngEnc, cellPngEncEncodePicture);
	REG_FUNC(cellPngEnc, cellPngEncWaitForOutput);
	REG_FUNC(cellPngEnc, cellPngEncGetStreamInfo);
	REG_FUNC(cellPngEnc, cellPngEncReset);
});
