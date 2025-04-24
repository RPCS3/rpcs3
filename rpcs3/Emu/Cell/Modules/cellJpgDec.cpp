#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

// STB_IMAGE_IMPLEMENTATION is already defined in stb_image.cpp
#include <stb_image.h>

#include "Emu/Cell/lv2/sys_fs.h"
#include "cellJpgDec.h"

LOG_CHANNEL(cellJpgDec);

template <>
void fmt_class_string<CellJpgDecError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_JPGDEC_ERROR_HEADER);
			STR_CASE(CELL_JPGDEC_ERROR_STREAM_FORMAT);
			STR_CASE(CELL_JPGDEC_ERROR_ARG);
			STR_CASE(CELL_JPGDEC_ERROR_SEQ);
			STR_CASE(CELL_JPGDEC_ERROR_BUSY);
			STR_CASE(CELL_JPGDEC_ERROR_FATAL);
			STR_CASE(CELL_JPGDEC_ERROR_OPEN_FILE);
			STR_CASE(CELL_JPGDEC_ERROR_SPU_UNSUPPORT);
			STR_CASE(CELL_JPGDEC_ERROR_CB_PARAM);
		}

		return unknown;
	});
}

error_code cellJpgDecCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam)
{
	cellJpgDec.todo("cellJpgDecCreate(mainHandle=0x%x, threadInParam=0x%x, threadOutParam=0x%x)", mainHandle, threadInParam, threadOutParam);
	return CELL_OK;
}

error_code cellJpgDecExtCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam, u32 extThreadInParam, u32 extThreadOutParam)
{
	cellJpgDec.todo("cellJpgDecExtCreate(mainHandle=0x%x, threadInParam=0x%x, threadOutParam=0x%x, extThreadInParam=0x%x, extThreadOutParam=0x%x)", mainHandle, threadInParam, threadOutParam, extThreadInParam, extThreadOutParam);
	return CELL_OK;
}

error_code cellJpgDecDestroy(u32 mainHandle)
{
	cellJpgDec.todo("cellJpgDecDestroy(mainHandle=0x%x)", mainHandle);
	return CELL_OK;
}

error_code cellJpgDecOpen(u32 mainHandle, vm::ptr<u32> subHandle, vm::ptr<CellJpgDecSrc> src, vm::ptr<CellJpgDecOpnInfo> openInfo)
{
	cellJpgDec.warning("cellJpgDecOpen(mainHandle=0x%x, subHandle=*0x%x, src=*0x%x, openInfo=*0x%x)", mainHandle, subHandle, src, openInfo);

	CellJpgDecSubHandle current_subHandle;

	current_subHandle.fd = 0;
	current_subHandle.src = *src;

	switch (src->srcSelect)
	{
	case CELL_JPGDEC_BUFFER:
		current_subHandle.fileSize = src->streamSize;
		break;

	case CELL_JPGDEC_FILE:
	{
		// Get file descriptor and size
		const std::string real_path = vfs::get(src->fileName.get_ptr());
		fs::file file_s(real_path);
		if (!file_s) return CELL_JPGDEC_ERROR_OPEN_FILE;

		current_subHandle.fileSize = file_s.size();
		current_subHandle.fd = idm::make<lv2_fs_object, lv2_file>(src->fileName.get_ptr(), std::move(file_s), 0, 0, real_path);
		break;
	}
	default: break; // TODO
	}

	// From now, every u32 subHandle argument is a pointer to a CellJpgDecSubHandle struct.
	*subHandle = idm::make<CellJpgDecSubHandle>(current_subHandle);

	return CELL_OK;
}

error_code cellJpgDecExtOpen()
{
	cellJpgDec.todo("cellJpgDecExtOpen()");
	return CELL_OK;
}

error_code cellJpgDecClose(u32 mainHandle, u32 subHandle)
{
	cellJpgDec.warning("cellJpgDecOpen(mainHandle=0x%x, subHandle=0x%x)", mainHandle, subHandle);

	const auto subHandle_data = idm::get_unlocked<CellJpgDecSubHandle>(subHandle);

	if (!subHandle_data)
	{
		return CELL_JPGDEC_ERROR_FATAL;
	}

	idm::remove<lv2_fs_object, lv2_file>(subHandle_data->fd);
	idm::remove<CellJpgDecSubHandle>(subHandle);

	return CELL_OK;
}

error_code cellJpgDecReadHeader(u32 mainHandle, u32 subHandle, vm::ptr<CellJpgDecInfo> info)
{
	cellJpgDec.trace("cellJpgDecReadHeader(mainHandle=0x%x, subHandle=0x%x, info=*0x%x)", mainHandle, subHandle, info);

	const auto subHandle_data = idm::get_unlocked<CellJpgDecSubHandle>(subHandle);

	if (!subHandle_data)
	{
		return CELL_JPGDEC_ERROR_FATAL;
	}

	const u32 fd = subHandle_data->fd;
	const u64 fileSize = subHandle_data->fileSize;
	CellJpgDecInfo& current_info = subHandle_data->info;

	// Write the header to buffer
	std::unique_ptr<u8[]> buffer(new u8[fileSize]);

	switch (subHandle_data->src.srcSelect)
	{
	case CELL_JPGDEC_BUFFER:
		std::memcpy(buffer.get(), vm::base(subHandle_data->src.streamPtr), fileSize);
		break;

	case CELL_JPGDEC_FILE:
	{
		auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);
		file->file.seek(0);
		file->file.read(buffer.get(), fileSize);
		break;
	}
	default: break; // TODO
	}

	if (read_from_ptr<le_t<u32>>(buffer.get() + 0) != 0xE0FFD8FF || // Error: Not a valid SOI header
		read_from_ptr<u32>(buffer.get() + 6) != "JFIF"_u32)   // Error: Not a valid JFIF string
	{
		return CELL_JPGDEC_ERROR_HEADER;
	}

	u32 i = 4;

	if (i >= fileSize)
		return CELL_JPGDEC_ERROR_HEADER;

	u16 block_length = buffer[i] * 0xFF + buffer[i + 1];

	while (true)
	{
		i += block_length;                                  // Increase the file index to get to the next block
		if (i >= fileSize ||                                // Check to protect against segmentation faults
			buffer[i] != 0xFF)                              // Check that we are truly at the start of another block
		{
			return CELL_JPGDEC_ERROR_HEADER;
		}

		if (buffer[i + 1] == 0xC0)
			break;                                          // 0xFFC0 is the "Start of frame" marker which contains the file size

		i += 2;                                             // Skip the block marker
		block_length = buffer[i] * 0xFF + buffer[i + 1];    // Go to the next block
	}

	current_info.imageWidth    = buffer[i + 7] * 0x100 + buffer[i + 8];
	current_info.imageHeight   = buffer[i + 5] * 0x100 + buffer[i + 6];
	current_info.numComponents = 3; // Unimplemented
	current_info.colorSpace    = CELL_JPG_RGB;

	*info = current_info;

	return CELL_OK;
}

error_code cellJpgDecExtReadHeader()
{
	cellJpgDec.todo("cellJpgDecExtReadHeader()");
	return CELL_OK;
}

error_code cellJpgDecDecodeData(u32 mainHandle, u32 subHandle, vm::ptr<u8> data, vm::cptr<CellJpgDecDataCtrlParam> dataCtrlParam, vm::ptr<CellJpgDecDataOutInfo> dataOutInfo)
{
	cellJpgDec.trace("cellJpgDecDecodeData(mainHandle=0x%x, subHandle=0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x)", mainHandle, subHandle, data, dataCtrlParam, dataOutInfo);

	dataOutInfo->status = CELL_JPGDEC_DEC_STATUS_STOP;

	const auto subHandle_data = idm::get_unlocked<CellJpgDecSubHandle>(subHandle);

	if (!subHandle_data)
	{
		return CELL_JPGDEC_ERROR_FATAL;
	}

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	const CellJpgDecOutParam& current_outParam = subHandle_data->outParam;

	//Copy the JPG file to a buffer
	std::unique_ptr<u8[]> jpg(new u8[fileSize]);

	switch (subHandle_data->src.srcSelect)
	{
	case CELL_JPGDEC_BUFFER:
		std::memcpy(jpg.get(), vm::base(subHandle_data->src.streamPtr), fileSize);
		break;

	case CELL_JPGDEC_FILE:
	{
		auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);
		file->file.seek(0);
		file->file.read(jpg.get(), fileSize);
		break;
	}
	default: break; // TODO
	}

	//Decode JPG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width = 0, height = 0, actual_components = 0;
	auto image = std::unique_ptr<unsigned char,decltype(&::free)>
		(
			stbi_load_from_memory(jpg.get(), ::narrow<int>(fileSize), &width, &height, &actual_components, 4),
			&::free
		);

	if (!image)
		return CELL_JPGDEC_ERROR_STREAM_FORMAT;

	const bool flip = current_outParam.outputMode == CELL_JPGDEC_BOTTOM_TO_TOP;
	const int bytesPerLine = static_cast<int>(dataCtrlParam->outputBytesPerLine);
	usz image_size = width * height;

	switch(current_outParam.outputColorSpace)
	{
	case CELL_JPG_RGB:
	case CELL_JPG_RGBA:
	{
		const char nComponents = current_outParam.outputColorSpace == CELL_JPG_RGBA ? 4 : 3;
		image_size *= nComponents;
		if (bytesPerLine > width * nComponents || flip) //check if we need padding
		{
			const int linesize = std::min(bytesPerLine, width * nComponents);
			for (int i = 0; i < height; i++)
			{
				const int dstOffset = i * bytesPerLine;
				const int srcOffset = width * nComponents * (flip ? height - i - 1 : i);
				memcpy(&data[dstOffset], &image.get()[srcOffset], linesize);
			}
		}
		else
		{
			memcpy(data.get_ptr(), image.get(), image_size);
		}
		break;
	}
	case CELL_JPG_ARGB:
	{
		constexpr int nComponents = 4;
		image_size *= nComponents;
		if (bytesPerLine > width * nComponents || flip) //check if we need padding
		{
			//TODO: Find out if we can't do padding without an extra copy
			const int linesize = std::min(bytesPerLine, width * nComponents);
			std::vector<char> output(image_size);
			for (int i = 0; i < height; i++)
			{
				const int dstOffset = i * bytesPerLine;
				const int srcOffset = width * nComponents * (flip ? height - i - 1 : i);
				for (int j = 0; j < linesize; j += nComponents)
				{
					output[j + 0] = image.get()[srcOffset + j + 3];
					output[j + 1] = image.get()[srcOffset + j + 0];
					output[j + 2] = image.get()[srcOffset + j + 1];
					output[j + 3] = image.get()[srcOffset + j + 2];
				}
				std::memcpy(&data[dstOffset], output.data(), linesize);
			}
		}
		else
		{
			std::vector<u32> img(image_size);
			const u32* source_current = reinterpret_cast<const u32*>(image.get());
			u32* dest_current = img.data();
			for (u32 i = 0; i < image_size / nComponents; i++)
			{
				const u32 val = *source_current;
				*dest_current = (val >> 24) | (val << 8); // set alpha (A8) as leftmost byte
				source_current++;
				dest_current++;
			}
			std::memcpy(data.get_ptr(), img.data(), image_size);
		}
		break;
	}
	case CELL_JPG_GRAYSCALE:
	case CELL_JPG_YCbCr:
	case CELL_JPG_UPSAMPLE_ONLY:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_RGBA:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_ARGB:
		cellJpgDec.error("cellJpgDecDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace);
		break;

	default:
		return CELL_JPGDEC_ERROR_ARG;
	}

	dataOutInfo->status = CELL_JPGDEC_DEC_STATUS_FINISH;

	if (dataCtrlParam->outputBytesPerLine)
		dataOutInfo->outputLines = static_cast<u32>(image_size / dataCtrlParam->outputBytesPerLine);

	return CELL_OK;
}

error_code cellJpgDecExtDecodeData()
{
	cellJpgDec.todo("cellJpgDecExtDecodeData()");
	return CELL_OK;
}

error_code cellJpgDecSetParameter(u32 mainHandle, u32 subHandle, vm::cptr<CellJpgDecInParam> inParam, vm::ptr<CellJpgDecOutParam> outParam)
{
	cellJpgDec.trace("cellJpgDecSetParameter(mainHandle=0x%x, subHandle=0x%x, inParam=*0x%x, outParam=*0x%x)", mainHandle, subHandle, inParam, outParam);

	const auto subHandle_data = idm::get_unlocked<CellJpgDecSubHandle>(subHandle);

	if (!subHandle_data)
	{
		return CELL_JPGDEC_ERROR_FATAL;
	}

	CellJpgDecInfo& current_info = subHandle_data->info;
	CellJpgDecOutParam& current_outParam = subHandle_data->outParam;

	current_outParam.outputWidthByte  = (current_info.imageWidth * current_info.numComponents);
	current_outParam.outputWidth      = current_info.imageWidth;
	current_outParam.outputHeight     = current_info.imageHeight;
	current_outParam.outputColorSpace = inParam->outputColorSpace;

	switch (current_outParam.outputColorSpace)
	{
	case CELL_JPG_GRAYSCALE:               current_outParam.outputComponents = 1; break;

	case CELL_JPG_RGB:
	case CELL_JPG_YCbCr:                   current_outParam.outputComponents = 3; break;

	case CELL_JPG_UPSAMPLE_ONLY:           current_outParam.outputComponents = current_info.numComponents; break;

	case CELL_JPG_RGBA:
	case CELL_JPG_ARGB:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_RGBA:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_ARGB: current_outParam.outputComponents = 4; break;

	default: return CELL_JPGDEC_ERROR_ARG; // Not supported color space
	}

	current_outParam.outputMode     = inParam->outputMode;
	current_outParam.downScale      = inParam->downScale;
	current_outParam.useMemorySpace = 0; // Unimplemented

	*outParam = current_outParam;

	return CELL_OK;
}

error_code cellJpgDecExtSetParameter()
{
	cellJpgDec.todo("cellJpgDecExtSetParameter()");
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellJpgDec)("cellJpgDec", []()
{
	REG_FUNC(cellJpgDec, cellJpgDecCreate);
	REG_FUNC(cellJpgDec, cellJpgDecExtCreate);
	REG_FUNC(cellJpgDec, cellJpgDecOpen);
	REG_FUNC(cellJpgDec, cellJpgDecReadHeader);
	REG_FUNC(cellJpgDec, cellJpgDecSetParameter);
	REG_FUNC(cellJpgDec, cellJpgDecDecodeData);
	REG_FUNC(cellJpgDec, cellJpgDecClose);
	REG_FUNC(cellJpgDec, cellJpgDecDestroy);

	REG_FUNC(cellJpgDec, cellJpgDecExtOpen);
	REG_FUNC(cellJpgDec, cellJpgDecExtReadHeader);
	REG_FUNC(cellJpgDec, cellJpgDecExtSetParameter);
	REG_FUNC(cellJpgDec, cellJpgDecExtDecodeData);
});
