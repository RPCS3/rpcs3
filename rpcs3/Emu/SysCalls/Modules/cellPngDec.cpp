#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "stblib/stb_image.h"

void cellPngDec_init();
Module cellPngDec(0x0018, cellPngDec_init);

//Return Codes
enum
{
	CELL_PNGDEC_ERROR_HEADER 			= 0x80611201,
	CELL_PNGDEC_ERROR_STREAM_FORMAT 	= 0x80611202,
	CELL_PNGDEC_ERROR_ARG 				= 0x80611203,
	CELL_PNGDEC_ERROR_SEQ 				= 0x80611204,
	CELL_PNGDEC_ERROR_BUSY 				= 0x80611205,
	CELL_PNGDEC_ERROR_FATAL 			= 0x80611206,
	CELL_PNGDEC_ERROR_OPEN_FILE 		= 0x80611207,
	CELL_PNGDEC_ERROR_SPU_UNSUPPORT 	= 0x80611208,
	CELL_PNGDEC_ERROR_SPU_ERROR 		= 0x80611209,
	CELL_PNGDEC_ERROR_CB_PARAM 			= 0x8061120a,
};

enum CellPngDecColorSpace
{
	CELL_PNGDEC_GRAYSCALE			= 1,
	CELL_PNGDEC_RGB					= 2,
	CELL_PNGDEC_PALETTE				= 4,
	CELL_PNGDEC_GRAYSCALE_ALPHA		= 9,
	CELL_PNGDEC_RGBA				= 10,
	CELL_PNGDEC_ARGB				= 20,
};

enum CellPngDecDecodeStatus
{
	CELL_PNGDEC_DEC_STATUS_FINISH	= 0,	//Decoding finished
	CELL_PNGDEC_DEC_STATUS_STOP		= 1,	//Decoding halted
};

enum CellPngDecStreamSrcSel
{
    CELL_PNGDEC_FILE      = 0, 
    CELL_PNGDEC_BUFFER    = 1		
};

struct CellPngDecDataOutInfo
{
	be_t<u32> chunkInformation;
	be_t<u32> numText;
	be_t<u32> numUnknownChunk;
	be_t<u32> status;
};

struct CellPngDecDataCtrlParam
{
	be_t<u64> outputBytesPerLine;
};

struct CellPngDecInfo
{
	be_t<u32> imageWidth;
	be_t<u32> imageHeight;
	be_t<u32> numComponents;
	be_t<u32> colorSpace;			// CellPngDecColorSpace
	be_t<u32> bitDepth;
	be_t<u32> interlaceMethod;		// CellPngDecInterlaceMode
	be_t<u32> chunkInformation;
};

struct CellPngDecSrc
{
	be_t<u32> srcSelect;			// CellPngDecStreamSrcSel
	be_t<u32> fileName;				// const char*
	be_t<u64> fileOffset;			// int64_t
	be_t<u32> fileSize;
	be_t<u32> streamPtr;
	be_t<u32> streamSize;
	be_t<u32> spuThreadEnable;		// CellPngDecSpuThreadEna
};

struct CellPngDecInParam
{
	be_t<u32> commandPtr;
	be_t<u32> outputMode;			// CellPngDecOutputMode
	be_t<u32> outputColorSpace;	// CellPngDecColorSpace
	be_t<u32> outputBitDepth;
	be_t<u32> outputPackFlag;		// CellPngDecPackFlag
	be_t<u32> outputAlphaSelect;	// CellPngDecAlphaSelect
	be_t<u32> outputColorAlpha;
};

struct CellPngDecOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputWidth;
	be_t<u32> outputHeight;
	be_t<u32> outputComponents;
	be_t<u32> outputBitDepth;
	be_t<u32> outputMode;			// CellPngDecOutputMode
	be_t<u32> outputColorSpace;	// CellPngDecColorSpace
	be_t<u32> useMemorySpace;
};

struct CellPngDecSubHandle //Custom struct
{
	u32 fd;
	u64 fileSize;
	CellPngDecInfo info;
	CellPngDecOutParam outParam;
	CellPngDecSrc src;
};

int cellPngDecCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

int cellPngDecDestroy(u32 mainHandle)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

int cellPngDecOpen(u32 mainHandle, mem32_t subHandle, u32 src_addr, u32 openInfo)
{
	cellPngDec.Warning("cellPngDecOpen(mainHandle=0x%x,subHandle=0x%x,src_addr=0x%x,openInfo=0x%x)", mainHandle, subHandle.GetAddr(), src_addr, openInfo);

	CellPngDecSrc* src;

	src = (CellPngDecSrc*)Memory.GetMemFromAddr(src_addr);

	CellPngDecSubHandle *current_subHandle = new CellPngDecSubHandle;

	current_subHandle->fd = NULL;
	current_subHandle->src = *src;

	switch(src->srcSelect.ToBE())
	{
	case const_se_t<u32, CELL_PNGDEC_BUFFER>::value:
		current_subHandle->fileSize = src->streamSize.ToLE();
		break;

	case const_se_t<u32, CELL_PNGDEC_FILE>::value:
		// Get file descriptor
		MemoryAllocator<be_t<u32>> fd;
		int ret = cellFsOpen(src->fileName, 0, fd, NULL, 0);
		current_subHandle->fd = fd->ToLE();
		if(ret != CELL_OK) return CELL_PNGDEC_ERROR_OPEN_FILE;

		// Get size of file
		MemoryAllocator<CellFsStat> sb; // Alloc a CellFsStat struct
		ret = cellFsFstat(current_subHandle->fd, sb.GetAddr());
		if(ret != CELL_OK) return ret;
		current_subHandle->fileSize = sb->st_size;	// Get CellFsStat.st_size
		break;
	}

	// From now, every u32 subHandle argument is a pointer to a CellPngDecSubHandle struct.
	subHandle = cellPngDec.GetNewId(current_subHandle);

	return CELL_OK;
}

int cellPngDecClose(u32 mainHandle, u32 subHandle)
{
	cellPngDec.Warning("cellPngDecClose(mainHandle=0x%x,subHandle=0x%x)", mainHandle, subHandle);

	ID sub_handle_id_data;
	if(!cellPngDec.CheckId(subHandle, sub_handle_id_data))
		return CELL_PNGDEC_ERROR_FATAL;

	auto subHandle_data = (CellPngDecSubHandle*)sub_handle_id_data.m_data;

	cellFsClose(subHandle_data->fd);
	Emu.GetIdManager().RemoveID(subHandle);

	return CELL_OK;
}

int cellPngDecReadHeader(u32 mainHandle, u32 subHandle, mem_ptr_t<CellPngDecInfo> info)
{
	cellPngDec.Warning("cellPngDecReadHeader(mainHandle=0x%x, subHandle=0x%x, info_addr=0x%llx)", mainHandle, subHandle, info.GetAddr());
	ID sub_handle_id_data;
	if(!cellPngDec.CheckId(subHandle, sub_handle_id_data))
		return CELL_PNGDEC_ERROR_FATAL;

	auto subHandle_data = (CellPngDecSubHandle*)sub_handle_id_data.m_data;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	CellPngDecInfo& current_info = subHandle_data->info;

	//Check size of file
	if(fileSize < 29) return CELL_PNGDEC_ERROR_HEADER;	// Error: The file is smaller than the length of a PNG header
	
	//Write the header to buffer
	MemoryAllocator<be_t<u32>> buffer(34); // Alloc buffer for PNG header
	MemoryAllocator<be_t<u64>> pos, nread;

	switch(subHandle_data->src.srcSelect.ToLE())
	{
	case CELL_PNGDEC_BUFFER:
		memcpy(Memory.VirtualToRealAddr(buffer.GetAddr()), Memory.VirtualToRealAddr(subHandle_data->src.streamPtr.ToLE()), buffer.GetSize());
		break;
	case CELL_PNGDEC_FILE:
		cellFsLseek(fd, 0, CELL_SEEK_SET, pos);
		cellFsRead(fd, buffer.GetAddr(), buffer.GetSize(), nread);
		break;
	}

	if (buffer[0] != 0x89504E47 ||
		buffer[1] != 0x0D0A1A0A ||  // Error: The first 8 bytes are not a valid PNG signature
		buffer[3] != 0x49484452)   // Error: The PNG file does not start with an IHDR chunk
	{
		return CELL_PNGDEC_ERROR_HEADER; 
	}

	switch (buffer.To<u8>()[25])
	{
	case 0: current_info.colorSpace = CELL_PNGDEC_GRAYSCALE;		current_info.numComponents = 1; break;
	case 2: current_info.colorSpace = CELL_PNGDEC_RGB;				current_info.numComponents = 3; break;
	case 3: current_info.colorSpace = CELL_PNGDEC_PALETTE;			current_info.numComponents = 1; break;
	case 4: current_info.colorSpace = CELL_PNGDEC_GRAYSCALE_ALPHA;	current_info.numComponents = 2; break;
	case 6: current_info.colorSpace = CELL_PNGDEC_RGBA;				current_info.numComponents = 4; break;
	default: return CELL_PNGDEC_ERROR_HEADER; // Not supported color type
	}

	current_info.imageWidth			= buffer[4];
	current_info.imageHeight		= buffer[5];
	current_info.bitDepth			= buffer.To<u8>()[24];
	current_info.interlaceMethod	= buffer.To<u8>()[28];
	current_info.chunkInformation	= 0; // Unimplemented

	*info = current_info;

	return CELL_OK;
}

int cellPngDecDecodeData(u32 mainHandle, u32 subHandle, mem8_ptr_t data, const mem_ptr_t<CellPngDecDataCtrlParam> dataCtrlParam, mem_ptr_t<CellPngDecDataOutInfo> dataOutInfo)
{
	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_STOP;
	ID sub_handle_id_data;
	if(!cellPngDec.CheckId(subHandle, sub_handle_id_data))
		return CELL_PNGDEC_ERROR_FATAL;

	auto subHandle_data = (CellPngDecSubHandle*)sub_handle_id_data.m_data;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	const CellPngDecOutParam& current_outParam = subHandle_data->outParam;

	//Copy the PNG file to a buffer
	MemoryAllocator<unsigned char> png(fileSize);
	MemoryAllocator<u64> pos, nread;

	switch(subHandle_data->src.srcSelect.ToLE())
	{
	case CELL_PNGDEC_BUFFER:
		memcpy(Memory.VirtualToRealAddr(png.GetAddr()), Memory.VirtualToRealAddr(subHandle_data->src.streamPtr.ToLE()), png.GetSize());
		break;
	case CELL_PNGDEC_FILE:
		cellFsLseek(fd, 0, CELL_SEEK_SET, pos);
		cellFsRead(fd, png.GetAddr(), png.GetSize(), nread);
		break;
	}

	//Decode PNG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	std::shared_ptr<unsigned char> image(stbi_load_from_memory(png, fileSize, &width, &height, &actual_components, 4));
	if (!image)	return CELL_PNGDEC_ERROR_STREAM_FORMAT;

	uint image_size = width * height;
	switch(current_outParam.outputColorSpace)
	{
	case CELL_PNGDEC_RGB:
	case CELL_PNGDEC_RGBA:
		image_size *= current_outParam.outputColorSpace == CELL_PNGDEC_RGBA ? 4 : 3;
		memcpy(data, image.get(), image_size);
	break;

	case CELL_PNGDEC_ARGB:
		image_size *= 4;

		for(uint i = 0; i < image_size; i+=4)
		{
			data += image.get()[i+3];
			data += image.get()[i+0];
			data += image.get()[i+1];
			data += image.get()[i+2];
		}
	break;

	case CELL_PNGDEC_GRAYSCALE:
	case CELL_PNGDEC_PALETTE:
	case CELL_PNGDEC_GRAYSCALE_ALPHA:
		cellPngDec.Error("cellPngDecDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace.ToLE());
	break;

	default:
		return CELL_PNGDEC_ERROR_ARG;
	}

	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_FINISH;

	return CELL_OK;
}

int cellPngDecSetParameter(u32 mainHandle, u32 subHandle, const mem_ptr_t<CellPngDecInParam> inParam, mem_ptr_t<CellPngDecOutParam> outParam)
{
	ID sub_handle_id_data;
	if(!cellPngDec.CheckId(subHandle, sub_handle_id_data))
		return CELL_PNGDEC_ERROR_FATAL;

	auto subHandle_data = (CellPngDecSubHandle*)sub_handle_id_data.m_data;

	CellPngDecInfo& current_info = subHandle_data->info;
	CellPngDecOutParam& current_outParam = subHandle_data->outParam;

	current_outParam.outputWidthByte	= (current_info.imageWidth * current_info.numComponents * current_info.bitDepth) / 8;
	current_outParam.outputWidth		= current_info.imageWidth;
	current_outParam.outputHeight		= current_info.imageHeight;
	current_outParam.outputColorSpace	= inParam->outputColorSpace;
	switch (current_outParam.outputColorSpace)
	{
	case CELL_PNGDEC_PALETTE:
	case CELL_PNGDEC_GRAYSCALE:			current_outParam.outputComponents = 1; break;

	case CELL_PNGDEC_GRAYSCALE_ALPHA:	current_outParam.outputComponents = 2; break;

	case CELL_PNGDEC_RGB:				current_outParam.outputComponents = 3; break;

	case CELL_PNGDEC_RGBA:
	case CELL_PNGDEC_ARGB:				current_outParam.outputComponents = 4; break;

	default: return CELL_PNGDEC_ERROR_ARG;	// Not supported color space
	}

	current_outParam.outputBitDepth		= inParam->outputBitDepth;
	current_outParam.outputMode			= inParam->outputMode;
	current_outParam.useMemorySpace		= 0;	// Unimplemented

	*outParam = current_outParam;

	return CELL_OK;
}

void cellPngDec_init()
{
	cellPngDec.AddFunc(0x157d30c5, cellPngDecCreate);
	cellPngDec.AddFunc(0x820dae1a, cellPngDecDestroy);
	cellPngDec.AddFunc(0xd2bc5bfd, cellPngDecOpen);
	cellPngDec.AddFunc(0x5b3d1ff1, cellPngDecClose);
	cellPngDec.AddFunc(0x9ccdcc95, cellPngDecReadHeader);
	cellPngDec.AddFunc(0x2310f155, cellPngDecDecodeData);
	cellPngDec.AddFunc(0xe97c9bd4, cellPngDecSetParameter);

	/*cellPngDec.AddFunc(0x48436b2d, cellPngDecExtCreate);
	cellPngDec.AddFunc(0x0c515302, cellPngDecExtOpen);
	cellPngDec.AddFunc(0x8b33f863, cellPngDecExtReadHeader);
	cellPngDec.AddFunc(0x726fc1d0, cellPngDecExtDecodeData);
	cellPngDec.AddFunc(0x9e9d7d42, cellPngDecExtSetParameter);
	cellPngDec.AddFunc(0x7585a275, cellPngDecGetbKGD);
	cellPngDec.AddFunc(0x7a062d26, cellPngDecGetcHRM);
	cellPngDec.AddFunc(0xb153629c, cellPngDecGetgAMA);
	cellPngDec.AddFunc(0xb905ebb7, cellPngDecGethIST);
	cellPngDec.AddFunc(0xf44b6c30, cellPngDecGetiCCP);
	cellPngDec.AddFunc(0x27c921b5, cellPngDecGetoFFs);
	cellPngDec.AddFunc(0xb4fe75e1, cellPngDecGetpCAL);
	cellPngDec.AddFunc(0x3d50016a, cellPngDecGetpHYs);
	cellPngDec.AddFunc(0x30cb334a, cellPngDecGetsBIT);
	cellPngDec.AddFunc(0xc41e1198, cellPngDecGetsCAL);
	cellPngDec.AddFunc(0xa5cdf57e, cellPngDecGetsPLT);
	cellPngDec.AddFunc(0xe4416e82, cellPngDecGetsRGB);
	cellPngDec.AddFunc(0x35a6846c, cellPngDecGettIME);
	cellPngDec.AddFunc(0xb96fb26e, cellPngDecGettRNS);
	cellPngDec.AddFunc(0xe163977f, cellPngDecGetPLTE);
	cellPngDec.AddFunc(0x609ec7d5, cellPngDecUnknownChunks);
	cellPngDec.AddFunc(0xb40ca175, cellPngDecGetTextChunk);*/
}
