#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"

extern "C"
{
#include "stblib/stb_image.h"
#include "stblib/stb_image.c"
}

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFileBase.h"
#include "Emu/SysCalls/lv2/sys_fs.h"

#include "cellGifDec.h"

extern Module cellGifDec;

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
	cellGifDec.Warning("cellGifDecOpen(mainHandle=*0x%x, subHandle=**0x%x, src=*0x%x, openInfo=*0x%x)", mainHandle, subHandle, src, openInfo);

	GifStream current_subHandle;
	current_subHandle.fd = 0;
	current_subHandle.src = *src;

	switch (src->srcSelect.value())
	{
	case CELL_GIFDEC_BUFFER:
		current_subHandle.fileSize = src->streamSize;
		break;

	case CELL_GIFDEC_FILE:
	{
		// Get file descriptor and size
		std::shared_ptr<vfsStream> file_s(Emu.GetVFS().OpenFile(src->fileName.get_ptr(), vfsRead));
		if (!file_s) return CELL_GIFDEC_ERROR_OPEN_FILE;

		current_subHandle.fd = idm::make<lv2_file_t>(file_s, 0, 0);
		current_subHandle.fileSize = file_s->GetSize();
		break;
	}
	}

	subHandle->set(vm::alloc(sizeof32(GifStream), vm::main));

	**subHandle = current_subHandle;

	return CELL_OK;
}

s32 cellGifDecExtOpen()
{
	throw EXCEPTION("");
}

s32 cellGifDecReadHeader(PMainHandle mainHandle, PSubHandle subHandle, PInfo info)
{
	cellGifDec.Warning("cellGifDecReadHeader(mainHandle=*0x%x, subHandle=*0x%x, info=*0x%x)", mainHandle, subHandle, info);

	const u32& fd = subHandle->fd;
	const u64& fileSize = subHandle->fileSize;
	CellGifDecInfo& current_info = subHandle->info;
	
	// Write the header to buffer
	u8 buffer[13];

	switch (subHandle->src.srcSelect.value())
	{
	case CELL_GIFDEC_BUFFER:
		std::memcpy(buffer, subHandle->src.streamPtr.get_ptr(), sizeof(buffer));
		break;

	case CELL_GIFDEC_FILE:
	{
		auto file = idm::get<lv2_file_t>(fd);
		file->file->Seek(0);
		file->file->Read(buffer, sizeof(buffer));
		break;
	}
	}

	if (*(be_t<u32>*)buffer != 0x47494638 ||
		(*(le_t<u16>*)(buffer + 4) != 0x6139 && *(le_t<u16>*)(buffer + 4) != 0x6137)) // Error: The first 6 bytes are not a valid GIF signature
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
	throw EXCEPTION("");
}

s32 cellGifDecSetParameter(PMainHandle mainHandle, PSubHandle subHandle, PInParam inParam, POutParam outParam)
{
	cellGifDec.Warning("cellGifDecSetParameter(mainHandle=*0x%x, subHandle=*0x%x, inParam=*0x%x, outParam=*0x%x)", mainHandle, subHandle, inParam, outParam);

	CellGifDecInfo& current_info = subHandle->info;
	CellGifDecOutParam& current_outParam = subHandle->outParam;

	current_outParam.outputWidthByte  = (current_info.SWidth * current_info.SColorResolution * 3)/8;
	current_outParam.outputWidth      = current_info.SWidth;
	current_outParam.outputHeight     = current_info.SHeight;
	current_outParam.outputColorSpace = inParam->colorSpace;
	switch ((u32)current_outParam.outputColorSpace)
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
	throw EXCEPTION("");
}

s32 cellGifDecDecodeData(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<u8> data, PDataCtrlParam dataCtrlParam, PDataOutInfo dataOutInfo)
{
	cellGifDec.Warning("cellGifDecDecodeData(mainHandle=*0x%x, subHandle=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x)", mainHandle, subHandle, data, dataCtrlParam, dataOutInfo);

	dataOutInfo->status = CELL_GIFDEC_DEC_STATUS_STOP;

	const u32& fd = subHandle->fd;
	const u64& fileSize = subHandle->fileSize;
	const CellGifDecOutParam& current_outParam = subHandle->outParam; 

	//Copy the GIF file to a buffer
	std::unique_ptr<u8[]> gif(new u8[fileSize]);

	switch (subHandle->src.srcSelect.value())
	{
	case CELL_GIFDEC_BUFFER:
		std::memcpy(gif.get(), subHandle->src.streamPtr.get_ptr(), fileSize);
		break;

	case CELL_GIFDEC_FILE:
	{
		auto file = idm::get<lv2_file_t>(fd);
		file->file->Seek(0);
		file->file->Read(gif.get(), fileSize);
		break;
	}
	}

	//Decode GIF file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	auto image = std::unique_ptr<unsigned char,decltype(&::free)>
		(
			stbi_load_from_memory(gif.get(), (s32)fileSize, &width, &height, &actual_components, 4),
			&::free
		);

	if (!image)
		return CELL_GIFDEC_ERROR_STREAM_FORMAT;

	const int bytesPerLine = (u32)dataCtrlParam->outputBytesPerLine;
	const char nComponents = 4;
	uint image_size = width * height * nComponents;

	switch((u32)current_outParam.outputColorSpace)
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
			char *output = (char *) malloc(linesize);
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
				memcpy(&data[dstOffset], output, linesize);
			}
			free(output);
		}
		else
		{
			uint* img = (uint*)new char[image_size];
			uint* source_current = (uint*)&(image.get()[0]);
			uint* dest_current = img;
			for (uint i = 0; i < image_size / nComponents; i++) 
			{
				uint val = *source_current;
				*dest_current = (val >> 24) | (val << 8); // set alpha (A8) as leftmost byte
				source_current++;
				dest_current++;
			}
			memcpy(data.get_ptr(), img, image_size); 
			delete[] img;
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
	throw EXCEPTION("");
}

s32 cellGifDecClose(PMainHandle mainHandle, PSubHandle subHandle)
{
	cellGifDec.Warning("cellGifDecClose(mainHandle=*0x%x, subHandle=*0x%x)", mainHandle, subHandle);

	idm::remove<lv2_file_t>(subHandle->fd);

	vm::dealloc(subHandle.addr());

	return CELL_OK;
}

s32 cellGifDecDestroy(PMainHandle mainHandle)
{
	UNIMPLEMENTED_FUNC(cellGifDec);
	return CELL_OK;
}

Module cellGifDec("cellGifDec", []()
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
