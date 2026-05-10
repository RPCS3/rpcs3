#pragma once

#include "Emu/Memory/vm_ptr.h"

// Return Codes
enum CellGifDecError : u32
{
	CELL_GIFDEC_ERROR_OPEN_FILE     = 0x80611300,
	CELL_GIFDEC_ERROR_STREAM_FORMAT = 0x80611301,
	CELL_GIFDEC_ERROR_SEQ           = 0x80611302,
	CELL_GIFDEC_ERROR_ARG           = 0x80611303,
	CELL_GIFDEC_ERROR_FATAL         = 0x80611304,
	CELL_GIFDEC_ERROR_SPU_UNSUPPORT = 0x80611305,
	CELL_GIFDEC_ERROR_SPU_ERROR     = 0x80611306,
	CELL_GIFDEC_ERROR_CB_PARAM      = 0x80611307,
};

enum CellGifDecStreamSrcSel : s32
{
	CELL_GIFDEC_FILE   = 0, // Input from a file
	CELL_GIFDEC_BUFFER = 1, // Input from a buffer
};

enum CellGifDecSpuThreadEna : s32
{
	CELL_GIFDEC_SPU_THREAD_DISABLE = 0, // Do not use SPU threads
	CELL_GIFDEC_SPU_THREAD_ENABLE  = 1, // Use SPU threads
};

enum CellGifDecRecordType : s32
{
	CELL_GIFDEC_RECORD_TYPE_IMAGE_DESC = 1, // Image data block
	CELL_GIFDEC_RECORD_TYPE_EXTENSION  = 2, // Extension block
	CELL_GIFDEC_RECORD_TYPE_TERMINATE  = 3, // Trailer block
};

enum CellGifDecColorSpace : s32
{
	CELL_GIFDEC_RGBA = 10, // RGBA
	CELL_GIFDEC_ARGB = 20, // ARGB
};

enum CellGifDecCommand : s32
{
	CELL_GIFDEC_CONTINUE = 0, // Continue decoding
	CELL_GIFDEC_STOP     = 1, // Force decoding to stop
};

enum CellGifDecDecodeStatus : s32
{
	CELL_GIFDEC_DEC_STATUS_FINISH = 0, // Decoding finished
	CELL_GIFDEC_DEC_STATUS_STOP   = 1, // Decoding was stopped
};

enum CellGifDecBufferMode : s32
{
	CELL_GIFDEC_LINE_MODE = 1
};

enum CellGifDecSpuMode : s32
{
    CELL_GIFDEC_RECEIVE_EVENT    = 0,
    CELL_GIFDEC_TRYRECEIVE_EVENT = 1
};

enum CellGifDecInterlaceMode : s32
{
    CELL_GIFDEC_NO_INTERLACE = 0,
    CELL_GIFDEC_INTERLACE    = 1
};

// Callbacks for memory management
using CellGifDecCbControlMalloc = vm::ptr<void>(u32 size, vm::ptr<void> cbCtrlMallocArg);
using CellGifDecCbControlFree = s32(vm::ptr<void> ptr, vm::ptr<void> cbCtrlFreeArg);

// Structs
struct CellGifDecThreadInParam
{
	be_t<s32> spuThreadEnable; // CellGifDecSpuThreadEna
	be_t<u32> ppuThreadPriority;
	be_t<u32> spuThreadPriority;
	vm::bptr<CellGifDecCbControlMalloc> cbCtrlMallocFunc;
	vm::bptr<void> cbCtrlMallocArg;
	vm::bptr<CellGifDecCbControlFree> cbCtrlFreeFunc;
	vm::bptr<void> cbCtrlFreeArg;
};

struct CellGifDecThreadOutParam
{
	be_t<u32> gifCodecVersion;
};

struct CellGifDecExtThreadInParam
{
	vm::bptr<struct CellSpurs> spurs;
	u8 priority[8];
	be_t<u32> maxContention;
};

struct CellGifDecExtThreadOutParam
{
	be_t<u64> reserved;
};

struct CellGifDecSrc
{
	be_t<s32> srcSelect; // CellGifDecStreamSrcSel
	vm::bcptr<char> fileName;
	be_t<s64> fileOffset;
	be_t<u32> fileSize;
	vm::bptr<void> streamPtr;
	be_t<u32> streamSize;
	be_t<s32> spuThreadEnable; // CellGifDecSpuThreadEna
};

struct CellGifDecOpnInfo
{
	be_t<u32> initSpaceAllocated;
};

struct CellGifDecInfo
{
	be_t<u32> SWidth;
	be_t<u32> SHeight;
	be_t<u32> SGlobalColorTableFlag;
	be_t<u32> SColorResolution;
	be_t<u32> SSortFlag;
	be_t<u32> SSizeOfGlobalColorTable;
	be_t<u32> SBackGroundColor;
	be_t<u32> SPixelAspectRatio;
};

struct CellGifDecInParam
{
	vm::bptr<volatile s32> commandPtr; // CellGifDecCommand
	be_t<s32> colorSpace; // CellGifDecColorSpace
	u8 outputColorAlpha1;
	u8 outputColorAlpha2;
	u8 reserved[2];
};

struct CellGifDecOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputWidth;
	be_t<u32> outputHeight;
	be_t<u32> outputComponents;
	be_t<u32> outputBitDepth;
	be_t<s32> outputColorSpace; // CellGifDecColorSpace
	be_t<u32> useMemorySpace;
};

struct CellGifDecExtension
{
	u8 label;
	vm::bptr<u8> data;
};

struct CellGifDecDataOutInfo
{
	be_t<s32> recordType; // CellGifDecRecordType
	CellGifDecExtension outExtension;
	be_t<s32> status; // CellGifDecDecodeStatus
};

struct CellGifDecDataCtrlParam
{
	be_t<u64> outputBytesPerLine;
};

struct CellGifDecExtInfo
{
	vm::bptr<u64> reserved;
};

struct CellGifDecStrmInfo
{
	be_t<u32> decodedStrmSize;
};

struct CellGifDecStrmParam
{
	vm::bptr<void> strmPtr;
	be_t<u32> strmSize;
};

using CellGifDecCbControlStream = s32(vm::ptr<CellGifDecStrmInfo> strmInfo, vm::ptr<CellGifDecStrmParam> strmParam, vm::ptr<void> cbCtrlStrmArg);

struct CellGifDecCbCtrlStrm
{
	vm::bptr<CellGifDecCbControlStream> cbCtrlStrmFunc;
	vm::bptr<void> cbCtrlStrmArg;
};

struct CellGifDecExtInParam
{
	be_t<s32> bufferMode; // CellGifDecBufferMode
	be_t<u32> outputCounts;
	be_t<s32> spuMode; // CellGifDecSpuMode
};

struct CellGifDecExtOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputHeight;
};

struct CellGifDecDispParam
{
	vm::bptr<void> nextOutputImage;
};

struct CellGifDecDispInfo
{
    be_t<u64> outputFrameWidthByte;
    be_t<u32> outputFrameHeight;
    be_t<u64> outputStartXByte;
    be_t<u32> outputStartY;
    be_t<u64> outputWidthByte;
    be_t<u32> outputHeight;
    be_t<u32> outputBitDepth;
    be_t<u32> outputComponents;
    be_t<u32> scanPassCount;
    vm::bptr<void> outputImage;
    be_t<s32> interlaceFlag; // CellGifDecInterlaceMode
};

using CellGifDecCbControlDisp = s32(vm::ptr<CellGifDecDispInfo> dispInfo, vm::ptr<CellGifDecDispParam> dispParam, vm::ptr<void> cbCtrlDispArg);

struct CellGifDecCbCtrlDisp
{
	vm::bptr<CellGifDecCbControlDisp> cbCtrlDispFunc;
	vm::bptr<void> cbCtrlDispArg;
};

// Custom structs
struct GifDecoder
{
};

struct GifStream
{
	u32 fd{};
	u64 fileSize{};
	CellGifDecInfo info{};
	CellGifDecOutParam outParam{};
	CellGifDecSrc src{};
};
