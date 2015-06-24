#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"

extern "C"
{
#include "stblib/stb_image.h"
}

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFileBase.h"
#include "Emu/SysCalls/lv2/sys_fs.h"

#include "cellPngDec.h"

extern Module cellPngDec;

s32 pngDecCreate(
	vm::ptr<CellPngDecMainHandle> mainHandle,
	vm::cptr<CellPngDecThreadInParam> param,
	vm::cptr<CellPngDecExtThreadInParam> ext = vm::null)
{
	// alloc memory (should probably use param->cbCtrlMallocFunc)
	auto dec = CellPngDecMainHandle::make(Memory.Alloc(sizeof(PngDecoder), 128));

	if (!dec)
	{
		return CELL_PNGDEC_ERROR_FATAL;
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
	*mainHandle = dec;

	return CELL_OK;
}

s32 pngDecDestroy(CellPngDecMainHandle dec)
{
	if (!Memory.Free(dec.addr()))
	{
		return CELL_PNGDEC_ERROR_FATAL;
	}

	return CELL_OK;
}

s32 pngDecOpen(
	CellPngDecMainHandle dec,
	vm::ptr<CellPngDecSubHandle> subHandle,
	vm::cptr<CellPngDecSrc> src,
	vm::ptr<CellPngDecOpnInfo> openInfo,
	vm::cptr<CellPngDecCbCtrlStrm> cb = vm::null,
	vm::cptr<CellPngDecOpnParam> param = vm::null)
{
	// alloc memory (should probably use dec->malloc)
	auto stream = CellPngDecSubHandle::make(Memory.Alloc(sizeof(PngStream), 128));

	if (!stream)
	{
		return CELL_PNGDEC_ERROR_FATAL;
	}
	
	// initialize stream
	stream->fd = 0;
	stream->src = *src;

	switch (src->srcSelect.value())
	{
	case CELL_PNGDEC_BUFFER:
		stream->fileSize = src->streamSize;
		break;

	case CELL_PNGDEC_FILE:
	{
		// Get file descriptor and size
		std::shared_ptr<vfsStream> file_s(Emu.GetVFS().OpenFile(src->fileName.get_ptr(), vfsRead));
		if (!file_s) return CELL_PNGDEC_ERROR_OPEN_FILE;

		stream->fd = Emu.GetIdManager().make<lv2_file_t>(file_s, 0, 0);
		stream->fileSize = file_s->GetSize();
		break;
	}
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
	*subHandle = stream;

	// set memory info
	openInfo->initSpaceAllocated = 4096;

	return CELL_OK;
}

s32 pngDecClose(CellPngDecSubHandle stream)
{
	Emu.GetIdManager().remove<lv2_file_t>(stream->fd);

	if (!Memory.Free(stream.addr()))
	{
		return CELL_PNGDEC_ERROR_FATAL;
	}

	return CELL_OK;
}

s32 pngReadHeader(
	CellPngDecSubHandle stream,
	vm::ptr<CellPngDecInfo> info,
	vm::ptr<CellPngDecExtInfo> extInfo = vm::null)
{
	CellPngDecInfo& current_info = stream->info;

	if (stream->fileSize < 29)
	{
		return CELL_PNGDEC_ERROR_HEADER; // The file is smaller than the length of a PNG header
	}

	//Write the header to buffer
	vm::var<u8[34]> buffer; // Alloc buffer for PNG header
	auto buffer_32 = buffer.To<be_t<u32>>();

	switch (stream->src.srcSelect.value())
	{
	case CELL_PNGDEC_BUFFER:
		memmove(buffer.begin(), stream->src.streamPtr.get_ptr(), buffer.size());
		break;
	case CELL_PNGDEC_FILE:
	{
		auto file = Emu.GetIdManager().get<lv2_file_t>(stream->fd);
		file->file->Seek(0);
		file->file->Read(buffer.begin(), buffer.size());
		break;
	}
	}

	if (buffer_32[0] != 0x89504E47 ||
		buffer_32[1] != 0x0D0A1A0A ||  // Error: The first 8 bytes are not a valid PNG signature
		buffer_32[3] != 0x49484452)   // Error: The PNG file does not start with an IHDR chunk
	{
		return CELL_PNGDEC_ERROR_HEADER;
	}

	switch (buffer[25])
	{
	case 0: current_info.colorSpace = CELL_PNGDEC_GRAYSCALE;       current_info.numComponents = 1; break;
	case 2: current_info.colorSpace = CELL_PNGDEC_RGB;             current_info.numComponents = 3; break;
	case 3: current_info.colorSpace = CELL_PNGDEC_PALETTE;         current_info.numComponents = 1; break;
	case 4: current_info.colorSpace = CELL_PNGDEC_GRAYSCALE_ALPHA; current_info.numComponents = 2; break;
	case 6: current_info.colorSpace = CELL_PNGDEC_RGBA;            current_info.numComponents = 4; break;
	default:
		cellPngDec.Error("cellPngDecDecodeData: Unsupported color space (%d)", (u32)buffer[25]);
		return CELL_PNGDEC_ERROR_HEADER;
	}

	current_info.imageWidth = buffer_32[4];
	current_info.imageHeight = buffer_32[5];
	current_info.bitDepth = buffer[24];
	current_info.interlaceMethod = (CellPngDecInterlaceMode)buffer[28];
	current_info.chunkInformation = 0; // Unimplemented

	*info = current_info;

	if (extInfo)
	{
		extInfo->reserved = 0;
	}

	return CELL_OK;
}

s32 pngDecSetParameter(
	CellPngDecSubHandle stream,
	vm::cptr<CellPngDecInParam> inParam,
	vm::ptr<CellPngDecOutParam> outParam,
	vm::cptr<CellPngDecExtInParam> extInParam = vm::null,
	vm::ptr<CellPngDecExtOutParam> extOutParam = vm::null)
{
	CellPngDecInfo& current_info = stream->info;
	CellPngDecOutParam& current_outParam = stream->outParam;

	current_outParam.outputWidthByte = (current_info.imageWidth * current_info.numComponents * current_info.bitDepth) / 8;
	current_outParam.outputWidth = current_info.imageWidth;
	current_outParam.outputHeight = current_info.imageHeight;
	current_outParam.outputColorSpace = inParam->outputColorSpace;

	switch (current_outParam.outputColorSpace.value())
	{
	case CELL_PNGDEC_PALETTE:
	case CELL_PNGDEC_GRAYSCALE:
		current_outParam.outputComponents = 1; break;

	case CELL_PNGDEC_GRAYSCALE_ALPHA:
		current_outParam.outputComponents = 2; break;

	case CELL_PNGDEC_RGB:
		current_outParam.outputComponents = 3; break;

	case CELL_PNGDEC_RGBA:
	case CELL_PNGDEC_ARGB:
		current_outParam.outputComponents = 4; break;

	default:
		cellPngDec.Error("pngDecSetParameter: Unsupported color space (%d)", current_outParam.outputColorSpace);
		return CELL_PNGDEC_ERROR_ARG;
	}

	current_outParam.outputBitDepth = inParam->outputBitDepth;
	current_outParam.outputMode = inParam->outputMode;
	current_outParam.useMemorySpace = 0; // Unimplemented

	*outParam = current_outParam;

	return CELL_OK;
}

s32 pngDecodeData(
	CellPngDecSubHandle stream,
	vm::ptr<u8> data,
	vm::cptr<CellPngDecDataCtrlParam> dataCtrlParam,
	vm::ptr<CellPngDecDataOutInfo> dataOutInfo,
	vm::cptr<CellPngDecCbCtrlDisp> cbCtrlDisp = vm::null,
	vm::ptr<CellPngDecDispParam> dispParam = vm::null)
{
	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_STOP;

	const u32& fd = stream->fd;
	const u64& fileSize = stream->fileSize;
	const CellPngDecOutParam& current_outParam = stream->outParam;

	//Copy the PNG file to a buffer
	vm::var<unsigned char[]> png((u32)fileSize);

	switch (stream->src.srcSelect.value())
	{
	case CELL_PNGDEC_BUFFER:
		memmove(png.begin(), stream->src.streamPtr.get_ptr(), png.size());
		break;

	case CELL_PNGDEC_FILE:
	{
		auto file = Emu.GetIdManager().get<lv2_file_t>(stream->fd);
		file->file->Seek(0);
		file->file->Read(png.ptr(), png.size());
		break;
	}
	}

	//Decode PNG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	auto image = std::unique_ptr<unsigned char, decltype(&::free)>
		(
		stbi_load_from_memory(png.ptr(), (s32)fileSize, &width, &height, &actual_components, 4),
		&::free
		);
	if (!image)
	{
		cellPngDec.Error("pngDecodeData: stbi_load_from_memory failed");
		return CELL_PNGDEC_ERROR_STREAM_FORMAT;
	}

	const bool flip = current_outParam.outputMode == CELL_PNGDEC_BOTTOM_TO_TOP;
	const int bytesPerLine = (u32)dataCtrlParam->outputBytesPerLine;
	uint image_size = width * height;

	switch (current_outParam.outputColorSpace.value())
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
		break;
	}

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
		break;
	}

	case CELL_PNGDEC_GRAYSCALE:
	case CELL_PNGDEC_PALETTE:
	case CELL_PNGDEC_GRAYSCALE_ALPHA:
		cellPngDec.Error("pngDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace);
		break;

	default:
		cellPngDec.Error("pngDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace);
		return CELL_PNGDEC_ERROR_ARG;
	}

	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_FINISH;

	return CELL_OK;
}

s32 cellPngDecCreate(
	vm::ptr<CellPngDecMainHandle> mainHandle,
	vm::cptr<CellPngDecThreadInParam> threadInParam,
	vm::ptr<CellPngDecThreadOutParam> threadOutParam)
{
	cellPngDec.Warning("cellPngDecCreate(mainHandle=**0x%x, threadInParam=*0x%x, threadOutParam=*0x%x)", mainHandle, threadInParam, threadOutParam);

	// create decoder
	if (auto res = pngDecCreate(mainHandle, threadInParam)) return res;

	// set codec version
	threadOutParam->pngCodecVersion = PNGDEC_CODEC_VERSION;

	return CELL_OK;
}

s32 cellPngDecExtCreate(
	vm::ptr<CellPngDecMainHandle> mainHandle,
	vm::cptr<CellPngDecThreadInParam> threadInParam,
	vm::ptr<CellPngDecThreadOutParam> threadOutParam,
	vm::cptr<CellPngDecExtThreadInParam> extThreadInParam,
	vm::ptr<CellPngDecExtThreadOutParam> extThreadOutParam)
{
	cellPngDec.Warning("cellPngDecCreate(mainHandle=**0x%x, threadInParam=*0x%x, threadOutParam=*0x%x, extThreadInParam=*0x%x, extThreadOutParam=*0x%x)",
		mainHandle, threadInParam, threadOutParam, extThreadInParam, extThreadOutParam);

	// create decoder
	if (auto res = pngDecCreate(mainHandle, threadInParam, extThreadInParam)) return res;

	// set codec version
	threadOutParam->pngCodecVersion = PNGDEC_CODEC_VERSION;

	extThreadOutParam->reserved = 0;

	return CELL_OK;
}

s32 cellPngDecDestroy(CellPngDecMainHandle mainHandle)
{
	cellPngDec.Warning("cellPngDecDestroy(mainHandle=*0x%x)", mainHandle);

	// destroy decoder
	return pngDecDestroy(mainHandle);
}

s32 cellPngDecOpen(
	CellPngDecMainHandle mainHandle,
	vm::ptr<CellPngDecSubHandle> subHandle,
	vm::cptr<CellPngDecSrc> src,
	vm::ptr<CellPngDecOpnInfo> openInfo)
{
	cellPngDec.Warning("cellPngDecOpen(mainHandle=*0x%x, subHandle=**0x%x, src=*0x%x, openInfo=*0x%x)", mainHandle, subHandle, src, openInfo);

	// create stream handle
	return pngDecOpen(mainHandle, subHandle, src, openInfo);
}

s32 cellPngDecExtOpen(
	CellPngDecMainHandle mainHandle,
	vm::ptr<CellPngDecSubHandle> subHandle,
	vm::cptr<CellPngDecSrc> src,
	vm::ptr<CellPngDecOpnInfo> openInfo,
	vm::cptr<CellPngDecCbCtrlStrm> cbCtrlStrm,
	vm::cptr<CellPngDecOpnParam> opnParam)
{
	cellPngDec.Warning("cellPngDecExtOpen(mainHandle=*0x%x, subHandle=**0x%x, src=*0x%x, openInfo=*0x%x, cbCtrlStrm=*0x%x, opnParam=*0x%x)", mainHandle, subHandle, src, openInfo, cbCtrlStrm, opnParam);

	// create stream handle
	return pngDecOpen(mainHandle, subHandle, src, openInfo, cbCtrlStrm, opnParam);
}

s32 cellPngDecClose(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle)
{
	cellPngDec.Warning("cellPngDecClose(mainHandle=*0x%x, subHandle=*0x%x)", mainHandle, subHandle);

	return pngDecClose(subHandle);
}

s32 cellPngDecReadHeader(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngDecInfo> info)
{
	cellPngDec.Warning("cellPngDecReadHeader(mainHandle=*0x%x, subHandle=*0x%x, info=*0x%x)", mainHandle, subHandle, info);

	return pngReadHeader(subHandle, info);
}

s32 cellPngDecExtReadHeader(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<CellPngDecInfo> info,
	vm::ptr<CellPngDecExtInfo> extInfo)
{
	cellPngDec.Warning("cellPngDecExtReadHeader(mainHandle=*0x%x, subHandle=*0x%x, info=*0x%x, extInfo=*0x%x)", mainHandle, subHandle, info, extInfo);

	return pngReadHeader(subHandle, info, extInfo);
}

s32 cellPngDecSetParameter(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::cptr<CellPngDecInParam> inParam,
	vm::ptr<CellPngDecOutParam> outParam)
{
	cellPngDec.Warning("cellPngDecSetParameter(mainHandle=*0x%x, subHandle=*0x%x, inParam=*0x%x, outParam=*0x%x)", mainHandle, subHandle, inParam, outParam);

	return pngDecSetParameter(subHandle, inParam, outParam);
}

s32 cellPngDecExtSetParameter(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::cptr<CellPngDecInParam> inParam,
	vm::ptr<CellPngDecOutParam> outParam,
	vm::cptr<CellPngDecExtInParam> extInParam,
	vm::ptr<CellPngDecExtOutParam> extOutParam)
{
	cellPngDec.Warning("cellPngDecExtSetParameter(mainHandle=*0x%x, subHandle=*0x%x, inParam=*0x%x, outParam=*0x%x, extInParam=*0x%x, extOutParam=*0x%x", mainHandle, subHandle, inParam, outParam, extInParam, extOutParam);

	return pngDecSetParameter(subHandle, inParam, outParam, extInParam, extOutParam);
}

s32 cellPngDecDecodeData(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<u8> data,
	vm::cptr<CellPngDecDataCtrlParam> dataCtrlParam,
	vm::ptr<CellPngDecDataOutInfo> dataOutInfo)
{
	cellPngDec.Warning("cellPngDecDecodeData(mainHandle=*0x%x, subHandle=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x)",
		mainHandle, subHandle, data, dataCtrlParam, dataOutInfo);

	return pngDecodeData(subHandle, data, dataCtrlParam, dataOutInfo);
}

s32 cellPngDecExtDecodeData(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<u8> data,
	vm::cptr<CellPngDecDataCtrlParam> dataCtrlParam,
	vm::ptr<CellPngDecDataOutInfo> dataOutInfo,
	vm::cptr<CellPngDecCbCtrlDisp> cbCtrlDisp,
	vm::ptr<CellPngDecDispParam> dispParam)
{
	cellPngDec.Warning("cellPngDecExtDecodeData(mainHandle=*0x%x, subHandle=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x, cbCtrlDisp=*0x%x, dispParam=*0x%x)",
		mainHandle, subHandle, data, dataCtrlParam, dataOutInfo, cbCtrlDisp, dispParam);

	return pngDecodeData(subHandle, data, dataCtrlParam, dataOutInfo, cbCtrlDisp, dispParam);
}

s32 cellPngDecGetUnknownChunks(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::pptr<CellPngUnknownChunk> unknownChunk,
	vm::ptr<u32> unknownChunkNumber)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetpCAL(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngPCAL> pcal)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetcHRM(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngCHRM> chrm)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetsCAL(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSCAL> scal)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetpHYs(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngPHYS> phys)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetoFFs(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngOFFS> offs)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetsPLT(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSPLT> splt)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetbKGD(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngBKGD> bkgd)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGettIME(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngTIME> time)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGethIST(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngHIST> hist)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGettRNS(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngTRNS> trns)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetsBIT(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSBIT> sbit)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetiCCP(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngICCP> iccp)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetsRGB(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSRGB> srgb)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetgAMA(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngGAMA> gama)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetPLTE(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngPLTE> plte)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetTextChunk(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<u32> textInfoNum,
	vm::pptr<CellPngTextInfo> textInfo)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

Module cellPngDec("cellPngDec", []()
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
});
