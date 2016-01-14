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

extern Module<> cellPngDec;

// cellPngDec aliases (only for cellPngDec.cpp)
using PPMainHandle = vm::pptr<PngDecoder>;
using PMainHandle = vm::ptr<PngDecoder>;
using PThreadInParam = vm::cptr<CellPngDecThreadInParam>;
using PThreadOutParam = vm::ptr<CellPngDecThreadOutParam>;
using PExtThreadInParam = vm::cptr<CellPngDecExtThreadInParam>;
using PExtThreadOutParam = vm::ptr<CellPngDecExtThreadOutParam>;
using PPSubHandle = vm::pptr<PngStream>;
using PSubHandle = vm::ptr<PngStream>;
using PSrc = vm::cptr<CellPngDecSrc>;
using POpenInfo = vm::ptr<CellPngDecOpnInfo>;
using PInfo = vm::ptr<CellPngDecInfo>;
using PExtInfo = vm::ptr<CellPngDecExtInfo>;
using PInParam = vm::cptr<CellPngDecInParam>;
using POutParam = vm::ptr<CellPngDecOutParam>;
using PExtInParam = vm::cptr<CellPngDecExtInParam>;
using PExtOutParam = vm::ptr<CellPngDecExtOutParam>;
using PDataCtrlParam = vm::cptr<CellPngDecDataCtrlParam>;
using PDataOutInfo = vm::ptr<CellPngDecDataOutInfo>;
using PCbCtrlDisp = vm::cptr<CellPngDecCbCtrlDisp>;
using PDispParam = vm::ptr<CellPngDecDispParam>;

s32 pngDecCreate(PPMainHandle mainHandle, PThreadInParam param, PExtThreadInParam ext = vm::null)
{
	// alloc memory (should probably use param->cbCtrlMallocFunc)
	auto dec = vm::ptr<PngDecoder>::make(vm::alloc(sizeof(PngDecoder), vm::main));

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

s32 pngDecDestroy(PMainHandle dec)
{
	if (!vm::dealloc(dec.addr(), vm::main))
	{
		return CELL_PNGDEC_ERROR_FATAL;
	}

	return CELL_OK;
}

s32 pngDecOpen(PMainHandle dec, PPSubHandle subHandle, PSrc src, POpenInfo openInfo, vm::cptr<CellPngDecCbCtrlStrm> cb = vm::null, vm::cptr<CellPngDecOpnParam> param = vm::null)
{
	// alloc memory (should probably use dec->malloc)
	auto stream = vm::ptr<PngStream>::make(vm::alloc(sizeof(PngStream), vm::main));

	if (!stream)
	{
		return CELL_PNGDEC_ERROR_FATAL;
	}
	
	// initialize stream
	stream->fd = 0;
	stream->src = *src;

	switch (src->srcSelect)
	{
	case CELL_PNGDEC_BUFFER:
		stream->fileSize = src->streamSize;
		break;

	case CELL_PNGDEC_FILE:
	{
		// Get file descriptor and size
		std::shared_ptr<vfsStream> file_s(Emu.GetVFS().OpenFile(src->fileName.get_ptr(), fom::read));
		if (!file_s) return CELL_PNGDEC_ERROR_OPEN_FILE;

		stream->fd = idm::make<lv2_file_t>(file_s, 0, 0);
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

s32 pngDecClose(PSubHandle stream)
{
	idm::remove<lv2_file_t>(stream->fd);

	if (!vm::dealloc(stream.addr(), vm::main))
	{
		return CELL_PNGDEC_ERROR_FATAL;
	}

	return CELL_OK;
}

s32 pngReadHeader(PSubHandle stream, vm::ptr<CellPngDecInfo> info, PExtInfo extInfo = vm::null)
{
	CellPngDecInfo& current_info = stream->info;

	if (stream->fileSize < 29)
	{
		return CELL_PNGDEC_ERROR_HEADER; // The file is smaller than the length of a PNG header
	}

	// Write the header to buffer
	u8 buffer[34]; be_t<u32>* buffer_32 = reinterpret_cast<be_t<u32>*>(buffer);

	switch (stream->src.srcSelect)
	{
	case CELL_PNGDEC_BUFFER:
		std::memcpy(buffer, stream->src.streamPtr.get_ptr(), sizeof(buffer));
		break;
	case CELL_PNGDEC_FILE:
	{
		auto file = idm::get<lv2_file_t>(stream->fd);
		file->file->Seek(0);
		file->file->Read(buffer, sizeof(buffer));
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
		cellPngDec.error("cellPngDecDecodeData: Unsupported color space (%d)", (u32)buffer[25]);
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

s32 pngDecSetParameter(PSubHandle stream, PInParam inParam, POutParam outParam, PExtInParam extInParam = vm::null, PExtOutParam extOutParam = vm::null)
{
	CellPngDecInfo& current_info = stream->info;
	CellPngDecOutParam& current_outParam = stream->outParam;

	current_outParam.outputWidthByte = (current_info.imageWidth * current_info.numComponents * current_info.bitDepth) / 8;
	current_outParam.outputWidth = current_info.imageWidth;
	current_outParam.outputHeight = current_info.imageHeight;
	current_outParam.outputColorSpace = inParam->outputColorSpace;

	switch (current_outParam.outputColorSpace)
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
		cellPngDec.error("pngDecSetParameter: Unsupported color space (%d)", current_outParam.outputColorSpace);
		return CELL_PNGDEC_ERROR_ARG;
	}

	current_outParam.outputBitDepth = inParam->outputBitDepth;
	current_outParam.outputMode = inParam->outputMode;
	current_outParam.useMemorySpace = 0; // Unimplemented

	*outParam = current_outParam;

	return CELL_OK;
}

s32 pngDecodeData(PSubHandle stream, vm::ptr<u8> data, PDataCtrlParam dataCtrlParam, PDataOutInfo dataOutInfo, PCbCtrlDisp cbCtrlDisp = vm::null, PDispParam dispParam = vm::null)
{
	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_STOP;

	const u32& fd = stream->fd;
	const u64& fileSize = stream->fileSize;
	const CellPngDecOutParam& current_outParam = stream->outParam;

	// Copy the PNG file to a buffer
	std::unique_ptr<u8[]> png(new u8[fileSize]);

	switch (stream->src.srcSelect)
	{
	case CELL_PNGDEC_BUFFER:
		std::memcpy(png.get(), stream->src.streamPtr.get_ptr(), fileSize);
		break;

	case CELL_PNGDEC_FILE:
	{
		auto file = idm::get<lv2_file_t>(stream->fd);
		file->file->Seek(0);
		file->file->Read(png.get(), fileSize);
		break;
	}
	}

	//Decode PNG file. (TODO: Is there any faster alternative? Can we do it without external libraries?)
	int width, height, actual_components;
	auto image = std::unique_ptr<unsigned char, decltype(&::free)>
		(
		stbi_load_from_memory(png.get(), (s32)fileSize, &width, &height, &actual_components, 4),
		&::free
		);
	if (!image)
	{
		cellPngDec.error("pngDecodeData: stbi_load_from_memory failed");
		return CELL_PNGDEC_ERROR_STREAM_FORMAT;
	}

	const bool flip = current_outParam.outputMode == CELL_PNGDEC_BOTTOM_TO_TOP;
	const int bytesPerLine = (u32)dataCtrlParam->outputBytesPerLine;
	uint image_size = width * height;

	switch (current_outParam.outputColorSpace)
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
		cellPngDec.error("pngDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace);
		break;

	default:
		cellPngDec.error("pngDecodeData: Unsupported color space (%d)", current_outParam.outputColorSpace);
		return CELL_PNGDEC_ERROR_ARG;
	}

	dataOutInfo->status = CELL_PNGDEC_DEC_STATUS_FINISH;

	return CELL_OK;
}

s32 cellPngDecCreate(PPMainHandle mainHandle, PThreadInParam threadInParam, PThreadOutParam threadOutParam)
{
	cellPngDec.warning("cellPngDecCreate(mainHandle=**0x%x, threadInParam=*0x%x, threadOutParam=*0x%x)", mainHandle, threadInParam, threadOutParam);

	// create decoder
	if (auto res = pngDecCreate(mainHandle, threadInParam)) return res;

	// set codec version
	threadOutParam->pngCodecVersion = PNGDEC_CODEC_VERSION;

	return CELL_OK;
}

s32 cellPngDecExtCreate(PPMainHandle mainHandle, PThreadInParam threadInParam, PThreadOutParam threadOutParam, PExtThreadInParam extThreadInParam, PExtThreadOutParam extThreadOutParam)
{
	cellPngDec.warning("cellPngDecCreate(mainHandle=**0x%x, threadInParam=*0x%x, threadOutParam=*0x%x, extThreadInParam=*0x%x, extThreadOutParam=*0x%x)",
		mainHandle, threadInParam, threadOutParam, extThreadInParam, extThreadOutParam);

	// create decoder
	if (auto res = pngDecCreate(mainHandle, threadInParam, extThreadInParam)) return res;

	// set codec version
	threadOutParam->pngCodecVersion = PNGDEC_CODEC_VERSION;

	extThreadOutParam->reserved = 0;

	return CELL_OK;
}

s32 cellPngDecDestroy(PMainHandle mainHandle)
{
	cellPngDec.warning("cellPngDecDestroy(mainHandle=*0x%x)", mainHandle);

	// destroy decoder
	return pngDecDestroy(mainHandle);
}

s32 cellPngDecOpen(PMainHandle mainHandle, PPSubHandle subHandle, PSrc src, POpenInfo openInfo)
{
	cellPngDec.warning("cellPngDecOpen(mainHandle=*0x%x, subHandle=**0x%x, src=*0x%x, openInfo=*0x%x)", mainHandle, subHandle, src, openInfo);

	// create stream handle
	return pngDecOpen(mainHandle, subHandle, src, openInfo);
}

s32 cellPngDecExtOpen(PMainHandle mainHandle, PPSubHandle subHandle, PSrc src, POpenInfo openInfo, vm::cptr<CellPngDecCbCtrlStrm> cbCtrlStrm, vm::cptr<CellPngDecOpnParam> opnParam)
{
	cellPngDec.warning("cellPngDecExtOpen(mainHandle=*0x%x, subHandle=**0x%x, src=*0x%x, openInfo=*0x%x, cbCtrlStrm=*0x%x, opnParam=*0x%x)", mainHandle, subHandle, src, openInfo, cbCtrlStrm, opnParam);

	// create stream handle
	return pngDecOpen(mainHandle, subHandle, src, openInfo, cbCtrlStrm, opnParam);
}

s32 cellPngDecClose(PMainHandle mainHandle, PSubHandle subHandle)
{
	cellPngDec.warning("cellPngDecClose(mainHandle=*0x%x, subHandle=*0x%x)", mainHandle, subHandle);

	return pngDecClose(subHandle);
}

s32 cellPngDecReadHeader(PMainHandle mainHandle, PSubHandle subHandle, PInfo info)
{
	cellPngDec.warning("cellPngDecReadHeader(mainHandle=*0x%x, subHandle=*0x%x, info=*0x%x)", mainHandle, subHandle, info);

	return pngReadHeader(subHandle, info);
}

s32 cellPngDecExtReadHeader(PMainHandle mainHandle, PSubHandle subHandle, PInfo info, PExtInfo extInfo)
{
	cellPngDec.warning("cellPngDecExtReadHeader(mainHandle=*0x%x, subHandle=*0x%x, info=*0x%x, extInfo=*0x%x)", mainHandle, subHandle, info, extInfo);

	return pngReadHeader(subHandle, info, extInfo);
}

s32 cellPngDecSetParameter(PMainHandle mainHandle, PSubHandle subHandle, PInParam inParam, POutParam outParam)
{
	cellPngDec.warning("cellPngDecSetParameter(mainHandle=*0x%x, subHandle=*0x%x, inParam=*0x%x, outParam=*0x%x)", mainHandle, subHandle, inParam, outParam);

	return pngDecSetParameter(subHandle, inParam, outParam);
}

s32 cellPngDecExtSetParameter(PMainHandle mainHandle, PSubHandle subHandle, PInParam inParam, POutParam outParam, PExtInParam extInParam, PExtOutParam extOutParam)
{
	cellPngDec.warning("cellPngDecExtSetParameter(mainHandle=*0x%x, subHandle=*0x%x, inParam=*0x%x, outParam=*0x%x, extInParam=*0x%x, extOutParam=*0x%x",
		mainHandle, subHandle, inParam, outParam, extInParam, extOutParam);

	return pngDecSetParameter(subHandle, inParam, outParam, extInParam, extOutParam);
}

s32 cellPngDecDecodeData(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<u8> data, PDataCtrlParam dataCtrlParam, PDataOutInfo dataOutInfo)
{
	cellPngDec.warning("cellPngDecDecodeData(mainHandle=*0x%x, subHandle=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x)",
		mainHandle, subHandle, data, dataCtrlParam, dataOutInfo);

	return pngDecodeData(subHandle, data, dataCtrlParam, dataOutInfo);
}

s32 cellPngDecExtDecodeData(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<u8> data, PDataCtrlParam dataCtrlParam, PDataOutInfo dataOutInfo, PCbCtrlDisp cbCtrlDisp, PDispParam dispParam)
{
	cellPngDec.warning("cellPngDecExtDecodeData(mainHandle=*0x%x, subHandle=*0x%x, data=*0x%x, dataCtrlParam=*0x%x, dataOutInfo=*0x%x, cbCtrlDisp=*0x%x, dispParam=*0x%x)",
		mainHandle, subHandle, data, dataCtrlParam, dataOutInfo, cbCtrlDisp, dispParam);

	return pngDecodeData(subHandle, data, dataCtrlParam, dataOutInfo, cbCtrlDisp, dispParam);
}

s32 cellPngDecGetUnknownChunks(PMainHandle mainHandle, PSubHandle subHandle, vm::pptr<CellPngUnknownChunk> unknownChunk, vm::ptr<u32> unknownChunkNumber)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetpCAL(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngPCAL> pcal)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetcHRM(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngCHRM> chrm)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetsCAL(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngSCAL> scal)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetpHYs(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngPHYS> phys)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetoFFs(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngOFFS> offs)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetsPLT(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngSPLT> splt)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetbKGD(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngBKGD> bkgd)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGettIME(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngTIME> time)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGethIST(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngHIST> hist)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGettRNS(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngTRNS> trns)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetsBIT(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngSBIT> sbit)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetiCCP(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngICCP> iccp)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetsRGB(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngSRGB> srgb)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetgAMA(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngGAMA> gama)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetPLTE(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<CellPngPLTE> plte)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

s32 cellPngDecGetTextChunk(PMainHandle mainHandle, PSubHandle subHandle, vm::ptr<u32> textInfoNum, vm::pptr<CellPngTextInfo> textInfo)
{
	UNIMPLEMENTED_FUNC(cellPngDec);
	return CELL_OK;
}

Module<> cellPngDec("cellPngDec", []()
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
