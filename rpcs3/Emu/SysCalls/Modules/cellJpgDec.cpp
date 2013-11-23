#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "stblib/stb_image.h"

void cellJpgDec_init();
Module cellJpgDec(0x000f, cellJpgDec_init);

//Return Codes
enum
{
	CELL_JPGDEC_ERROR_HEADER			= 0x80611101,
	CELL_JPGDEC_ERROR_STREAM_FORMAT		= 0x80611102,
	CELL_JPGDEC_ERROR_ARG				= 0x80611103,
	CELL_JPGDEC_ERROR_SEQ				= 0x80611104,
	CELL_JPGDEC_ERROR_BUSY				= 0x80611105,
	CELL_JPGDEC_ERROR_FATAL				= 0x80611106,
	CELL_JPGDEC_ERROR_OPEN_FILE			= 0x80611107,
	CELL_JPGDEC_ERROR_SPU_UNSUPPORT		= 0x80611108,
	CELL_JPGDEC_ERROR_CB_PARAM			= 0x80611109,
};

enum CellJpgDecColorSpace
{
	CELL_JPG_UNKNOWN					= 0,
	CELL_JPG_GRAYSCALE					= 1,
	CELL_JPG_RGB						= 2,
	CELL_JPG_YCbCr						= 3,
	CELL_JPG_RGBA						= 10,
	CELL_JPG_UPSAMPLE_ONLY				= 11,
	CELL_JPG_ARGB						= 20,
	CELL_JPG_GRAYSCALE_TO_ALPHA_RGBA	= 40,
	CELL_JPG_GRAYSCALE_TO_ALPHA_ARGB	= 41,
};

enum CellJpgDecDecodeStatus
{
	CELL_JPGDEC_DEC_STATUS_FINISH	= 0,	//Decoding finished
	CELL_JPGDEC_DEC_STATUS_STOP		= 1,	//Decoding halted
};

struct CellJpgDecInfo
{
	be_t<u32> imageWidth;
	be_t<u32> imageHeight;
	be_t<u32> numComponents;
	be_t<u32> colorSpace;			// CellJpgDecColorSpace
};

struct CellJpgDecSrc
{
	be_t<u32> srcSelect;			// CellJpgDecStreamSrcSel
	be_t<u32> fileName;				// const char*
	be_t<u64> fileOffset;			// int64_t
	be_t<u32> fileSize;
	be_t<u32> streamPtr;
	be_t<u32> streamSize;
	be_t<u32> spuThreadEnable;	// CellJpgDecSpuThreadEna
};

struct CellJpgDecInParam
{
	be_t<u32> commandPtr;
	be_t<u32> downScale;
	be_t<u32> method;				// CellJpgDecMethod
	be_t<u32> outputMode;			// CellJpgDecOutputMode
	be_t<u32> outputColorSpace;		// CellJpgDecColorSpace
	be_t<u8> outputColorAlpha;
	be_t<u8> reserved[3];
};

struct CellJpgDecOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputWidth;
	be_t<u32> outputHeight;
	be_t<u32> outputComponents;
	be_t<u32> outputMode;			// CellJpgDecOutputMode
	be_t<u32> outputColorSpace;	// CellJpgDecColorSpace
	be_t<u32> downScale;
	be_t<u32> useMemorySpace;
};

struct CellJpgDecOpnInfo
{
	be_t<u32> initSpaceAllocated;
};

struct CellJpgDecDataCtrlParam
{
	be_t<u64> outputBytesPerLine;
};

struct CellJpgDecDataOutInfo
{
	be_t<float> mean;
	be_t<u32> outputLines;
	be_t<u32> status;
};


struct CellJpgDecSubHandle	//Custom struct
{
	u32 fd;
	u64 fileSize;
	CellJpgDecInfo info;
	CellJpgDecOutParam outParam;
};

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

int cellJpgDecOpen(u32 mainHandle, mem32_t subHandle, u32 src_addr, mem_ptr_t<CellJpgDecOpnInfo> openInfo)
{
	//u32 srcSelect       = Memory.Read32(src_addr);
	u32 fileName		  = Memory.Read32(src_addr+4);
	//u32 fileOffset      = Memory.Read32(src_addr+8);
	//u32 fileSize        = Memory.Read32(src_addr+12);
	//u32 streamPtr       = Memory.Read32(src_addr+16);
	//u32 streamSize      = Memory.Read32(src_addr+20);
	//u32 spuThreadEnable = Memory.Read32(src_addr+24);

	CellJpgDecSubHandle *current_subHandle = new CellJpgDecSubHandle;

	// Get file descriptor
	MemoryAllocator<be_t<u32>> fd;
	int ret = cellFsOpen(fileName, 0, fd, NULL, 0);
	current_subHandle->fd = fd->ToLE();
	if(ret != CELL_OK) return CELL_JPGDEC_ERROR_OPEN_FILE;

	// Get size of file
	MemoryAllocator<CellFsStat> sb; // Alloc a CellFsStat struct
	ret = cellFsFstat(current_subHandle->fd, sb.GetAddr());
	if(ret != CELL_OK) return ret;
	current_subHandle->fileSize = sb->st_size;	// Get CellFsStat.st_size

	// From now, every u32 subHandle argument is a pointer to a CellPngDecSubHandle struct.
	subHandle = cellJpgDec.GetNewId(current_subHandle);

	return CELL_OK;
}

int cellJpgDecClose(u32 mainHandle, u32 subHandle)
{
	ID sub_handle_id_data;
	if(!cellJpgDec.CheckId(subHandle, sub_handle_id_data))
		return CELL_JPGDEC_ERROR_FATAL;

	auto subHandle_data = (CellJpgDecSubHandle*)sub_handle_id_data.m_data;

	cellFsClose(subHandle_data->fd);
	Emu.GetIdManager().RemoveID(subHandle);

	return CELL_OK;
}

int cellJpgDecReadHeader(u32 mainHandle, u32 subHandle, mem_ptr_t<CellJpgDecInfo> info)
{
	cellJpgDec.Log("cellJpgDecReadHeader(mainHandle=0x%x, subHandle=0x%x, info_addr=0x%llx)", mainHandle, subHandle, info.GetAddr());
	ID sub_handle_id_data;
	if(!cellJpgDec.CheckId(subHandle, sub_handle_id_data))
		return CELL_JPGDEC_ERROR_FATAL;

	auto subHandle_data = (CellJpgDecSubHandle*)sub_handle_id_data.m_data;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	CellJpgDecInfo& current_info = subHandle_data->info;

	//Copy the JPG file to a buffer
	MemoryAllocator<u8> buffer(fileSize);
	MemoryAllocator<be_t<u64>> pos, nread;

	cellFsLseek(fd, 0, CELL_SEEK_SET, pos);
	cellFsRead(fd, buffer.GetAddr(), buffer.GetSize(), nread);

	if (*buffer.To<u32>(0) != 0xE0FFD8FF ||		// Error: Not a valid SOI header
		*buffer.To<u32>(6) != 0x4649464A)		// Error: Not a valid JFIF string
	{
		return CELL_JPGDEC_ERROR_HEADER; 
	}

	u32 i = 4;
	
	if(i >= fileSize)
		return CELL_JPGDEC_ERROR_HEADER;

	u16 block_length = buffer[i] * 0xFF + buffer[i+1];

	while(true)
	{
		i += block_length;									// Increase the file index to get to the next block
		if (i >= fileSize ||								// Check to protect against segmentation faults
			buffer[i] != 0xFF)								// Check that we are truly at the start of another block
		{
			return CELL_JPGDEC_ERROR_HEADER;
		}

		if(buffer[i+1] == 0xC0)
			break;											// 0xFFC0 is the "Start of frame" marker which contains the file size

		i += 2;												// Skip the block marker
		block_length = buffer[i] * 0xFF + buffer[i+1];		// Go to the next block
	}

	current_info.imageWidth			= buffer[i+7]*0x100 + buffer[i+8];
	current_info.imageHeight		= buffer[i+5]*0x100 + buffer[i+6];
	current_info.numComponents		= 3;	// Unimplemented
	current_info.colorSpace			= CELL_JPG_RGB;

	*info = current_info;

	return CELL_OK;
}

int cellJpgDecDecodeData(u32 mainHandle, u32 subHandle, mem8_ptr_t data, const mem_ptr_t<CellJpgDecDataCtrlParam> dataCtrlParam, mem_ptr_t<CellJpgDecDataOutInfo> dataOutInfo)
{
	dataOutInfo->status = CELL_JPGDEC_DEC_STATUS_STOP;
	ID sub_handle_id_data;
	if(!cellJpgDec.CheckId(subHandle, sub_handle_id_data))
		return CELL_JPGDEC_ERROR_FATAL;

	auto subHandle_data = (CellJpgDecSubHandle*)sub_handle_id_data.m_data;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	const CellJpgDecOutParam& current_outParam = subHandle_data->outParam; 

	//Copy the JPG file to a buffer
	MemoryAllocator<unsigned char> jpg(fileSize);
	MemoryAllocator<u64> pos, nread;
	cellFsLseek(fd, 0, CELL_SEEK_SET, pos);
	cellFsRead(fd, jpg.GetAddr(), jpg.GetSize(), nread);

	//Decode JPG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	std::shared_ptr<unsigned char> image(stbi_load_from_memory(jpg, fileSize, &width, &height, &actual_components, 4));

	if (!image) return CELL_JPGDEC_ERROR_STREAM_FORMAT;

	uint image_size = width * height;
	switch(current_outParam.outputColorSpace)
	{
	case CELL_JPG_RGBA:
	case CELL_JPG_RGB:
		image_size *= current_outParam.outputColorSpace == CELL_JPG_RGBA ? 4 : 3;
		memcpy(data, image.get(), image_size);
	break;

	case CELL_JPG_ARGB:
		image_size *= 4;

		for(u32 i = 0; i < image_size; i+=4)
		{
			data += image.get()[i+3];
			data += image.get()[i+0];
			data += image.get()[i+1];
			data += image.get()[i+2];
		}
	break;

	case CELL_JPG_GRAYSCALE:
	case CELL_JPG_YCbCr:
	case CELL_JPG_UPSAMPLE_ONLY:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_RGBA:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_ARGB:
		cellJpgDec.Error("cellJpgDecDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace.ToLE());
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
	ID sub_handle_id_data;
	if(!cellJpgDec.CheckId(subHandle, sub_handle_id_data))
		return CELL_JPGDEC_ERROR_FATAL;

	auto subHandle_data = (CellJpgDecSubHandle*)sub_handle_id_data.m_data;

	CellJpgDecInfo& current_info = subHandle_data->info;
	CellJpgDecOutParam& current_outParam = subHandle_data->outParam;

	current_outParam.outputWidthByte	= (current_info.imageWidth * current_info.numComponents);
	current_outParam.outputWidth		= current_info.imageWidth;
	current_outParam.outputHeight		= current_info.imageHeight;
	current_outParam.outputColorSpace	= inParam->outputColorSpace;

	switch (current_outParam.outputColorSpace)
	{
	case CELL_JPG_GRAYSCALE:				current_outParam.outputComponents = 1; break;

	case CELL_JPG_RGB:
	case CELL_JPG_YCbCr:					current_outParam.outputComponents = 3; break;

	case CELL_JPG_UPSAMPLE_ONLY:			current_outParam.outputComponents = current_info.numComponents; break;

	case CELL_JPG_RGBA:
	case CELL_JPG_ARGB:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_RGBA:
	case CELL_JPG_GRAYSCALE_TO_ALPHA_ARGB:	current_outParam.outputComponents = 4; break;

	default: return CELL_JPGDEC_ERROR_ARG;	// Not supported color space
	}

	current_outParam.outputMode		= inParam->outputMode;
	current_outParam.downScale		= inParam->downScale;
	current_outParam.useMemorySpace	= 0;	// Unimplemented

	*outParam = current_outParam;

	return CELL_OK;
}


void cellJpgDec_init()
{
	cellJpgDec.AddFunc(0xa7978f59, cellJpgDecCreate);
	cellJpgDec.AddFunc(0x8b300f66, cellJpgDecExtCreate);
	cellJpgDec.AddFunc(0x976ca5c2, cellJpgDecOpen);
	cellJpgDec.AddFunc(0x6d9ebccf, cellJpgDecReadHeader);
	cellJpgDec.AddFunc(0xe08f3910, cellJpgDecSetParameter);
	cellJpgDec.AddFunc(0xaf8bb012, cellJpgDecDecodeData);
	cellJpgDec.AddFunc(0x9338a07a, cellJpgDecClose);
	cellJpgDec.AddFunc(0xd8ea91f8, cellJpgDecDestroy);

	/*cellJpgDec.AddFunc(0xa9f703e3, cellJpgDecExtOpen);
	cellJpgDec.AddFunc(0xb91eb3d2, cellJpgDecExtReadHeader);
	cellJpgDec.AddFunc(0x65cbbb16, cellJpgDecExtSetParameter);
	cellJpgDec.AddFunc(0x716f8792, cellJpgDecExtDecodeData);*/
}
