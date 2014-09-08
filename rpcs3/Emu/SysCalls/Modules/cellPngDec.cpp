#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "stblib/stb_image.h"
#include "Emu/SysCalls/lv2/lv2Fs.h"
#include "cellPngDec.h"
#include <map>

//void cellPngDec_init();
//Module cellPngDec(0x0018, cellPngDec_init);
Module *cellPngDec = nullptr;

#ifdef PRX_DEBUG
#include "prx_libpngdec.h"
u32 libpngdec;
u32 libpngdec_rtoc;
#endif

u32 pngDecCreate(
	vm::ptr<const CellPngDecThreadInParam> param,
	vm::ptr<const CellPngDecExtThreadInParam> ext = {})
{
	// alloc memory (should probably use param->cbCtrlMallocFunc)
	auto dec = CellPngDecMainHandle::make(Memory.Alloc(sizeof(PngDecoder), 128));

	if (!dec)
	{
		throw hle::error(CELL_PNGDEC_ERROR_FATAL, "Memory allocation failed");
	}

	// initialize decoder
	dec->malloc = param->cbCtrlMallocFunc;
	dec->malloc_arg = param->cbCtrlMallocArg;
	dec->free = param->cbCtrlFreeFunc;
	dec->free_arg = param->cbCtrlFreeArg;

	if (ext)
	{
	}
	
	// use virtual memory address as a handle
	return dec.addr();
}

void pngDecDestroy(CellPngDecMainHandle dec)
{
	if (!Memory.Free(dec.addr()))
	{
		throw hle::error(CELL_PNGDEC_ERROR_FATAL, "Memory deallocation failed");
	}
}

u32 pngDecOpen(
	CellPngDecMainHandle dec,
	vm::ptr<const CellPngDecSrc> src,
	vm::ptr<const CellPngDecCbCtrlStrm> cb = {},
	vm::ptr<const CellPngDecOpnParam> param = {})
{
	// alloc memory (should probably use dec->malloc)
	auto stream = CellPngDecSubHandle::make(Memory.Alloc(sizeof(PngStream), 128));

	if (!stream)
	{
		throw hle::error(CELL_PNGDEC_ERROR_FATAL, "Memory allocation failed");
	}
	
	// initialize stream
	stream->fd = 0;
	stream->src = *src;

	switch (src->srcSelect.ToBE())
	{
	case se32(CELL_PNGDEC_BUFFER):
		stream->fileSize = src->streamSize.ToLE();
		break;

	case se32(CELL_PNGDEC_FILE):
		// Get file descriptor
		vm::var<be_t<u32>> fd;
		int ret = cellFsOpen(vm::ptr<const char>::make(src->fileName.addr()), 0, fd, vm::ptr<be_t<u32>>::make(0), 0);
		stream->fd = fd->ToLE();
		if (ret != CELL_OK) return CELL_PNGDEC_ERROR_OPEN_FILE;

		// Get size of file
		vm::var<CellFsStat> sb; // Alloc a CellFsStat struct
		ret = cellFsFstat(stream->fd, sb);
		if (ret != CELL_OK) return ret;
		stream->fileSize = sb->st_size;	// Get CellFsStat.st_size
		break;
	}

	if (cb)
	{
		// TODO: callback
	}

	if (param)
	{
		// TODO: param->selectChunk
	}

	// use virtual memory address as a handle
	return stream.addr();
}

void pngDecClose(CellPngDecSubHandle stream)
{
	cellFsClose(stream->fd);
	if (!Memory.Free(stream.addr()))
	{
		throw hle::error(CELL_PNGDEC_ERROR_FATAL, "Memory deallocation failed");
	}
}

void pngReadHeader(
	CellPngDecSubHandle stream,
	vm::ptr<CellPngDecInfo> info,
	vm::ptr<CellPngDecExtInfo> extInfo = {})
{
	CellPngDecInfo& current_info = stream->info;

	if (stream->fileSize < 29)
	{
		throw hle::error(CELL_PNGDEC_ERROR_HEADER, "The file is smaller than the length of a PNG header");
	}

	//Write the header to buffer
	vm::var<u8[34]> buffer; // Alloc buffer for PNG header
	auto buffer_32 = buffer.To<be_t<u32>>();
	vm::var<be_t<u64>> pos, nread;

	switch (stream->src.srcSelect.ToBE())
	{
	case se32(CELL_PNGDEC_BUFFER):
		memmove(buffer.begin(), stream->src.streamPtr.get_ptr(), buffer.size());
		break;
	case se32(CELL_PNGDEC_FILE):
		cellFsLseek(stream->fd, 0, CELL_SEEK_SET, pos);
		cellFsRead(stream->fd, vm::ptr<void>::make(buffer.addr()), buffer.size(), nread);
		break;
	}

	if (buffer_32[0].ToBE() != se32(0x89504E47) ||
		buffer_32[1].ToBE() != se32(0x0D0A1A0A) ||  // Error: The first 8 bytes are not a valid PNG signature
		buffer_32[3].ToBE() != se32(0x49484452))   // Error: The PNG file does not start with an IHDR chunk
	{
		throw hle::error(CELL_PNGDEC_ERROR_HEADER, "Invalid PNG header");
	}

	switch (buffer[25])
	{
	case 0: current_info.colorSpace = CELL_PNGDEC_GRAYSCALE;       current_info.numComponents = 1; break;
	case 2: current_info.colorSpace = CELL_PNGDEC_RGB;             current_info.numComponents = 3; break;
	case 3: current_info.colorSpace = CELL_PNGDEC_PALETTE;         current_info.numComponents = 1; break;
	case 4: current_info.colorSpace = CELL_PNGDEC_GRAYSCALE_ALPHA; current_info.numComponents = 2; break;
	case 6: current_info.colorSpace = CELL_PNGDEC_RGBA;            current_info.numComponents = 4; break;
	default: throw hle::error(CELL_PNGDEC_ERROR_HEADER, "Unknown color space (%d)");  return;
	}

	current_info.imageWidth = buffer_32[4];
	current_info.imageHeight = buffer_32[5];
	current_info.bitDepth = buffer[24];
	current_info.interlaceMethod = (CellPngDecInterlaceMode)buffer[28];
	current_info.chunkInformation = 0; // Unimplemented

	*info = current_info;
}

void pngDecSetParameter(
	CellPngDecSubHandle stream,
	vm::ptr<const CellPngDecInParam> inParam,
	vm::ptr<CellPngDecOutParam> outParam,
	vm::ptr<const CellPngDecExtInParam> extInParam = {},
	vm::ptr<CellPngDecExtOutParam> extOutParam = {})
{
	CellPngDecInfo& current_info = stream->info;
	CellPngDecOutParam& current_outParam = stream->outParam;

	current_outParam.outputWidthByte = (current_info.imageWidth * current_info.numComponents * current_info.bitDepth) / 8;
	current_outParam.outputWidth = current_info.imageWidth;
	current_outParam.outputHeight = current_info.imageHeight;
	current_outParam.outputColorSpace = inParam->outputColorSpace;

	switch ((u32)current_outParam.outputColorSpace)
	{
	case CELL_PNGDEC_PALETTE:
	case CELL_PNGDEC_GRAYSCALE:       current_outParam.outputComponents = 1; break;

	case CELL_PNGDEC_GRAYSCALE_ALPHA: current_outParam.outputComponents = 2; break;

	case CELL_PNGDEC_RGB:             current_outParam.outputComponents = 3; break;

	case CELL_PNGDEC_RGBA:
	case CELL_PNGDEC_ARGB:            current_outParam.outputComponents = 4; break;

	default: throw hle::error(CELL_PNGDEC_ERROR_ARG, fmt::Format("Unknown color space (%d)", (u32)current_outParam.outputColorSpace).c_str());
	}

	current_outParam.outputBitDepth = inParam->outputBitDepth;
	current_outParam.outputMode = inParam->outputMode;
	current_outParam.useMemorySpace = 0; // Unimplemented

	*outParam = current_outParam;
}

void pngDecodeData(
	CellPngDecSubHandle stream,
	vm::ptr<u8> data,
	vm::ptr<const CellPngDecDataCtrlParam> dataCtrlParam,
	vm::ptr<CellPngDecDataOutInfo> dataOutInfo,
	vm::ptr<const CellPngDecCbCtrlDisp> cbCtrlDisp = {},
	vm::ptr<CellPngDecDispParam> dispParam = {})
{
	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_STOP;

	const u32& fd = stream->fd;
	const u64& fileSize = stream->fileSize;
	const CellPngDecOutParam& current_outParam = stream->outParam;

	//Copy the PNG file to a buffer
	vm::var<unsigned char[]> png((u32)fileSize);
	vm::var<be_t<u64>> pos, nread;

	switch (stream->src.srcSelect.ToBE())
	{
	case se32(CELL_PNGDEC_BUFFER):
		memmove(png.begin(), stream->src.streamPtr.get_ptr(), png.size());
		break;

	case se32(CELL_PNGDEC_FILE):
		cellFsLseek(fd, 0, CELL_SEEK_SET, pos);
		cellFsRead(fd, vm::ptr<void>::make(png.addr()), png.size(), nread);
		break;
	}

	//Decode PNG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	auto image = std::unique_ptr<unsigned char, decltype(&::free)>
		(
		stbi_load_from_memory(png.ptr(), (s32)fileSize, &width, &height, &actual_components, 4),
		&::free
		);
	if (!image)	throw hle::error(CELL_PNGDEC_ERROR_STREAM_FORMAT);

	const bool flip = current_outParam.outputMode == CELL_PNGDEC_BOTTOM_TO_TOP;
	const int bytesPerLine = (u32)dataCtrlParam->outputBytesPerLine;
	uint image_size = width * height;

	switch ((u32)current_outParam.outputColorSpace)
	{
	case CELL_PNGDEC_RGB:
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
				memcpy(&data[dstOffset], &image.get()[srcOffset], linesize);
			}
		}
		else
		{
			memcpy(data.get_ptr(), image.get(), image_size);
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
			char *output = (char *)malloc(linesize);
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

	case CELL_PNGDEC_GRAYSCALE:
	case CELL_PNGDEC_PALETTE:
	case CELL_PNGDEC_GRAYSCALE_ALPHA:
		cellPngDec->Error("cellPngDecDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace.ToLE());
		break;

	default:
		throw hle::error(CELL_PNGDEC_ERROR_ARG);
	}

	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_FINISH;
}

s32 cellPngDecCreate(vm::ptr<u32> mainHandle, vm::ptr<const CellPngDecThreadInParam> threadInParam, vm::ptr<CellPngDecThreadOutParam> threadOutParam)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x295C, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecCreate(mainHandle_addr=0x%x, threadInParam_addr=0x%x, threadOutParam_addr=0x%x)",
		mainHandle.addr(), threadInParam.addr(), threadOutParam.addr());

	try
	{
		// create decoder
		*mainHandle = pngDecCreate(threadInParam);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	// set codec version
	threadOutParam->pngCodecVersion = PNGDEC_CODEC_VERSION;

	return CELL_OK;
#endif
}

s32 cellPngDecExtCreate(
	vm::ptr<u32> mainHandle,
	vm::ptr<const CellPngDecThreadInParam> threadInParam,
	vm::ptr<CellPngDecThreadOutParam> threadOutParam,
	vm::ptr<const CellPngDecExtThreadInParam> extThreadInParam,
	vm::ptr<CellPngDecExtThreadOutParam> extThreadOutParam)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x296C, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecCreate(mainHandle_addr=0x%x, threadInParam_addr=0x%x, threadOutParam_addr=0x%x, extThreadInParam_addr=0x%x, extThreadOutParam_addr=0x%x)",
		mainHandle.addr(), threadInParam.addr(), threadOutParam.addr(), extThreadInParam.addr(), extThreadOutParam.addr());

	try
	{
		// create decoder
		*mainHandle = pngDecCreate(threadInParam, extThreadInParam);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	// set codec version
	threadOutParam->pngCodecVersion = PNGDEC_CODEC_VERSION;

	extThreadOutParam->reserved = 0;

	return CELL_OK;
#endif
}

s32 cellPngDecDestroy(CellPngDecMainHandle mainHandle)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x1E6C, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecDestroy(mainHandle=0x%x)", mainHandle.addr());

	try
	{
		pngDecDestroy(mainHandle);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	return CELL_OK;
#endif
}

s32 cellPngDecOpen(
	CellPngDecMainHandle mainHandle,
	vm::ptr<u32> subHandle,
	vm::ptr<const CellPngDecSrc> src,
	vm::ptr<CellPngDecOpnInfo> openInfo)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x3F3C, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecOpen(mainHandle=0x%x, subHandle_addr=0x%x, src_addr=0x%x, openInfo_addr=0x%x)",
		mainHandle.addr(), subHandle.addr(), src.addr(), openInfo.addr());

	try
	{
		// create stream handle
		*subHandle = pngDecOpen(mainHandle, src);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	// set memory info
	openInfo->initSpaceAllocated = 4096;

	return CELL_OK;
#endif
}

s32 cellPngDecExtOpen(
	CellPngDecMainHandle mainHandle,
	vm::ptr<u32> subHandle,
	vm::ptr<const CellPngDecSrc> src,
	vm::ptr<CellPngDecOpnInfo> openInfo,
	vm::ptr<const CellPngDecCbCtrlStrm> cbCtrlStrm,
	vm::ptr<const CellPngDecOpnParam> opnParam)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x3F34, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecExtOpen(mainHandle=0x%x, subHandle_addr=0x%x, src_addr=0x%x, openInfo_addr=0x%x, cbCtrlStrm_addr=0x%x, opnParam_addr=0x%x)",
		mainHandle.addr(), subHandle.addr(), src.addr(), openInfo.addr(), cbCtrlStrm.addr(), opnParam.addr());

	try
	{
		// create stream handle
		*subHandle = pngDecOpen(mainHandle, src, cbCtrlStrm, opnParam);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	// set memory info
	openInfo->initSpaceAllocated = 4096;

	return CELL_OK;
#endif
}

s32 cellPngDecClose(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x066C, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecClose(mainHandle=0x%x, subHandle=0x%x)", mainHandle.addr(), subHandle.addr());

	try
	{
		pngDecClose(subHandle);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	return CELL_OK;
#endif
}

s32 cellPngDecReadHeader(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngDecInfo> info)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x3A3C, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecReadHeader(mainHandle=0x%x, subHandle=0x%x, info_addr=0x%x)",
		mainHandle.addr(), subHandle.addr(), info.addr());

	try
	{
		pngReadHeader(subHandle, info);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	return CELL_OK;
#endif
}

s32 cellPngDecExtReadHeader(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<CellPngDecInfo> info,
	vm::ptr<CellPngDecExtInfo> extInfo)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x3A34, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecExtReadHeader(mainHandle=0x%x, subHandle=0x%x, info_addr=0x%x, extInfo_addr=0x%x)",
		mainHandle.addr(), subHandle.addr(), info.addr(), extInfo.addr());

	try
	{
		pngReadHeader(subHandle, info, extInfo);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	return CELL_OK;
#endif
}

s32 cellPngDecSetParameter(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<const CellPngDecInParam> inParam,
	vm::ptr<CellPngDecOutParam> outParam)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x33F4, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecSetParameter(mainHandle=0x%x, subHandle=0x%x, inParam_addr=0x%x, outParam_addr=0x%x)",
		mainHandle.addr(), subHandle.addr(), inParam.addr(), outParam.addr());

	try
	{
		pngDecSetParameter(subHandle, inParam, outParam);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	return CELL_OK;
#endif
}

s32 cellPngDecExtSetParameter(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<const CellPngDecInParam> inParam,
	vm::ptr<CellPngDecOutParam> outParam,
	vm::ptr<const CellPngDecExtInParam> extInParam,
	vm::ptr<CellPngDecExtOutParam> extOutParam)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x33EC, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecExtSetParameter(mainHandle=0x%x, subHandle=0x%x, inParam_addr=0x%x, outParam_addr=0x%x, extInParam=0x%x, extOutParam=0x%x",
		mainHandle.addr(), subHandle.addr(), inParam.addr(), outParam.addr(), extInParam.addr(), extOutParam.addr());

	try
	{
		pngDecSetParameter(subHandle, inParam, outParam, extInParam, extOutParam);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	return CELL_OK;
#endif
}

s32 cellPngDecDecodeData(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<u8> data,
	vm::ptr<const CellPngDecDataCtrlParam> dataCtrlParam,
	vm::ptr<CellPngDecDataOutInfo> dataOutInfo)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x5D40, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecDecodeData(mainHandle=0x%x, subHandle=0x%x, data_addr=0x%x, dataCtrlParam_addr=0x%x, dataOutInfo_addr=0x%x)",
		mainHandle.addr(), subHandle.addr(), data.addr(), dataCtrlParam.addr(), dataOutInfo.addr());

	try
	{
		pngDecodeData(subHandle, data, dataCtrlParam, dataOutInfo);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	return CELL_OK;
#endif
}

s32 cellPngDecExtDecodeData(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<u8> data,
	vm::ptr<const CellPngDecDataCtrlParam> dataCtrlParam,
	vm::ptr<CellPngDecDataOutInfo> dataOutInfo,
	vm::ptr<const CellPngDecCbCtrlDisp> cbCtrlDisp,
	vm::ptr<CellPngDecDispParam> dispParam)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x5D38, libpngdec_rtoc);
#else
	cellPngDec->Warning("cellPngDecExtDecodeData(mainHandle=0x%x, subHandle=0x%x, data_addr=0x%x, dataCtrlParam_addr=0x%x, dataOutInfo_addr=0x%x, cbCtrlDisp_addr=0x%x, dispParam_addr=0x%x)",
		mainHandle.addr(), subHandle.addr(), data.addr(), dataCtrlParam.addr(), dataOutInfo.addr(), cbCtrlDisp.addr(), dispParam.addr());

	try
	{
		pngDecodeData(subHandle, data, dataCtrlParam, dataOutInfo, cbCtrlDisp, dispParam);
	}
	catch (hle::error& e)
	{
		e.print(__FUNCTION__);
		return e.code;
	}

	return CELL_OK;
#endif
}

s32 cellPngDecGetUnknownChunks(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<vm::bptr<CellPngUnknownChunk>> unknownChunk,
	vm::ptr<u32> unknownChunkNumber)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x03EC, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetpCAL(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngPCAL> pcal)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x0730, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetcHRM(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngCHRM> chrm)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x0894, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetsCAL(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSCAL> scal)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x09EC, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetpHYs(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngPHYS> phys)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x0B14, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetoFFs(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngOFFS> offs)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x0C58, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetsPLT(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSPLT> splt)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x0D9C, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetbKGD(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngBKGD> bkgd)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x0ED0, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGettIME(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngTIME> time)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x1024, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGethIST(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngHIST> hist)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x116C, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGettRNS(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngTRNS> trns)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x12A4, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetsBIT(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSBIT> sbit)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x1420, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetiCCP(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngICCP> iccp)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x1574, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetsRGB(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSRGB> srgb)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x16B4, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetgAMA(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngGAMA> gama)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x17CC, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetPLTE(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngPLTE> plte)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x18E4, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

s32 cellPngDecGetTextChunk(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<u32> textInfoNum,
	vm::ptr<vm::bptr<CellPngTextInfo>> textInfo)
{
#ifdef PRX_DEBUG
	cellPngDec->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libpngdec + 0x19FC, libpngdec_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
#endif
}

void cellPngDec_init()
{
	REG_FUNC(cellPngDec, cellPngDecGetUnknownChunks);
	REG_FUNC(cellPngDec, cellPngDecClose);
	REG_FUNC(cellPngDec, cellPngDecGetpCAL);
	REG_FUNC(cellPngDec, cellPngDecGetcHRM);
	REG_FUNC(cellPngDec, cellPngDecGetsCAL);
	REG_FUNC(cellPngDec, cellPngDecGetpHYs);
	REG_FUNC(cellPngDec, cellPngDecGetoFFs);
	REG_FUNC(cellPngDec, cellPngDecGetsPLT);
	REG_FUNC(cellPngDec, cellPngDecGetbKGD);
	REG_FUNC(cellPngDec, cellPngDecGettIME);
	REG_FUNC(cellPngDec, cellPngDecGethIST);
	REG_FUNC(cellPngDec, cellPngDecGettRNS);
	REG_FUNC(cellPngDec, cellPngDecGetsBIT);
	REG_FUNC(cellPngDec, cellPngDecGetiCCP);
	REG_FUNC(cellPngDec, cellPngDecGetsRGB);
	REG_FUNC(cellPngDec, cellPngDecGetgAMA);
	REG_FUNC(cellPngDec, cellPngDecGetPLTE);
	REG_FUNC(cellPngDec, cellPngDecGetTextChunk);
	REG_FUNC(cellPngDec, cellPngDecDestroy);
	REG_FUNC(cellPngDec, cellPngDecCreate);
	REG_FUNC(cellPngDec, cellPngDecExtCreate);
	REG_FUNC(cellPngDec, cellPngDecExtSetParameter);
	REG_FUNC(cellPngDec, cellPngDecSetParameter);
	REG_FUNC(cellPngDec, cellPngDecExtReadHeader);
	REG_FUNC(cellPngDec, cellPngDecReadHeader);
	REG_FUNC(cellPngDec, cellPngDecExtOpen);
	REG_FUNC(cellPngDec, cellPngDecOpen);
	REG_FUNC(cellPngDec, cellPngDecExtDecodeData);
	REG_FUNC(cellPngDec, cellPngDecDecodeData);

#ifdef PRX_DEBUG
	CallAfter([]()
	{
		libpngdec = (u32)Memory.PRXMem.AllocAlign(sizeof(libpngdec_data), 4096);
		memcpy(vm::get_ptr<void>(libpngdec), libpngdec_data, sizeof(libpngdec_data));
		libpngdec_rtoc = libpngdec + 0x49710;

		extern Module* sysPrxForUser;
		extern Module* cellSpurs;
		extern Module* sys_fs;
		
		FIX_IMPORT(sysPrxForUser, _sys_snprintf                   , libpngdec + 0x1E6D0);
		FIX_IMPORT(sysPrxForUser, _sys_strlen                     , libpngdec + 0x1E6F0);
		fix_import(sysPrxForUser, 0x3EF17F8C                      , libpngdec + 0x1E710);
		FIX_IMPORT(sysPrxForUser, _sys_memset                     , libpngdec + 0x1E730);
		FIX_IMPORT(sysPrxForUser, _sys_memcpy                     , libpngdec + 0x1E750);
		FIX_IMPORT(sysPrxForUser, _sys_strcpy                     , libpngdec + 0x1E770);
		FIX_IMPORT(sysPrxForUser, _sys_strncpy                    , libpngdec + 0x1E790);
		FIX_IMPORT(sysPrxForUser, _sys_memcmp                     , libpngdec + 0x1E7B0);
		FIX_IMPORT(cellSpurs, cellSpursQueueDetachLv2EventQueue   , libpngdec + 0x1E7D0);
		FIX_IMPORT(cellSpurs, cellSpursAttributeSetNamePrefix     , libpngdec + 0x1E7F0);
		FIX_IMPORT(cellSpurs, _cellSpursQueueInitialize           , libpngdec + 0x1E810);
		FIX_IMPORT(cellSpurs, _cellSpursTasksetAttributeInitialize, libpngdec + 0x1E830);
		FIX_IMPORT(cellSpurs, cellSpursTasksetAttributeSetName    , libpngdec + 0x1E850);
		FIX_IMPORT(cellSpurs, cellSpursTaskGetReadOnlyAreaPattern , libpngdec + 0x1E870);
		FIX_IMPORT(cellSpurs, cellSpursTaskGetContextSaveAreaSize , libpngdec + 0x1E890);
		FIX_IMPORT(cellSpurs, cellSpursQueuePopBody               , libpngdec + 0x1E8B0);
		FIX_IMPORT(cellSpurs, cellSpursQueuePushBody              , libpngdec + 0x1E8D0);
		FIX_IMPORT(cellSpurs, _cellSpursAttributeInitialize       , libpngdec + 0x1E8F0);
		FIX_IMPORT(cellSpurs, cellSpursJoinTaskset                , libpngdec + 0x1E910);
		FIX_IMPORT(cellSpurs, cellSpursShutdownTaskset            , libpngdec + 0x1E930);
		FIX_IMPORT(cellSpurs, cellSpursInitializeWithAttribute    , libpngdec + 0x1E950);
		FIX_IMPORT(cellSpurs, cellSpursCreateTask                 , libpngdec + 0x1E970);
		FIX_IMPORT(cellSpurs, cellSpursCreateTasksetWithAttribute , libpngdec + 0x1E990);
		FIX_IMPORT(cellSpurs, cellSpursFinalize                   , libpngdec + 0x1E9B0);
		FIX_IMPORT(cellSpurs, cellSpursQueueAttachLv2EventQueue   , libpngdec + 0x1E9D0);
		FIX_IMPORT(sys_fs, cellFsClose                            , libpngdec + 0x1E9F0);
		FIX_IMPORT(sys_fs, cellFsRead                             , libpngdec + 0x1EA10);
		FIX_IMPORT(sys_fs, cellFsOpen                             , libpngdec + 0x1EA30);
		FIX_IMPORT(sys_fs, cellFsLseek                            , libpngdec + 0x1EA50);

		fix_relocs(cellPngDec, libpngdec, 0x41C30, 0x47AB0, 0x40A00);
	});
#endif
}
