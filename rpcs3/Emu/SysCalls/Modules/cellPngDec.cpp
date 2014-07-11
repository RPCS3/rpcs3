#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/Modules.h"
#include "cellPngDec.h"
#include "stblib/stb_image.h"
#include <map>

//void cellPngDec_init();
//Module cellPngDec(0x0018, cellPngDec_init);
Module *cellPngDec = nullptr;

static std::map<u32, CellPngDecMainHandle *> cellPngDecMap;

CellPngDecMainHandle *getCellPngDecCtx(u32 mainHandle) {
	if (cellPngDecMap.find(mainHandle) == cellPngDecMap.end())
		return NULL;

	return cellPngDecMap[mainHandle];
}

int cellPngDecCreate(u32 mainHandle, u32 threadInParam, u32 threadOutParam)
{
	cellPngDec->Warning("cellPngDecCreate(mainHandle=0x%x, threadInParam=0x%x, threadOutParam=0x%x)", mainHandle, threadInParam, threadOutParam);
	CellPngDecMainHandle *ctx = new CellPngDecMainHandle;
	if (cellPngDecMap.find(mainHandle) != cellPngDecMap.end()) {
		delete cellPngDecMap[mainHandle];
		cellPngDecMap.erase(mainHandle);
	}
	cellPngDecMap[mainHandle] = ctx;

	ctx->threadInParam = threadInParam;
	ctx->threadOutParam = threadOutParam;

	return CELL_OK;
}

int cellPngDecDestroy(u32 mainHandle)
{
	cellPngDec->Warning("cellPngDecDestroy(mainHandle=0x%x)", mainHandle);
	CellPngDecMainHandle *ctx = getCellPngDecCtx(mainHandle);
	if (!ctx) {
		cellPngDec->Warning("cellPngDecDestroy(mainHandle=0x%x): bad handle", mainHandle);
		return -1;
	}

	delete ctx;
	cellPngDecMap.erase(mainHandle);

	return CELL_OK;
}

int cellPngDecOpen(u32 mainHandle, mem32_t subHandle, mem_ptr_t<CellPngDecSrc> src, u32 openInfo)
{
	cellPngDec->Warning("cellPngDecOpen(mainHandle=0x%x, subHandle=0x%x, src_addr=0x%x, openInfo=0x%x)",
		mainHandle, subHandle.GetAddr(), src.GetAddr(), openInfo);

	if (!subHandle.IsGood() || !src.IsGood())
		return CELL_PNGDEC_ERROR_ARG;

	CellPngDecSubHandle *current_subHandle = new CellPngDecSubHandle;

	current_subHandle->fd = 0;
	current_subHandle->src = *src;

	switch(src->srcSelect.ToBE())
	{
	case se32(CELL_PNGDEC_BUFFER):
		current_subHandle->fileSize = src->streamSize.ToLE();
		break;

	case se32(CELL_PNGDEC_FILE):
		// Get file descriptor
		MemoryAllocator<be_t<u32>> fd;
		int ret = cellFsOpen(src->fileName_addr, 0, fd.GetAddr(), 0, 0);
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
	subHandle = cellPngDec->GetNewId(current_subHandle);

	return CELL_OK;
}

int cellPngDecExtOpen(u32 mainHandle, mem32_t subHandle, mem_ptr_t<CellPngDecSrc> src, u32 openInfo, mem_ptr_t<CellPngDecCbCtrlStrm> cbCtrlStrm, mem_ptr_t<CellPngDecOpnParam> opnParam)
{
	cellPngDec->Warning("cellPngDecExtOpen(mainHandle=0x%x, subHandle=0x%x, src_addr=0x%x, openInfo=0x%x, cbCtrlStrm_addr=0x%x, opnParam=0x%x)",
		mainHandle, subHandle.GetAddr(), src.GetAddr(), openInfo, cbCtrlStrm.GetAddr(), opnParam.GetAddr());

	cellPngDec->Warning("*** cbCtrlStrm->cbCtrlStrmFunc_addr=0x%x", cbCtrlStrm->cbCtrlStrmFunc.GetAddr());

	MemoryAllocator<CellPngDecStrmInfo> streamInfo;
	MemoryAllocator<CellPngDecStrmParam> streamParam;

	int res = cellPngDecOpen(mainHandle, subHandle, src, openInfo);

	if (!res) cbCtrlStrm->cbCtrlStrmFunc(streamInfo.GetAddr(), streamParam.GetAddr(), cbCtrlStrm->cbCtrlStrmArg);

	return res;
}

int cellPngDecClose(u32 mainHandle, u32 subHandle)
{
	cellPngDec->Warning("cellPngDecClose(mainHandle=0x%x,subHandle=0x%x)", mainHandle, subHandle);

	CellPngDecSubHandle* subHandle_data;
	if(!cellPngDec->CheckId(subHandle, subHandle_data))
		return CELL_PNGDEC_ERROR_FATAL;

	cellFsClose(subHandle_data->fd);
	Emu.GetIdManager().RemoveID(subHandle);

	return CELL_OK;
}

int cellPngDecReadHeader(u32 mainHandle, u32 subHandle, mem_ptr_t<CellPngDecInfo> info)
{
	if (!info.IsGood())
		return CELL_PNGDEC_ERROR_ARG;

	cellPngDec->Warning("cellPngDecReadHeader(mainHandle=0x%x, subHandle=0x%x, info_addr=0x%x)", mainHandle, subHandle, info.GetAddr());
	CellPngDecSubHandle* subHandle_data;
	if(!cellPngDec->CheckId(subHandle, subHandle_data))
		return CELL_PNGDEC_ERROR_FATAL;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	CellPngDecInfo& current_info = subHandle_data->info;

	//Check size of file
	if(fileSize < 29) return CELL_PNGDEC_ERROR_HEADER;	// Error: The file is smaller than the length of a PNG header
	
	//Write the header to buffer
	MemoryAllocator<be_t<u32>> buffer(34); // Alloc buffer for PNG header
	MemoryAllocator<be_t<u64>> pos, nread;

	switch(subHandle_data->src.srcSelect.ToBE())
	{
	case se32(CELL_PNGDEC_BUFFER):
		if (!Memory.Copy(buffer.GetAddr(), subHandle_data->src.streamPtr.ToLE(), buffer.GetSize()))
		{
			cellPngDec->Error("cellPngDecReadHeader() failed ()");
			return CELL_EFAULT;
		}
		break;
	case se32(CELL_PNGDEC_FILE):
		cellFsLseek(fd, 0, CELL_SEEK_SET, pos.GetAddr());
		cellFsRead(fd, buffer.GetAddr(), buffer.GetSize(), nread.GetAddr());
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
	case 0: current_info.colorSpace = CELL_PNGDEC_GRAYSCALE;       current_info.numComponents = 1; break;
	case 2: current_info.colorSpace = CELL_PNGDEC_RGB;             current_info.numComponents = 3; break;
	case 3: current_info.colorSpace = CELL_PNGDEC_PALETTE;         current_info.numComponents = 1; break;
	case 4: current_info.colorSpace = CELL_PNGDEC_GRAYSCALE_ALPHA; current_info.numComponents = 2; break;
	case 6: current_info.colorSpace = CELL_PNGDEC_RGBA;            current_info.numComponents = 4; break;
	default: return CELL_PNGDEC_ERROR_HEADER; // Not supported color type
	}

	current_info.imageWidth       = buffer[4];
	current_info.imageHeight      = buffer[5];
	current_info.bitDepth         = buffer.To<u8>()[24];
	current_info.interlaceMethod  = buffer.To<u8>()[28];
	current_info.chunkInformation = 0; // Unimplemented

	*info = current_info;

	return CELL_OK;
}

int cellPngDecExtReadHeader(u32 mainHandle, u32 subHandle, mem_ptr_t<CellPngDecInfo> info, mem_ptr_t<CellPngDecExtInfo> extInfo)
{
	cellPngDec->Warning("cellPngDecExtReadHeader(mainHandle=0x%x, subHandle=0x%x, info_addr=0x%x, extInfo_addr=0x%x)",
		mainHandle, subHandle, info.GetAddr(), extInfo.GetAddr());

	return cellPngDecReadHeader(mainHandle, subHandle, info);
}

int cellPngDecDecodeData(u32 mainHandle, u32 subHandle, mem8_ptr_t data, const mem_ptr_t<CellPngDecDataCtrlParam> dataCtrlParam, mem_ptr_t<CellPngDecDataOutInfo> dataOutInfo)
{
	if (!data.IsGood() || !dataCtrlParam.IsGood() || !dataOutInfo.IsGood())
		return CELL_PNGDEC_ERROR_ARG;

	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_STOP;
	CellPngDecSubHandle* subHandle_data;
	if(!cellPngDec->CheckId(subHandle, subHandle_data))
		return CELL_PNGDEC_ERROR_FATAL;

	const u32& fd = subHandle_data->fd;
	const u64& fileSize = subHandle_data->fileSize;
	const CellPngDecOutParam& current_outParam = subHandle_data->outParam;

	//Copy the PNG file to a buffer
	MemoryAllocator<unsigned char> png(fileSize);
	MemoryAllocator<u64> pos, nread;

	switch(subHandle_data->src.srcSelect.ToLE())
	{
	case CELL_PNGDEC_BUFFER:
		if (!Memory.Copy(png.GetAddr(), subHandle_data->src.streamPtr.ToLE(), png.GetSize()))
		{
			cellPngDec->Error("cellPngDecDecodeData() failed (I)");
			return CELL_EFAULT;
		}
		break;
	case CELL_PNGDEC_FILE:
		cellFsLseek(fd, 0, CELL_SEEK_SET, pos.GetAddr());
		cellFsRead(fd, png.GetAddr(), png.GetSize(), nread.GetAddr());
		break;
	}

	//Decode PNG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	auto image = std::unique_ptr<unsigned char,decltype(&::free)>
			(
				stbi_load_from_memory(png.GetPtr(), fileSize, &width, &height, &actual_components, 4),
				&::free
			);
	if (!image)	return CELL_PNGDEC_ERROR_STREAM_FORMAT;

	const bool flip = current_outParam.outputMode == CELL_PNGDEC_BOTTOM_TO_TOP;
	const int bytesPerLine = dataCtrlParam->outputBytesPerLine;

	uint image_size = width * height;
	switch((u32)current_outParam.outputColorSpace)
	{
	case CELL_PNGDEC_RGB:
		image_size = width * height;
	case CELL_PNGDEC_RGBA:
	{
		const char nComponents = current_outParam.outputColorSpace == CELL_PNGDEC_RGBA ? 4 : 3;
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
					cellPngDec->Error("cellPngDecDecodeData() failed (II)");
					return CELL_EFAULT;
				}
			}
		}
		else
		{
			if (!Memory.CopyFromReal(data.GetAddr(), image.get(), image_size))
			{
				cellPngDec->Error("cellPngDecDecodeData() failed (III)");
				return CELL_EFAULT;
			}
		}
	}
	break;

	case CELL_PNGDEC_ARGB:
	{
		const int nComponents = 4;
		image_size *= nComponents;
		if (bytesPerLine > width * nComponents || flip) //check if we need padding
		{
			//TODO: find out if we can't do padding without an extra copy
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
					cellPngDec->Error("cellPngDecDecodeData() failed (IV)");
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

	case CELL_PNGDEC_GRAYSCALE:
	case CELL_PNGDEC_PALETTE:
	case CELL_PNGDEC_GRAYSCALE_ALPHA:
		cellPngDec->Error("cellPngDecDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace.ToLE());
	break;

	default:
		return CELL_PNGDEC_ERROR_ARG;
	}

	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_FINISH;

	return CELL_OK;
}

int cellPngDecExtDecodeData(u32 mainHandle, u32 subHandle, mem8_ptr_t data, const mem_ptr_t<CellPngDecDataCtrlParam> dataCtrlParam,
	mem_ptr_t<CellPngDecDataOutInfo> dataOutInfo, mem_ptr_t<CellPngDecCbCtrlDisp> cbCtrlDisp, mem_ptr_t<CellPngDecDispParam> dispParam)
{
	cellPngDec->Warning("cellPngDecExtDecodeData(mainHandle=0x%x, subHandle=0x%x, data_addr=0x%x, dataCtrlParam_addr=0x%x, dataOutInfo_addr=0x%x, cbCtrlDisp_addr=0x%x, dispParam=0x%x",
		mainHandle, subHandle, data.GetAddr(), dataCtrlParam.GetAddr(), dataOutInfo.GetAddr(), cbCtrlDisp.GetAddr(), dispParam.GetAddr());

	if (cbCtrlDisp.GetAddr()) cellPngDec->Warning("*** cbCtrlDisp->cbCtrlDispFunc_addr=0x%x", (u32)cbCtrlDisp->cbCtrlDispFunc_addr);

	return cellPngDecDecodeData(mainHandle, subHandle, data, dataCtrlParam, dataOutInfo);
}

int cellPngDecSetParameter(u32 mainHandle, u32 subHandle, const mem_ptr_t<CellPngDecInParam> inParam, mem_ptr_t<CellPngDecOutParam> outParam)
{
	if (!inParam.IsGood() || !outParam.IsGood())
		return CELL_PNGDEC_ERROR_ARG;

	CellPngDecSubHandle* subHandle_data;
	if(!cellPngDec->CheckId(subHandle, subHandle_data))
		return CELL_PNGDEC_ERROR_FATAL;

	CellPngDecInfo& current_info = subHandle_data->info;
	CellPngDecOutParam& current_outParam = subHandle_data->outParam;

	current_outParam.outputWidthByte  = (current_info.imageWidth * current_info.numComponents * current_info.bitDepth) / 8;
	current_outParam.outputWidth      = current_info.imageWidth;
	current_outParam.outputHeight     = current_info.imageHeight;
	current_outParam.outputColorSpace = inParam->outputColorSpace;

	switch ((u32)current_outParam.outputColorSpace)
	{
	case CELL_PNGDEC_PALETTE:
	case CELL_PNGDEC_GRAYSCALE:       current_outParam.outputComponents = 1; break;

	case CELL_PNGDEC_GRAYSCALE_ALPHA: current_outParam.outputComponents = 2; break;

	case CELL_PNGDEC_RGB:             current_outParam.outputComponents = 3; break;

	case CELL_PNGDEC_RGBA:
	case CELL_PNGDEC_ARGB:            current_outParam.outputComponents = 4; break;

	default: return CELL_PNGDEC_ERROR_ARG; // Not supported color space
	}

	current_outParam.outputBitDepth = inParam->outputBitDepth;
	current_outParam.outputMode     = inParam->outputMode;
	current_outParam.useMemorySpace = 0; // Unimplemented

	*outParam = current_outParam;

	return CELL_OK;
}

int cellPngDecExtSetParameter(u32 mainHandle, u32 subHandle, const mem_ptr_t<CellPngDecInParam> inParam, mem_ptr_t<CellPngDecOutParam> outParam,
	mem_ptr_t<CellPngDecExtInParam> extInParam, mem_ptr_t<CellPngDecExtOutParam> extOutParam)
{
	cellPngDec->Warning("cellPngDecExtSetParameter(mainHandle=0x%x, subHandle=0x%x, inParam_addr=0x%x, outParam_addr=0x%x, extInParam=0x%x, extOutParam=0x%x",
		mainHandle, subHandle, inParam.GetAddr(), outParam.GetAddr(), extInParam.GetAddr(), extOutParam.GetAddr());

	return cellPngDecSetParameter(mainHandle, subHandle, inParam, outParam);
}

void cellPngDec_init()
{
	cellPngDec->AddFunc(0x157d30c5, cellPngDecCreate);
	cellPngDec->AddFunc(0x820dae1a, cellPngDecDestroy);
	cellPngDec->AddFunc(0xd2bc5bfd, cellPngDecOpen);
	cellPngDec->AddFunc(0x5b3d1ff1, cellPngDecClose);
	cellPngDec->AddFunc(0x9ccdcc95, cellPngDecReadHeader);
	cellPngDec->AddFunc(0x2310f155, cellPngDecDecodeData);
	cellPngDec->AddFunc(0xe97c9bd4, cellPngDecSetParameter);

	cellPngDec->AddFunc(0x0c515302, cellPngDecExtOpen);
	cellPngDec->AddFunc(0x8b33f863, cellPngDecExtReadHeader);
	cellPngDec->AddFunc(0x726fc1d0, cellPngDecExtDecodeData);
	cellPngDec->AddFunc(0x9e9d7d42, cellPngDecExtSetParameter);

	/*cellPngDec->AddFunc(0x48436b2d, cellPngDecExtCreate);
	cellPngDec->AddFunc(0x7585a275, cellPngDecGetbKGD);
	cellPngDec->AddFunc(0x7a062d26, cellPngDecGetcHRM);
	cellPngDec->AddFunc(0xb153629c, cellPngDecGetgAMA);
	cellPngDec->AddFunc(0xb905ebb7, cellPngDecGethIST);
	cellPngDec->AddFunc(0xf44b6c30, cellPngDecGetiCCP);
	cellPngDec->AddFunc(0x27c921b5, cellPngDecGetoFFs);
	cellPngDec->AddFunc(0xb4fe75e1, cellPngDecGetpCAL);
	cellPngDec->AddFunc(0x3d50016a, cellPngDecGetpHYs);
	cellPngDec->AddFunc(0x30cb334a, cellPngDecGetsBIT);
	cellPngDec->AddFunc(0xc41e1198, cellPngDecGetsCAL);
	cellPngDec->AddFunc(0xa5cdf57e, cellPngDecGetsPLT);
	cellPngDec->AddFunc(0xe4416e82, cellPngDecGetsRGB);
	cellPngDec->AddFunc(0x35a6846c, cellPngDecGettIME);
	cellPngDec->AddFunc(0xb96fb26e, cellPngDecGettRNS);
	cellPngDec->AddFunc(0xe163977f, cellPngDecGetPLTE);
	cellPngDec->AddFunc(0x609ec7d5, cellPngDecUnknownChunks);
	cellPngDec->AddFunc(0xb40ca175, cellPngDecGetTextChunk);*/
}
