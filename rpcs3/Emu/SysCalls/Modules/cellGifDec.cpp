#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellGifDec.h"

#include "stblib/stb_image.h"
#include "stblib/stb_image.c" // (TODO: Should we put this elsewhere?)

void cellGifDec_init();
Module cellGifDec(0xf010, cellGifDec_init);

int cellGifDecCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam)
{
	UNIMPLEMENTED_FUNC(cellGifDec);
	return CELL_OK;
}

int cellGifDecExtCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam, u32 extThreadInParam, u32 extThreadOutParam)
{
	UNIMPLEMENTED_FUNC(cellGifDec);
	return CELL_OK;
}

int cellGifDecOpen(u32 mainHandle, mem32_t subHandle, const mem_ptr_t<CellGifDecSrc> src, mem_ptr_t<CellGifDecOpnInfo> openInfo)
{
	if (!subHandle.IsGood() || !src.IsGood())
		return CELL_GIFDEC_ERROR_ARG;
	/*
	vfsStream* stream;

	switch(src->srcSelect)
	{
	case CELL_GIFDEC_FILE:
		stream = Emu.GetVFS().Open(src->fileName.GetString(), vfsRead);
		stream->Seek(src->fileOffset);
		src->fileSize;
	break;

	case CELL_GIFDEC_BUFFER:
		if(src->streamSize < 5)
			return CELL_GIFDEC_ERROR_ARG;

		stream = new vfsStreamMemory(src->streamPtr.GetAddr(), src->streamSize);
	break;

	default:
		return CELL_GIFDEC_ERROR_ARG;
	}

	if(!stream->IsOpened())
	{
		return CELL_GIFDEC_ERROR_OPEN_FILE;
	}
	*/

	CellGifDecSubHandle *current_subHandle = new CellGifDecSubHandle;

	// Get file descriptor
	MemoryAllocator<be_t<u32>> fd;
	int ret = cellFsOpen(src->fileName, 0, fd, NULL, 0);
	current_subHandle->fd = fd->ToLE();
	if(ret != CELL_OK) return CELL_GIFDEC_ERROR_OPEN_FILE;

	// Get size of file
	MemoryAllocator<CellFsStat> sb; // Alloc a CellFsStat struct
	ret = cellFsFstat(current_subHandle->fd, sb.GetAddr());
	if(ret != CELL_OK) return ret;
	current_subHandle->fileSize = sb->st_size; // Get CellFsStat.st_size

	// From now, every u32 subHandle argument is a pointer to a CellPngDecSubHandle struct.
	subHandle = cellGifDec.GetNewId(current_subHandle);

	return CELL_OK;
}

int cellGifDecReadHeader(u32 mainHandle, u32 subHandle, mem_ptr_t<CellGifDecInfo> info)
{
	if (!info.IsGood())
		return CELL_GIFDEC_ERROR_ARG;

	CellGifDecSubHandle* subHandle_data;
	if(!cellGifDec.CheckId(subHandle, subHandle_data))
		return CELL_GIFDEC_ERROR_FATAL;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	CellGifDecInfo& current_info = subHandle_data->info;
	
	//Write the header to buffer
	MemoryAllocator<u8> buffer(13); // Alloc buffer for GIF header
	MemoryAllocator<be_t<u64>> pos, nread;

	cellFsLseek(fd, 0, CELL_SEEK_SET, pos);
	cellFsRead(fd, buffer.GetAddr(), buffer.GetSize(), nread);

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

int cellGifDecSetParameter(u32 mainHandle, u32 subHandle, const mem_ptr_t<CellGifDecInParam> inParam, mem_ptr_t<CellGifDecOutParam> outParam)
{
	if (!inParam.IsGood() || !outParam.IsGood())
		return CELL_GIFDEC_ERROR_ARG;

	CellGifDecSubHandle* subHandle_data;
	if(!cellGifDec.CheckId(subHandle, subHandle_data))
		return CELL_GIFDEC_ERROR_FATAL;

	CellGifDecInfo& current_info = subHandle_data->info;
	CellGifDecOutParam& current_outParam = subHandle_data->outParam;

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

int cellGifDecDecodeData(u32 mainHandle, u32 subHandle, mem8_ptr_t data, const mem_ptr_t<CellGifDecDataCtrlParam> dataCtrlParam, mem_ptr_t<CellGifDecDataOutInfo> dataOutInfo)
{
	if (!data.IsGood() || !dataCtrlParam.IsGood() || !dataOutInfo.IsGood())
		return CELL_GIFDEC_ERROR_ARG;

	dataOutInfo->status = CELL_GIFDEC_DEC_STATUS_STOP;

	CellGifDecSubHandle* subHandle_data;
	if(!cellGifDec.CheckId(subHandle, subHandle_data))
		return CELL_GIFDEC_ERROR_FATAL;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	const CellGifDecOutParam& current_outParam = subHandle_data->outParam; 

	//Copy the GIF file to a buffer
	MemoryAllocator<unsigned char> gif(fileSize);
	MemoryAllocator<u64> pos, nread;
	cellFsLseek(fd, 0, CELL_SEEK_SET, pos);
	cellFsRead(fd, gif.GetAddr(), gif.GetSize(), nread);

	//Decode GIF file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	std::shared_ptr<unsigned char> image(stbi_load_from_memory(gif, fileSize, &width, &height, &actual_components, 4));
	if (!image)	return CELL_GIFDEC_ERROR_STREAM_FORMAT;

	uint image_size = width * height * 4;

	switch(current_outParam.outputColorSpace)
	{
	case CELL_GIFDEC_RGBA:
		Memory.CopyFromReal(data.GetAddr(), image.get(), image_size);
	break;

	case CELL_GIFDEC_ARGB:
		for(uint i = 0; i < image_size; i+=4)
		{
			data += image.get()[i+3];
			data += image.get()[i+0];
			data += image.get()[i+1];
			data += image.get()[i+2];
		}
	break;

	default:
		return CELL_GIFDEC_ERROR_ARG;
	}

	dataOutInfo->status = CELL_GIFDEC_DEC_STATUS_FINISH;
	dataOutInfo->recordType = CELL_GIFDEC_RECORD_TYPE_IMAGE_DESC;

	return CELL_OK;
}

int cellGifDecClose(u32 mainHandle, u32 subHandle)
{
	CellGifDecSubHandle* subHandle_data;
	if(!cellGifDec.CheckId(subHandle, subHandle_data))
		return CELL_GIFDEC_ERROR_FATAL;

	cellFsClose(subHandle_data->fd);
	Emu.GetIdManager().RemoveID(subHandle);

	return CELL_OK;
}

int cellGifDecDestroy(u32 mainHandle)
{
	UNIMPLEMENTED_FUNC(cellGifDec);
	return CELL_OK;
}

void cellGifDec_init()
{
	cellGifDec.AddFunc(0xb60d42a5, cellGifDecCreate);
	cellGifDec.AddFunc(0x4711cb7f, cellGifDecExtCreate);
	cellGifDec.AddFunc(0x75745079, cellGifDecOpen);
	cellGifDec.AddFunc(0xf0da95de, cellGifDecReadHeader);
	cellGifDec.AddFunc(0x41a90dc4, cellGifDecSetParameter);
	cellGifDec.AddFunc(0x44b1bc61, cellGifDecDecodeData);
	cellGifDec.AddFunc(0x116a7da9, cellGifDecClose);
	cellGifDec.AddFunc(0xe74b2cb1, cellGifDecDestroy);
	
	/*cellGifDec.AddFunc(0x17fb83c1, cellGifDecExtOpen);
	cellGifDec.AddFunc(0xe53f91f2, cellGifDecExtReadHeader);
	cellGifDec.AddFunc(0x95cae771, cellGifDecExtSetParameter);
	cellGifDec.AddFunc(0x02e7e03e, cellGifDecExtDecodeData);*/
}
