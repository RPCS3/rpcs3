#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "cellJpgDec.h"
#include "stblib/stb_image.h"

//void cellJpgDec_init();
//Module cellJpgDec(0x000f, cellJpgDec_init);
Module *cellJpgDec = nullptr;

int cellJpgDecCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam)
{
	UNIMPLEMENTED_FUNC(cellJpgDec);
	return CELL_OK;
}

int cellJpgDecExtCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam, u32 extThreadInParam, u32 extThreadOutParam)
{
	UNIMPLEMENTED_FUNC(cellJpgDec);
	return CELL_OK;
}

int cellJpgDecDestroy(u32 mainHandle)
{
	UNIMPLEMENTED_FUNC(cellJpgDec);
	return CELL_OK;
}

int cellJpgDecOpen(u32 mainHandle, mem32_t subHandle, mem_ptr_t<CellJpgDecSrc> src, mem_ptr_t<CellJpgDecOpnInfo> openInfo)
{
	cellJpgDec->Warning("cellJpgDecOpen(mainHandle=0x%x, subHandle=0x%x, src_addr=0x%x, openInfo=0x%x)",
		mainHandle, subHandle.GetAddr(), src.GetAddr(), openInfo);

	if (!subHandle.IsGood() || !src.IsGood())
		return CELL_JPGDEC_ERROR_ARG;

	CellJpgDecSubHandle *current_subHandle = new CellJpgDecSubHandle;

	current_subHandle->fd = 0;
	current_subHandle->src = *src;

	switch(src->srcSelect.ToBE())
	{
	case se32(CELL_JPGDEC_BUFFER):
		current_subHandle->fileSize = src->streamSize.ToLE();
		break;

	case se32(CELL_JPGDEC_FILE):
		// Get file descriptor
		vm::var<be_t<u32>> fd;
		int ret = cellFsOpen(src->fileName, 0, fd.addr(), 0, 0);
		current_subHandle->fd = fd->ToLE();
		if (ret != CELL_OK) return CELL_JPGDEC_ERROR_OPEN_FILE;

		// Get size of file
		vm::var<CellFsStat> sb; // Alloc a CellFsStat struct
		ret = cellFsFstat(current_subHandle->fd, sb.addr());
		if (ret != CELL_OK) return ret;
		current_subHandle->fileSize = sb->st_size;	// Get CellFsStat.st_size
		break;
	}

	// From now, every u32 subHandle argument is a pointer to a CellJpgDecSubHandle struct.
	subHandle = cellJpgDec->GetNewId(current_subHandle);

	return CELL_OK;
}

int cellJpgDecClose(u32 mainHandle, u32 subHandle)
{
	CellJpgDecSubHandle* subHandle_data;
	if(!cellJpgDec->CheckId(subHandle, subHandle_data))
		return CELL_JPGDEC_ERROR_FATAL;

	cellFsClose(subHandle_data->fd);
	Emu.GetIdManager().RemoveID(subHandle);

	return CELL_OK;
}

int cellJpgDecReadHeader(u32 mainHandle, u32 subHandle, mem_ptr_t<CellJpgDecInfo> info)
{
	cellJpgDec->Log("cellJpgDecReadHeader(mainHandle=0x%x, subHandle=0x%x, info_addr=0x%llx)", mainHandle, subHandle, info.GetAddr());

	if (!info.IsGood())
		return CELL_JPGDEC_ERROR_ARG;

	CellJpgDecSubHandle* subHandle_data;
	if(!cellJpgDec->CheckId(subHandle, subHandle_data))
		return CELL_JPGDEC_ERROR_FATAL;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	CellJpgDecInfo& current_info = subHandle_data->info;

	//Write the header to buffer
	vm::var<u8[]> buffer(fileSize);
	vm::var<be_t<u64>> pos, nread;

	switch(subHandle_data->src.srcSelect.ToBE())
	{
	case se32(CELL_JPGDEC_BUFFER):
		if (!Memory.Copy(buffer.addr(), subHandle_data->src.streamPtr.ToLE(), buffer.size())) {
			cellJpgDec->Error("cellJpgDecReadHeader() failed ()");
			return CELL_EFAULT;
		}
		break;

	case se32(CELL_JPGDEC_FILE):
		cellFsLseek(fd, 0, CELL_SEEK_SET, pos.addr());
		cellFsRead(fd, buffer.addr(), buffer.size(), nread.addr());
		break;
	}

	if (*buffer.To<u32>(0) != 0xE0FFD8FF || // Error: Not a valid SOI header
		*buffer.To<u32>(6) != 0x4649464A)   // Error: Not a valid JFIF string
	{
		return CELL_JPGDEC_ERROR_HEADER; 
	}

	u32 i = 4;
	
	if(i >= fileSize)
		return CELL_JPGDEC_ERROR_HEADER;

	u16 block_length = buffer[i] * 0xFF + buffer[i+1];

	while(true)
	{
		i += block_length;                                  // Increase the file index to get to the next block
		if (i >= fileSize ||                                // Check to protect against segmentation faults
			buffer[i] != 0xFF)                              // Check that we are truly at the start of another block
		{
			return CELL_JPGDEC_ERROR_HEADER;
		}

		if(buffer[i+1] == 0xC0)
			break;                                          // 0xFFC0 is the "Start of frame" marker which contains the file size

		i += 2;                                             // Skip the block marker
		block_length = buffer[i] * 0xFF + buffer[i+1];      // Go to the next block
	}

	current_info.imageWidth    = buffer[i+7]*0x100 + buffer[i+8];
	current_info.imageHeight   = buffer[i+5]*0x100 + buffer[i+6];
	current_info.numComponents = 3; // Unimplemented
	current_info.colorSpace    = CELL_JPG_RGB;

	*info = current_info;

	return CELL_OK;
}

int cellJpgDecDecodeData(u32 mainHandle, u32 subHandle, mem8_ptr_t data, const mem_ptr_t<CellJpgDecDataCtrlParam> dataCtrlParam, mem_ptr_t<CellJpgDecDataOutInfo> dataOutInfo)
{
	if (!data.IsGood() || !dataCtrlParam.IsGood() || !dataOutInfo.IsGood())
		return CELL_JPGDEC_ERROR_ARG;

	dataOutInfo->status = CELL_JPGDEC_DEC_STATUS_STOP;
	CellJpgDecSubHandle* subHandle_data;
	if(!cellJpgDec->CheckId(subHandle, subHandle_data))
		return CELL_JPGDEC_ERROR_FATAL;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	const CellJpgDecOutParam& current_outParam = subHandle_data->outParam; 

	//Copy the JPG file to a buffer
	vm::var<unsigned char[]> jpg(fileSize);
	vm::var<u64> pos, nread;

	switch(subHandle_data->src.srcSelect.ToBE())
	{
	case se32(CELL_JPGDEC_BUFFER):
		if (!Memory.Copy(jpg.addr(), subHandle_data->src.streamPtr.ToLE(), jpg.size())) {
			cellJpgDec->Error("cellJpgDecDecodeData() failed (I)");
			return CELL_EFAULT;
		}
		break;

	case se32(CELL_JPGDEC_FILE):
		cellFsLseek(fd, 0, CELL_SEEK_SET, pos.addr());
		cellFsRead(fd, jpg.addr(), jpg.size(), nread.addr());
		break;
	}

	//Decode JPG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	auto image = std::unique_ptr<unsigned char,decltype(&::free)>
		(
			stbi_load_from_memory(jpg.ptr(), fileSize, &width, &height, &actual_components, 4),
			&::free
		);

	if (!image)
		return CELL_JPGDEC_ERROR_STREAM_FORMAT;

	const bool flip = current_outParam.outputMode == CELL_JPGDEC_BOTTOM_TO_TOP;
	const int bytesPerLine = dataCtrlParam->outputBytesPerLine;
	size_t image_size = width * height;

	switch((u32)current_outParam.outputColorSpace)
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
				if (!Memory.CopyFromReal(data.GetAddr() + dstOffset, &image.get()[srcOffset], linesize))
				{
					cellJpgDec->Error("cellJpgDecDecodeData() failed (II)");
					return CELL_EFAULT;
				}
			}
		}
		else
		{
			if (!Memory.CopyFromReal(data.GetAddr(), image.get(), image_size))
			{
				cellJpgDec->Error("cellJpgDecDecodeData() failed (III)");
				return CELL_EFAULT;
			}
		}
	}
	break;

	case CELL_JPG_ARGB:
	{
		const int nComponents = 4;
		image_size *= nComponents;
		if (bytesPerLine > width * nComponents || flip) //check if we need padding
		{
			//TODO: Find out if we can't do padding without an extra copy
			const int linesize = std::min(bytesPerLine, width * nComponents);
			char *output = (char *) malloc(linesize);
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
				if (!Memory.CopyFromReal(data.GetAddr() + dstOffset, output, linesize))
				{
					cellJpgDec->Error("cellJpgDecDecodeData() failed (IV)");
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

	case CELL_JPG_GRAYSCALE:
	case CELL_JPG_YCbCr:
	case CELL_JPG_UPSAMPLE_ONLY:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_RGBA:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_ARGB:
		cellJpgDec->Error("cellJpgDecDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace.ToLE());
	break;

	default:
		return CELL_JPGDEC_ERROR_ARG;
	}

	dataOutInfo->status = CELL_JPGDEC_DEC_STATUS_FINISH;

	if(dataCtrlParam->outputBytesPerLine)
		dataOutInfo->outputLines = image_size / dataCtrlParam->outputBytesPerLine;

	return CELL_OK;
}

int cellJpgDecSetParameter(u32 mainHandle, u32 subHandle, const mem_ptr_t<CellJpgDecInParam> inParam, mem_ptr_t<CellJpgDecOutParam> outParam)
{
	if (!inParam.IsGood() || !outParam.IsGood())
		return CELL_JPGDEC_ERROR_ARG;

	CellJpgDecSubHandle* subHandle_data;
	if(!cellJpgDec->CheckId(subHandle, subHandle_data))
		return CELL_JPGDEC_ERROR_FATAL;

	CellJpgDecInfo& current_info = subHandle_data->info;
	CellJpgDecOutParam& current_outParam = subHandle_data->outParam;

	current_outParam.outputWidthByte  = (current_info.imageWidth * current_info.numComponents);
	current_outParam.outputWidth      = current_info.imageWidth;
	current_outParam.outputHeight     = current_info.imageHeight;
	current_outParam.outputColorSpace = inParam->outputColorSpace;

	switch ((u32)current_outParam.outputColorSpace)
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


void cellJpgDec_init()
{
	cellJpgDec->AddFunc(0xa7978f59, cellJpgDecCreate);
	cellJpgDec->AddFunc(0x8b300f66, cellJpgDecExtCreate);
	cellJpgDec->AddFunc(0x976ca5c2, cellJpgDecOpen);
	cellJpgDec->AddFunc(0x6d9ebccf, cellJpgDecReadHeader);
	cellJpgDec->AddFunc(0xe08f3910, cellJpgDecSetParameter);
	cellJpgDec->AddFunc(0xaf8bb012, cellJpgDecDecodeData);
	cellJpgDec->AddFunc(0x9338a07a, cellJpgDecClose);
	cellJpgDec->AddFunc(0xd8ea91f8, cellJpgDecDestroy);

	/*cellJpgDec->AddFunc(0xa9f703e3, cellJpgDecExtOpen);
	cellJpgDec->AddFunc(0xb91eb3d2, cellJpgDecExtReadHeader);
	cellJpgDec->AddFunc(0x65cbbb16, cellJpgDecExtSetParameter);
	cellJpgDec->AddFunc(0x716f8792, cellJpgDecExtDecodeData);*/
}
