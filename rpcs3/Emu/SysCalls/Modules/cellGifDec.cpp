#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/SysCalls.h"
#include "cellGifDec.h"

#include "stblib/stb_image.h"
#include "stblib/stb_image.c" // (TODO: Should we put this elsewhere?)

//void cellGifDec_init();
//Module cellGifDec(0xf010, cellGifDec_init);
Module *cellGifDec = nullptr;

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
	
	CellGifDecSubHandle *current_subHandle = new CellGifDecSubHandle;
	current_subHandle->fd = 0;
	current_subHandle->src = *src;

	switch(src->srcSelect.ToBE())
	{
	case se32(CELL_GIFDEC_BUFFER):
		current_subHandle->fileSize = src->streamSize.ToLE();
		break;

	case se32(CELL_GIFDEC_FILE):
		// Get file descriptor
		vm::var<be_t<u32>> fd;
		int ret = cellFsOpen(src->fileName, 0, fd.addr(), 0, 0);
		current_subHandle->fd = fd->ToLE();
		if (ret != CELL_OK) return CELL_GIFDEC_ERROR_OPEN_FILE;

		// Get size of file
		vm::var<CellFsStat> sb; // Alloc a CellFsStat struct
		ret = cellFsFstat(current_subHandle->fd, sb.addr());
		if (ret != CELL_OK) return ret;
		current_subHandle->fileSize = sb->st_size; // Get CellFsStat.st_size
		break;
	}

	// From now, every u32 subHandle argument is a pointer to a CellGifDecSubHandle struct.
	subHandle = cellGifDec->GetNewId(current_subHandle);

	return CELL_OK;
}

int cellGifDecReadHeader(u32 mainHandle, u32 subHandle, mem_ptr_t<CellGifDecInfo> info)
{
	if (!info.IsGood())
		return CELL_GIFDEC_ERROR_ARG;

	CellGifDecSubHandle* subHandle_data;
	if(!cellGifDec->CheckId(subHandle, subHandle_data))
		return CELL_GIFDEC_ERROR_FATAL;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	CellGifDecInfo& current_info = subHandle_data->info;
	
	//Write the header to buffer
	vm::var<u8[13]> buffer; // Alloc buffer for GIF header
	vm::var<be_t<u64>> pos, nread;

	switch(subHandle_data->src.srcSelect.ToBE())
	{
	case se32(CELL_GIFDEC_BUFFER):
		if (!Memory.Copy(buffer.addr(), subHandle_data->src.streamPtr.ToLE(), buffer.size())) {
			cellGifDec->Error("cellGifDecReadHeader() failed ()");
			return CELL_EFAULT;
		}
		break;

	case se32(CELL_GIFDEC_FILE):
		cellFsLseek(fd, 0, CELL_SEEK_SET, pos.addr());
		cellFsRead(fd, buffer.addr(), buffer.size(), nread.addr());
		break;
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

int cellGifDecSetParameter(u32 mainHandle, u32 subHandle, const mem_ptr_t<CellGifDecInParam> inParam, mem_ptr_t<CellGifDecOutParam> outParam)
{
	if (!inParam.IsGood() || !outParam.IsGood())
		return CELL_GIFDEC_ERROR_ARG;

	CellGifDecSubHandle* subHandle_data;
	if(!cellGifDec->CheckId(subHandle, subHandle_data))
		return CELL_GIFDEC_ERROR_FATAL;

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

int cellGifDecDecodeData(u32 mainHandle, u32 subHandle, mem8_ptr_t data, const mem_ptr_t<CellGifDecDataCtrlParam> dataCtrlParam, mem_ptr_t<CellGifDecDataOutInfo> dataOutInfo)
{
	if (!data.IsGood() || !dataCtrlParam.IsGood() || !dataOutInfo.IsGood())
		return CELL_GIFDEC_ERROR_ARG;

	dataOutInfo->status = CELL_GIFDEC_DEC_STATUS_STOP;

	CellGifDecSubHandle* subHandle_data;
	if(!cellGifDec->CheckId(subHandle, subHandle_data))
		return CELL_GIFDEC_ERROR_FATAL;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	const CellGifDecOutParam& current_outParam = subHandle_data->outParam; 

	//Copy the GIF file to a buffer
	vm::var<unsigned char[]> gif(fileSize);
	vm::var<u64> pos, nread;

	switch(subHandle_data->src.srcSelect.ToBE())
	{
	case se32(CELL_GIFDEC_BUFFER):
		if (!Memory.Copy(gif.addr(), subHandle_data->src.streamPtr.ToLE(), gif.size())) {
			cellGifDec->Error("cellGifDecDecodeData() failed (I)");
			return CELL_EFAULT;
		}
		break;

	case se32(CELL_GIFDEC_FILE):
		cellFsLseek(fd, 0, CELL_SEEK_SET, pos.addr());
		cellFsRead(fd, gif.addr(), gif.size(), nread.addr());
		break;
	}

	//Decode GIF file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	auto image = std::unique_ptr<unsigned char,decltype(&::free)>
		(
			stbi_load_from_memory(gif.ptr(), fileSize, &width, &height, &actual_components, 4),
			&::free
		);

	if (!image)
		return CELL_GIFDEC_ERROR_STREAM_FORMAT;

	const int bytesPerLine = dataCtrlParam->outputBytesPerLine;
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
				if (!Memory.CopyFromReal(data.GetAddr() + dstOffset, &image.get()[srcOffset], linesize))
				{
					cellGifDec->Error("cellGifDecDecodeData() failed (II)");
					return CELL_EFAULT;
				}
			}
		}
		else
		{
			if (!Memory.CopyFromReal(data.GetAddr(), image.get(), image_size))
			{
				cellGifDec->Error("cellGifDecDecodeData() failed (III)");
				return CELL_EFAULT;
			}
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
				if (!Memory.CopyFromReal(data.GetAddr() + dstOffset, output, linesize))
				{
					cellGifDec->Error("cellGifDecDecodeData() failed (IV)");
					return CELL_EFAULT;
				}
			}
			free(output);
		}
		else
		{
			uint* dest = (uint*)new char[image_size];
			uint* source_current = (uint*)&(image.get()[0]);
			uint* dest_current = dest;
			for (uint i = 0; i < image_size / nComponents; i++) 
			{
				uint val = *source_current;
				*dest_current = (val >> 24) | (val << 8); // set alpha (A8) as leftmost byte
				source_current++;
				dest_current++;
			}
			// NOTE: AppendRawBytes has diff side-effect vs Memory.CopyFromReal
			data.AppendRawBytes((u8*)dest, image_size); 
			delete[] dest;
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

int cellGifDecClose(u32 mainHandle, u32 subHandle)
{
	CellGifDecSubHandle* subHandle_data;
	if(!cellGifDec->CheckId(subHandle, subHandle_data))
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
	cellGifDec->AddFunc(0xb60d42a5, cellGifDecCreate);
	cellGifDec->AddFunc(0x4711cb7f, cellGifDecExtCreate);
	cellGifDec->AddFunc(0x75745079, cellGifDecOpen);
	cellGifDec->AddFunc(0xf0da95de, cellGifDecReadHeader);
	cellGifDec->AddFunc(0x41a90dc4, cellGifDecSetParameter);
	cellGifDec->AddFunc(0x44b1bc61, cellGifDecDecodeData);
	cellGifDec->AddFunc(0x116a7da9, cellGifDecClose);
	cellGifDec->AddFunc(0xe74b2cb1, cellGifDecDestroy);
	
	/*cellGifDec->AddFunc(0x17fb83c1, cellGifDecExtOpen);
	cellGifDec->AddFunc(0xe53f91f2, cellGifDecExtReadHeader);
	cellGifDec->AddFunc(0x95cae771, cellGifDecExtSetParameter);
	cellGifDec->AddFunc(0x02e7e03e, cellGifDecExtDecodeData);*/
}
