#pragma once

#include "Emu/Memory/vm_ptr.h"

#include "png.h"

enum : u32
{
	PNGDEC_CODEC_VERSION = 0x00420000,
};

// Return Codes
enum CellPngDecError
{
	CELL_PNGDEC_ERROR_HEADER        = 0x80611201,
	CELL_PNGDEC_ERROR_STREAM_FORMAT = 0x80611202,
	CELL_PNGDEC_ERROR_ARG           = 0x80611203,
	CELL_PNGDEC_ERROR_SEQ           = 0x80611204,
	CELL_PNGDEC_ERROR_BUSY          = 0x80611205,
	CELL_PNGDEC_ERROR_FATAL         = 0x80611206,
	CELL_PNGDEC_ERROR_OPEN_FILE     = 0x80611207,
	CELL_PNGDEC_ERROR_SPU_UNSUPPORT = 0x80611208,
	CELL_PNGDEC_ERROR_SPU_ERROR     = 0x80611209,
	CELL_PNGDEC_ERROR_CB_PARAM      = 0x8061120a,
};

// Consts
enum CellPngDecColorSpace : s32
{
	CELL_PNGDEC_GRAYSCALE       = 1,
	CELL_PNGDEC_RGB             = 2,
	CELL_PNGDEC_PALETTE         = 4,
	CELL_PNGDEC_GRAYSCALE_ALPHA = 9,
	CELL_PNGDEC_RGBA            = 10,
	CELL_PNGDEC_ARGB            = 20,
};

enum CellPngDecSpuThreadEna : s32
{
	CELL_PNGDEC_SPU_THREAD_DISABLE = 0,
	CELL_PNGDEC_SPU_THREAD_ENABLE  = 1,
};

enum CellPngDecStreamSrcSel : s32
{
	CELL_PNGDEC_FILE   = 0,
	CELL_PNGDEC_BUFFER = 1,
};

enum CellPngDecInterlaceMode : s32
{
	CELL_PNGDEC_NO_INTERLACE = 0,
	CELL_PNGDEC_ADAM7_INTERLACE = 1,
};

enum CellPngDecOutputMode : s32
{
	CELL_PNGDEC_TOP_TO_BOTTOM = 0,
	CELL_PNGDEC_BOTTOM_TO_TOP = 1,
};

enum CellPngDecPackFlag : s32
{
	CELL_PNGDEC_1BYTE_PER_NPIXEL = 0,
	CELL_PNGDEC_1BYTE_PER_1PIXEL = 1,
};

enum CellPngDecAlphaSelect : s32
{
	CELL_PNGDEC_STREAM_ALPHA = 0,
	CELL_PNGDEC_FIX_ALPHA    = 1,
};

enum CellPngDecCommand : s32
{
	CELL_PNGDEC_CONTINUE = 0,
	CELL_PNGDEC_STOP     = 1,
};

enum CellPngDecDecodeStatus : s32
{
	CELL_PNGDEC_DEC_STATUS_FINISH = 0,
	CELL_PNGDEC_DEC_STATUS_STOP   = 1,
};

enum CellPngDecBufferMode : s32
{
	CELL_PNGDEC_LINE_MODE = 1,
};

enum CellPngDecSpuMode : s32
{
	CELL_PNGDEC_RECEIVE_EVENT    = 0,
	CELL_PNGDEC_TRYRECEIVE_EVENT = 1,
};

// Callbacks for memory management
using CellPngDecCbControlMalloc = vm::ptr<void>(u32 size, vm::ptr<void> cbCtrlMallocArg);
using CellPngDecCbControlFree   = s32(vm::ptr<void> ptr, vm::ptr<void> cbCtrlFreeArg);

// Structs
struct CellPngDecThreadInParam
{
	be_t<s32> spuThreadEnable; // CellPngDecSpuThreadEna
	be_t<u32> ppuThreadPriority;
	be_t<u32> spuThreadPriority;
	vm::bptr<CellPngDecCbControlMalloc> cbCtrlMallocFunc;
	vm::bptr<void> cbCtrlMallocArg;
	vm::bptr<CellPngDecCbControlFree> cbCtrlFreeFunc;
	vm::bptr<void> cbCtrlFreeArg;
};

struct CellPngDecExtThreadInParam
{
	vm::bptr<struct CellSpurs> spurs;
	u8 priority[8];
	be_t<u32> maxContention;
};

struct CellPngDecThreadOutParam
{
	be_t<u32> pngCodecVersion;
};

struct CellPngDecExtThreadOutParam
{
	be_t<u32> reserved;
};

struct CellPngDecSrc
{
	be_t<s32> srcSelect; // CellPngDecStreamSrcSel
	vm::bcptr<char> fileName;
	be_t<s64> fileOffset;
	be_t<u32> fileSize;
	vm::bptr<void> streamPtr;
	be_t<u32> streamSize;
	be_t<s32> spuThreadEnable; // CellGifDecSpuThreadEna
};

struct CellPngDecOpnInfo
{
	be_t<u32> initSpaceAllocated;
};

struct CellPngDecInfo
{
	be_t<u32> imageWidth;
	be_t<u32> imageHeight;
	be_t<u32> numComponents;
	be_t<s32> colorSpace; // CellPngDecColorSpace
	be_t<u32> bitDepth;
	be_t<s32> interlaceMethod; // CellPngDecInterlaceMode
	be_t<u32> chunkInformation;
};

struct CellPngDecInParam
{
	vm::bptr<volatile s32> commandPtr; // CellPngDecCommand
	be_t<s32> outputMode; // CellPngDecOutputMode
	be_t<s32> outputColorSpace; // CellPngDecColorSpace
	be_t<u32> outputBitDepth;
	be_t<s32> outputPackFlag; // CellPngDecPackFlag
	be_t<s32> outputAlphaSelect; // CellPngDecAlphaSelect
	be_t<u32> outputColorAlpha;
};

struct CellPngDecOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputWidth;
	be_t<u32> outputHeight;
	be_t<u32> outputComponents;
	be_t<u32> outputBitDepth;
	be_t<s32> outputMode; // CellPngDecOutputMode
	be_t<s32> outputColorSpace; // CellPngDecOutputMode
	be_t<u32> useMemorySpace;
};

struct CellPngDecDataCtrlParam
{
	be_t<u64> outputBytesPerLine;
};

struct CellPngDecDataOutInfo
{
	be_t<u32> chunkInformation;
	be_t<u32> numText;
	be_t<u32> numUnknownChunk;
	be_t<s32> status; // CellPngDecDecodeStatus
};

// Structs for decoding partial streams
struct CellPngDecStrmInfo
{
	be_t<u32> decodedStrmSize;
};

struct CellPngDecStrmParam
{
	vm::bptr<void> strmPtr;
	be_t<u32> strmSize;
};

struct CellPngDecDispInfo
{
	be_t<u64> outputFrameWidthByte;
	be_t<u32> outputFrameHeight;
	be_t<u64> outputStartXByte;
	be_t<u32> outputStartY;
	be_t<u64> outputWidthByte;
	be_t<u32> outputHeight;
	be_t<u32> outputBitDepth;
	be_t<u32> outputComponents;
	be_t<u32> nextOutputStartY;
	be_t<u32> scanPassCount;
	vm::bptr<void> outputImage;
};

struct CellPngDecDispParam
{
	vm::bptr<void> nextOutputImage;
};

struct CellPngDecOpnParam
{
	be_t<u32> selectChunk;
};

struct CellPngDecExtInfo
{
	be_t<u64> reserved;
};

struct CellPngDecExtInParam
{
	be_t<s32> bufferMode; // CellPngDecBufferMode
	be_t<u32> outputCounts;
	be_t<s32> spuMode; // CellPngDecSpuMode
};

struct CellPngDecExtOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputHeight;
};

// Callbacks for decoding partial streams
using CellPngDecCbControlStream = s32(vm::ptr<CellPngDecStrmInfo> strmInfo, vm::ptr<CellPngDecStrmParam> strmParam, vm::ptr<void> cbCtrlStrmArg);
using CellPngDecCbControlDisp   = s32(vm::ptr<CellPngDecDispInfo> dispInfo, vm::ptr<CellPngDecDispParam> dispParam, vm::ptr<void> cbCtrlDispArg);

struct CellPngDecCbCtrlStrm
{
	vm::bptr<CellPngDecCbControlStream> cbCtrlStrmFunc;
	vm::bptr<void> cbCtrlStrmArg;
};

struct CellPngDecCbCtrlDisp
{
	vm::bptr<CellPngDecCbControlDisp> cbCtrlDispFunc;
	vm::bptr<void> cbCtrlDispArg;
};

// Custom structs
struct PngHandle
{
	vm::ptr<CellPngDecCbControlMalloc> malloc_;
	vm::ptr<void> malloc_arg;
	vm::ptr<CellPngDecCbControlFree> free_;
	vm::ptr<void> free_arg;
};

// For reading from a buffer using libpng
struct PngBuffer
{
	// The cursor location and data pointer for reading from a buffer
	usz cursor;
	usz length;
	vm::bptr<void> data;

	// The file descriptor, and whether we need to read from a file descriptor
	bool file;
	u32 fd;
};

struct PngStream
{
	// PNG decoding structures
	CellPngDecInfo info;
	CellPngDecOutParam out_param;
	CellPngDecSrc source;

	// Partial decoding
	CellPngDecCbCtrlStrm cbCtrlStream;
	CellPngDecCbCtrlDisp cbCtrlDisp;
	vm::ptr<CellPngDecDispInfo> cbDispInfo;
	vm::ptr<CellPngDecDispParam> cbDispParam;
	ppu_thread* ppuContext;

	u32 outputCounts = 0;
	u32 nextRow = 0;
	bool endOfFile = false;

	// Pixel packing value
	be_t<s32> packing;
	u32 passes;

	// PNG custom read function structure, for decoding from a buffer
	vm::ptr<PngBuffer> buffer;

	// libpng structures for reading and decoding the PNG file
	png_structp png_ptr;
	png_infop info_ptr;
};

// Converts libpng colour type to cellPngDec colour type
static s32 getPngDecColourType(u8 type)
{
	switch (type)
	{
	case PNG_COLOR_TYPE_RGB:        return CELL_PNGDEC_RGB;
	case PNG_COLOR_TYPE_RGBA:       return CELL_PNGDEC_RGBA; // We can't diffrentiate between ARGB and RGBA. Doesn't seem to be exactly important.
	case PNG_COLOR_TYPE_PALETTE:    return CELL_PNGDEC_PALETTE;
	case PNG_COLOR_TYPE_GRAY:       return CELL_PNGDEC_GRAYSCALE;
	case PNG_COLOR_TYPE_GRAY_ALPHA: return CELL_PNGDEC_GRAYSCALE_ALPHA;
	default: fmt::throw_exception("Unknown colour type: %d", type);
	}
}

static bool cellPngColorSpaceHasAlpha(u32 colorspace)
{
	switch (colorspace)
	{
	case CELL_PNGDEC_RGBA:
	case CELL_PNGDEC_ARGB:
	case CELL_PNGDEC_GRAYSCALE_ALPHA:
		return true;
	default:
		return false;
	}
}

// Custom exception for libPng errors

class LibPngCustomException : public std::runtime_error
{
public:
	LibPngCustomException(char const* const message) : runtime_error(message) {}
};
