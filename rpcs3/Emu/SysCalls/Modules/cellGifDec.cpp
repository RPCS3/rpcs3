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

s32 cellGifDecCreate(
	vm::ptr<CellGifDecMainHandle> mainHandle,
	vm::cptr<CellGifDecThreadInParam> threadInParam,
	vm::ptr<CellGifDecThreadOutParam> threadOutParam)
{
	UNIMPLEMENTED_FUNC(cellGifDec);
	return CELL_OK;
}

s32 cellGifDecExtCreate(
	vm::ptr<CellGifDecMainHandle> mainHandle,
	vm::cptr<CellGifDecThreadInParam> threadInParam,
	vm::ptr<CellGifDecThreadOutParam> threadOutParam,
	vm::cptr<CellGifDecExtThreadInParam> extThreadInParam,
	vm::ptr<CellGifDecExtThreadOutParam> extThreadOutParam)
{
	UNIMPLEMENTED_FUNC(cellGifDec);
	return CELL_OK;
}

s32 cellGifDecOpen(
	CellGifDecMainHandle mainHandle,
	vm::ptr<CellGifDecSubHandle> subHandle,
	vm::cptr<CellGifDecSrc> src,
	vm::ptr<CellGifDecOpnInfo> openInfo)
{
	cellGifDec.Warning("cellGifDecOpen(mainHandle=0x%x, subHandle=*0x%x, src=*0x%x, openInfo=*0x%x)", mainHandle, subHandle, src, openInfo);

	auto current_subHandle = std::make_shared<GifStream>();
	current_subHandle->fd = 0;
	current_subHandle->src = *src;

	switch(src->srcSelect.data())
	{
	case se32(CELL_GIFDEC_BUFFER):
		current_subHandle->fileSize = src->streamSize;
		break;

	case se32(CELL_GIFDEC_FILE):
	{
		// Get file descriptor and size
		std::shared_ptr<vfsStream> file_s(Emu.GetVFS().OpenFile(src->fileName.get_ptr(), vfsRead));
		if (!file_s) return CELL_GIFDEC_ERROR_OPEN_FILE;

		current_subHandle->fd = Emu.GetIdManager().make<lv2_file_t>(file_s, 0, 0);
		current_subHandle->fileSize = file_s->GetSize();
		break;
	}
	}

	// From now, every u32 subHandle argument is a pointer to a CellGifDecSubHandle struct.
	*subHandle = Emu.GetIdManager().add(std::move(current_subHandle));

	return CELL_OK;
}

s32 cellGifDecReadHeader(
	CellGifDecMainHandle mainHandle,
	CellGifDecSubHandle subHandle,
	vm::ptr<CellGifDecInfo> info)
{
	cellGifDec.Warning("cellGifDecReadHeader(mainHandle=0x%x, subHandle=0x%x, info=*0x%x)", mainHandle, subHandle, info);

	const auto subHandle_data = Emu.GetIdManager().get<GifStream>(subHandle);

	if (!subHandle_data)
	{
		return CELL_GIFDEC_ERROR_FATAL;
	}

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	CellGifDecInfo& current_info = subHandle_data->info;
	
	//Write the header to buffer
	vm::var<u8[13]> buffer; // Alloc buffer for GIF header

	switch(subHandle_data->src.srcSelect.data())
	{
	case se32(CELL_GIFDEC_BUFFER):
		memmove(buffer.begin(), subHandle_data->src.streamPtr.get_ptr(), buffer.size());
		break;

	case se32(CELL_GIFDEC_FILE):
	{
		auto file = Emu.GetIdManager().get<lv2_file_t>(fd);
		file->file->Seek(0);
		file->file->Read(buffer.begin(), buffer.size());
		break;
	}
	}

	if (*buffer.To<be_t<u32>>(0) != 0x47494638 ||
		(*buffer.To<u16>(4) != 0x6139 && *buffer.To<u16>(4) != 0x6137)) // Error: The first 6 bytes are not a valid GIF signature
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

s32 cellGifDecSetParameter(
	CellGifDecMainHandle mainHandle,
	CellGifDecSubHandle subHandle,
	vm::cptr<CellGifDecInParam> inParam,
	vm::ptr<CellGifDecOutParam> outParam)
{
	cellGifDec.Warning("cellGifDecSetParameter(mainHandle=0x%x, subHandle=0x%x, inParam=*0x%x, outParam=*0x%x)", mainHandle, subHandle, inParam, outParam);

	const auto subHandle_data = Emu.GetIdManager().get<GifStream>(subHandle);

	if (!subHandle_data)
	{
		return CELL_GIFDEC_ERROR_FATAL;
	}

	CellGifDecInfo& current_info = subHandle_data->info;
	CellGifDecOutParam& current_outParam = subHandle_data->outParam;

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

s32 cellGifDecDecodeData(
	CellGifDecMainHandle mainHandle,
	CellGifDecSubHandle subHandle,
	vm::ptr<u8> data,
	vm::cptr<CellGifDecDataCtrlParam> dataCtrlParam,
	vm::ptr<CellGifDecDataOutInfo> dataOutInfo)
{
	cellGifDec.Warning("cellGifDecDecodeData(mainHandle=0x%x, subHandle=0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x)", mainHandle, subHandle, data, dataCtrlParam, dataOutInfo);

	dataOutInfo->status = CELL_GIFDEC_DEC_STATUS_STOP;

	const auto subHandle_data = Emu.GetIdManager().get<GifStream>(subHandle);

	if (!subHandle_data)
	{
		return CELL_GIFDEC_ERROR_FATAL;
	}

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	const CellGifDecOutParam& current_outParam = subHandle_data->outParam; 

	//Copy the GIF file to a buffer
	vm::var<unsigned char[]> gif((u32)fileSize);

	switch(subHandle_data->src.srcSelect.data())
	{
	case se32(CELL_GIFDEC_BUFFER):
		memmove(gif.begin(), subHandle_data->src.streamPtr.get_ptr(), gif.size());
		break;

	case se32(CELL_GIFDEC_FILE):
	{
		auto file = Emu.GetIdManager().get<lv2_file_t>(fd);
		file->file->Seek(0);
		file->file->Read(gif.ptr(), gif.size());
		break;
	}
	}

	//Decode GIF file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	auto image = std::unique_ptr<unsigned char,decltype(&::free)>
		(
			stbi_load_from_memory(gif.ptr(), (s32)fileSize, &width, &height, &actual_components, 4),
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

s32 cellGifDecClose(CellGifDecMainHandle mainHandle, CellGifDecSubHandle subHandle)
{
	cellGifDec.Warning("cellGifDecClose(mainHandle=0x%x, subHandle=0x%x)", mainHandle, subHandle);

	const auto subHandle_data = Emu.GetIdManager().get<GifStream>(subHandle);

	if (!subHandle_data)
	{
		return CELL_GIFDEC_ERROR_FATAL;
	}

	Emu.GetIdManager().remove<lv2_file_t>(subHandle_data->fd);
	Emu.GetIdManager().remove<CellGifDecSubHandle>(subHandle);

	return CELL_OK;
}

s32 cellGifDecDestroy(CellGifDecMainHandle mainHandle)
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
	
	/*REG_FUNC(cellGifDec, cellGifDecExtOpen);
	REG_FUNC(cellGifDec, cellGifDecExtReadHeader);
	REG_FUNC(cellGifDec, cellGifDecExtSetParameter);
	REG_FUNC(cellGifDec, cellGifDecExtDecodeData);*/
});
