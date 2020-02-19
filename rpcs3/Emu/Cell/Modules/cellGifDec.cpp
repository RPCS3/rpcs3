#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

// STB_IMAGE_IMPLEMENTATION is already defined in stb_image.cpp
#include <stb_image.h>

#include "Emu/Cell/lv2/sys_fs.h"
#include "cellGifDec.h"

LOG_CHANNEL(cellGifDec);

// cellGifDec aliases (only for cellGifDec.cpp)
using PPMainHandle = vm::pptr<GifDecoder>;
using PMainHandle = vm::ptr<GifDecoder>;
using PThreadInParam = vm::cptr<CellGifDecThreadInParam>;
using PThreadOutParam = vm::ptr<CellGifDecThreadOutParam>;
using PExtThreadInParam = vm::cptr<CellGifDecExtThreadInParam>;
using PExtThreadOutParam = vm::ptr<CellGifDecExtThreadOutParam>;
using PPSubHandle = vm::pptr<GifStream>;
using PSubHandle = vm::ptr<GifStream>;
using PSrc = vm::cptr<CellGifDecSrc>;
using POpenInfo = vm::ptr<CellGifDecOpnInfo>;
using PInfo = vm::ptr<CellGifDecInfo>;
using PInParam = vm::cptr<CellGifDecInParam>;
using POutParam = vm::ptr<CellGifDecOutParam>;
using PDataCtrlParam = vm::cptr<CellGifDecDataCtrlParam>;
using PDataOutInfo = vm::ptr<CellGifDecDataOutInfo>;

s32 cellGifDecCreate(PPMainHandle mainHandle, PThreadInParam threadInParam, PThreadOutParam threadOutParam)
{
	UNIMPLEMENTED_FUNC(cellGifDec);
	return CELL_OK;
}

s32 cellGifDecExtCreate(PPMainHandle mainHandle, PThreadInParam threadInParam, PThreadOutParam threadOutParam, PExtThreadInParam extThreadInParam, PExtThreadOutParam extThreadOutParam)
{
	UNIMPLEMENTED_FUNC(cellGifDec);
	return CELL_OK;
}

s32 cellGifDecOpen(PMainHandle mainHandle, PPSubHandle subHandle, PSrc src, POpenInfo openInfo)
{
	cellGifDec.warning("cellGifDecOpen(mainHandle=*0x%x, subHandle=**0x%x, src=*0x%x, openInfo=*0x%x)", mainHandle, subHandle, src, openInfo);

	GifStream current_subHandle;
	current_subHandle.fd = 0;
	current_subHandle.src = *src;

	switch (src->srcSelect)
	{
	case CELL_GIFDEC_BUFFER:
		current_subHandle.fileSize = src->streamSize;
		break;

	case CELL_GIFDEC_FILE:
	{
		// Get file descriptor and size
		fs::file file_s(vfs::get(src->fileName.get_ptr()));
		if (!file_s) return CELL_GIFDEC_ERROR_OPEN_FILE;

		current_subHandle.fileSize = file_s.size();
		current_subHandle.fd = idm::make<lv2_fs_object, lv2_file>(src->fileName.get_ptr(), std::move(file_s), 0, 0);
		break;
	}
	}

	subHandle->set(vm::alloc(sizeof(GifStream), vm::main));

	**subHandle = current_subHandle;

	return CELL_OK;
}

s32 cellGifDecExtOpen()
{
	cellGifDec.todo("cellGifDecExtOpen()");
	return CELL_OK;
}

s32 cellGifDecReadHeader(PMainHandle mainHandle, PSubHandle subHandle, PInfo info)
{
	cellGifDec.warning("cellGifDecReadHeader(mainHandle=*0x%x, subHandle=*0x%x, info=*0x%x)", mainHandle, subHandle, info);

	const u32& fd = subHandle->fd;
	const u64& fileSize = subHandle->fileSize;
	CellGifDecInfo& current_info = subHandle->info;

	// Write the header to buffer
	u8 buffer[13];

	switch (subHandle->src.srcSelect)
	{
	case CELL_GIFDEC_BUFFER:
		std::memcpy(buffer, subHandle->src.streamPtr.get_ptr(), sizeof(buffer));
		break;

	case CELL_GIFDEC_FILE:
	{
		auto file = idm::get<lv2_fs_object, lv2_file>(fd);
		file->file.seek(0);
		file->file.read(buffer, sizeof(buffer));
		break;
	}
	}

	if (*reinterpret_cast<be_t<u32>*>(buffer) != 0x47494638u ||
		(*reinterpret_cast<le_t<u16>*>(buffer + 4) != 0x6139u && *reinterpret_cast<le_t<u16>*>(buffer + 4) != 0x6137u)) // Error: The first 6 bytes are not a valid GIF signature
	{
		return CELL_GIFDEC_ERROR_STREAM_FORMAT; // Surprisingly there is no error code related with headerss
	}

	u8 packedField = buffer[10];
	current_info.SWidth                  = buffer[6] + buffer[7] * 0x100;
	current_info.SHeight                 = buffer[8] + buffer[9] * 0x100;
	current_info.SGlobalColorTableFlag   = packedField >> 7;
	current_info.SColorResolution        = ((packedField >> 4) & 7)+1;
	current_info.SSortFlag               = (packedField >> 3) & 1;
	current_info.SSizeOfGlobalColorTable = (packedField & 7)+1;
	current_info.SBackGroundColor        = buffer[11];
	current_info.SPixelAspectRatio       = buffer[12];

	*info = current_info;

	return CELL_OK;
}

s32 cellGifDecExtReadHeader()
{
	cellGifDec.todo("cellGifDecExtReadHeader()");
	return CELL_OK;
}

s32 cellGifDecSetParameter(PMainHandle mainHandle, PSubHandle subHandle, PInParam inParam, POutParam outParam)
{
	cellGifDec.warning("cellGifDecSetParameter(mainHandle=*0x%x, subHandle=*0x%x, inParam=*0x%x, outParam=*0x%x)", mainHandle, subHandle, inParam, outParam);

	CellGifDecInfo& current_info = subHandle->info;
	CellGifDecOutParam& current_outParam = subHandle->outParam;

	current_outParam.outputWidthByte  = (current_info.SWidth * current_info.SColorResolution * 3)/8;
	current_outParam.outputWidth      = current_info.SWidth;
	current_outParam.outputHeight     = current_info.SHeight;
	current_outParam.outputColorSpace = inParam->colorSpace;
	switch (current_outParam.outputColorSpace)
	{
	case CELL_GIFDEC_RGBA:
	case CELL_GIFDEC_ARGB: current_outParam.outputComponents = 4; break;
	default: return CELL_GIFDEC_ERROR_ARG; // Not supported color space
	}
	current_outParam.outputBitDepth	= 0;   // Unimplemented
	current_outParam.useMemorySpace	= 0;   // Unimplemented

	*outParam = current_outParam;

	return CELL_OK;
}

s32 cellGifDecExtSetParameter()
{
	cellGifDec.todo("cellGifDecExtSetParameter()");
	return CELL_OK;
}

s32 cellGifDecDecodeData(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<u8> data, PDataCtrlParam dataCtrlParam, PDataOutInfo dataOutInfo)
{
	cellGifDec.warning("cellGifDecDecodeData(mainHandle=*0x%x, subHandle=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x)", mainHandle, subHandle, data, dataCtrlParam, dataOutInfo);

	dataOutInfo->status = CELL_GIFDEC_DEC_STATUS_STOP;

	const u32 fd = subHandle->fd;
	const u64 fileSize = subHandle->fileSize;
	const CellGifDecOutParam& current_outParam = subHandle->outParam;

	//Copy the GIF file to a buffer
	std::unique_ptr<u8[]> gif(new u8[fileSize]);

	switch (subHandle->src.srcSelect)
	{
	case CELL_GIFDEC_BUFFER:
		std::memcpy(gif.get(), subHandle->src.streamPtr.get_ptr(), fileSize);
		break;

	case CELL_GIFDEC_FILE:
	{
		auto file = idm::get<lv2_fs_object, lv2_file>(fd);
		file->file.seek(0);
		file->file.read(gif.get(), fileSize);
		break;
	}
	}

	//Decode GIF file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	auto image = std::unique_ptr<unsigned char,decltype(&::free)>
		(
			stbi_load_from_memory(gif.get(), ::narrow<int>(fileSize), &width, &height, &actual_components, 4),
			&::free
		);

	if (!image)
		return CELL_GIFDEC_ERROR_STREAM_FORMAT;

	const int bytesPerLine = static_cast<int>(dataCtrlParam->outputBytesPerLine);
	const char nComponents = 4;
	uint image_size = width * height * nComponents;

	switch(current_outParam.outputColorSpace)
	{
	case CELL_GIFDEC_RGBA:
	{
		if (bytesPerLine > width * nComponents) // Check if we need padding
		{
			const int linesize = std::min(bytesPerLine, width * nComponents);
			for (int i = 0; i < height; i++)
			{
				const int dstOffset = i * bytesPerLine;
				const int srcOffset = width * nComponents * i;
				memcpy(&data[dstOffset], &image.get()[srcOffset], linesize);
			}
		}
		else
		{
			memcpy(data.get_ptr(), image.get(), image_size);
		}
	}
	break;

	case CELL_GIFDEC_ARGB:
	{
		if (bytesPerLine > width * nComponents) // Check if we need padding
		{
			//TODO: find out if we can't do padding without an extra copy
			const int linesize = std::min(bytesPerLine, width * nComponents);
			const auto output = std::make_unique<char[]>(linesize);
			for (int i = 0; i < height; i++)
			{
				const int dstOffset = i * bytesPerLine;
				const int srcOffset = width * nComponents * i;
				for (int j = 0; j < linesize; j += nComponents)
				{
					output[j + 0] = image.get()[srcOffset + j + 3];
					output[j + 1] = image.get()[srcOffset + j + 0];
					output[j + 2] = image.get()[srcOffset + j + 1];
					output[j + 3] = image.get()[srcOffset + j + 2];
				}
				std::memcpy(&data[dstOffset], output.get(), linesize);
			}
		}
		else
		{
			const auto img = std::make_unique<uint[]>(image_size);
			uint* source_current = reinterpret_cast<uint*>(image.get());
			uint* dest_current = img.get();
			for (uint i = 0; i < image_size / nComponents; i++)
			{
				uint val = *source_current;
				*dest_current = (val >> 24) | (val << 8); // set alpha (A8) as leftmost byte
				source_current++;
				dest_current++;
			}
			std::memcpy(data.get_ptr(), img.get(), image_size);
		}
	}
	break;

	default:
		return CELL_GIFDEC_ERROR_ARG;
	}

	dataOutInfo->status = CELL_GIFDEC_DEC_STATUS_FINISH;
	dataOutInfo->recordType = CELL_GIFDEC_RECORD_TYPE_IMAGE_DESC;

	return CELL_OK;
}

s32 cellGifDecExtDecodeData()
{
	cellGifDec.todo("cellGifDecExtDecodeData()");
	return CELL_OK;
}

s32 cellGifDecClose(PMainHandle mainHandle, PSubHandle subHandle)
{
	cellGifDec.warning("cellGifDecClose(mainHandle=*0x%x, subHandle=*0x%x)", mainHandle, subHandle);

	idm::remove<lv2_fs_object, lv2_file>(subHandle->fd);

	vm::dealloc(subHandle.addr());

	return CELL_OK;
}

s32 cellGifDecDestroy(PMainHandle mainHandle)
{
	UNIMPLEMENTED_FUNC(cellGifDec);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellGifDec)("cellGifDec", []()
{
	REG_FUNC(cellGifDec, cellGifDecCreate);
	REG_FUNC(cellGifDec, cellGifDecExtCreate);
	REG_FUNC(cellGifDec, cellGifDecOpen);
	REG_FUNC(cellGifDec, cellGifDecReadHeader);
	REG_FUNC(cellGifDec, cellGifDecSetParameter);
	REG_FUNC(cellGifDec, cellGifDecDecodeData);
	REG_FUNC(cellGifDec, cellGifDecClose);
	REG_FUNC(cellGifDec, cellGifDecDestroy);

	REG_FUNC(cellGifDec, cellGifDecExtOpen);
	REG_FUNC(cellGifDec, cellGifDecExtReadHeader);
	REG_FUNC(cellGifDec, cellGifDecExtSetParameter);
	REG_FUNC(cellGifDec, cellGifDecExtDecodeData);
});
