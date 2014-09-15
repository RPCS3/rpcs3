#pragma once
#include "cellPng.h"

enum : u32
{
	PNGDEC_CODEC_VERSION = 0x00420000,
};

struct PngDecoder;
struct PngStream;

typedef vm::ptr<PngDecoder> CellPngDecMainHandle;
typedef vm::ptr<PngStream> CellPngDecSubHandle;

// Return Codes
enum
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
enum CellPngDecColorSpace : u32
{
	CELL_PNGDEC_GRAYSCALE       = 1,
	CELL_PNGDEC_RGB             = 2,
	CELL_PNGDEC_PALETTE         = 4,
	CELL_PNGDEC_GRAYSCALE_ALPHA = 9,
	CELL_PNGDEC_RGBA            = 10,
	CELL_PNGDEC_ARGB            = 20,
};

enum CellPngDecSpuThreadEna : u32
{
	CELL_PNGDEC_SPU_THREAD_DISABLE = 0,
	CELL_PNGDEC_SPU_THREAD_ENABLE  = 1,
};

enum CellPngDecStreamSrcSel : u32
{
	CELL_PNGDEC_FILE   = 0,
	CELL_PNGDEC_BUFFER = 1,
};

enum CellPngDecInterlaceMode : u32
{
	CELL_PNGDEC_NO_INTERLACE = 0,
	CELL_PNGDEC_ADAM7_INTERLACE = 1,
};

enum CellPngDecOutputMode : u32
{
	CELL_PNGDEC_TOP_TO_BOTTOM = 0,
	CELL_PNGDEC_BOTTOM_TO_TOP = 1,
};

enum CellPngDecPackFlag : u32
{
	CELL_PNGDEC_1BYTE_PER_NPIXEL = 0,
	CELL_PNGDEC_1BYTE_PER_1PIXEL = 1,
};

enum CellPngDecAlphaSelect : u32
{
	CELL_PNGDEC_STREAM_ALPHA = 0,
	CELL_PNGDEC_FIX_ALPHA    = 1,
};

enum CellPngDecCommand : u32
{
	CELL_PNGDEC_CONTINUE = 0,
	CELL_PNGDEC_STOP     = 1,
};

enum CellPngDecDecodeStatus : u32
{
	CELL_PNGDEC_DEC_STATUS_FINISH = 0,
	CELL_PNGDEC_DEC_STATUS_STOP   = 1,
};

// Callbacks
typedef vm::ptr<void>(*CellPngDecCbControlMalloc)(u32 size, vm::ptr<void> cbCtrlMallocArg);
typedef s32(*CellPngDecCbControlFree)(vm::ptr<void> ptr, vm::ptr<void> cbCtrlFreeArg);

// Structs
struct CellPngDecThreadInParam
{
	be_t<CellPngDecSpuThreadEna> spuThreadEnable;
	be_t<u32> ppuThreadPriority;
	be_t<u32> spuThreadPriority;
	vm::bptr<CellPngDecCbControlMalloc> cbCtrlMallocFunc;
	vm::bptr<void> cbCtrlMallocArg;
	vm::bptr<CellPngDecCbControlFree> cbCtrlFreeFunc;
	vm::bptr<void> cbCtrlFreeArg;
};

struct CellPngDecExtThreadInParam
{
	be_t<u32> spurs_addr; // it could be vm::bptr<CellSpurs>, but nobody will use SPURS in HLE implementation
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
	be_t<CellPngDecStreamSrcSel> srcSelect;
	vm::bptr<const char> fileName;
	be_t<s64> fileOffset;
	be_t<u32> fileSize;
	vm::bptr<void> streamPtr;
	be_t<u32> streamSize;
	be_t<CellPngDecSpuThreadEna> spuThreadEnable;
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
	be_t<CellPngDecColorSpace> colorSpace;
	be_t<u32> bitDepth;
	be_t<CellPngDecInterlaceMode> interlaceMethod;
	be_t<u32> chunkInformation;
};

struct CellPngDecInParam
{
	vm::bptr<volatile CellPngDecCommand> commandPtr;
	be_t<CellPngDecOutputMode> outputMode;
	be_t<CellPngDecColorSpace> outputColorSpace;
	be_t<u32> outputBitDepth;
	be_t<CellPngDecPackFlag> outputPackFlag;
	be_t<CellPngDecAlphaSelect> outputAlphaSelect;
	be_t<u32> outputColorAlpha;
};

struct CellPngDecOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputWidth;
	be_t<u32> outputHeight;
	be_t<u32> outputComponents;
	be_t<u32> outputBitDepth;
	be_t<CellPngDecOutputMode> outputMode;
	be_t<CellPngDecColorSpace> outputColorSpace;
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
	be_t<CellPngDecDecodeStatus> status;
};

// Functions
s32 cellPngDecCreate(vm::ptr<u32> mainHandle, vm::ptr<const CellPngDecThreadInParam> threadInParam, vm::ptr<CellPngDecThreadOutParam> threadOutParam);

s32 cellPngDecExtCreate(
	vm::ptr<u32> mainHandle,
	vm::ptr<const CellPngDecThreadInParam> threadInParam,
	vm::ptr<CellPngDecThreadOutParam> threadOutParam,
	vm::ptr<const CellPngDecExtThreadInParam> extThreadInParam,
	vm::ptr<CellPngDecExtThreadOutParam> extThreadOutParam);

s32 cellPngDecOpen(
	CellPngDecMainHandle mainHandle,
	vm::ptr<u32> subHandle,
	vm::ptr<const CellPngDecSrc> src,
	vm::ptr<CellPngDecOpnInfo> openInfo);

s32 cellPngDecReadHeader(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngDecInfo> info);

s32 cellPngDecSetParameter(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<const CellPngDecInParam> inParam,
	vm::ptr<CellPngDecOutParam> outParam);

s32 cellPngDecDecodeData(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<u8> data,
	vm::ptr<const CellPngDecDataCtrlParam> dataCtrlParam,
	vm::ptr<CellPngDecDataOutInfo> dataOutInfo);

s32 cellPngDecClose(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle);

s32 cellPngDecDestroy(CellPngDecMainHandle mainHandle);

s32 cellPngDecGetTextChunk(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<u32> textInfoNum,
	vm::ptr<vm::bptr<CellPngTextInfo>> textInfo);

s32 cellPngDecGetPLTE(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngPLTE> plte);

s32 cellPngDecGetgAMA(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngGAMA> gama);

s32 cellPngDecGetsRGB(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSRGB> srgb);

s32 cellPngDecGetiCCP(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngICCP> iccp);

s32 cellPngDecGetsBIT(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSBIT> sbit);

s32 cellPngDecGettRNS(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngTRNS> trns);

s32 cellPngDecGethIST(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngHIST> hist);

s32 cellPngDecGettIME(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngTIME> time);

s32 cellPngDecGetbKGD(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngBKGD> bkgd);

s32 cellPngDecGetsPLT(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSPLT> splt);

s32 cellPngDecGetoFFs(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngOFFS> offs);

s32 cellPngDecGetpHYs(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngPHYS> phys);

s32 cellPngDecGetsCAL(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngSCAL> scal);

s32 cellPngDecGetcHRM(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngCHRM> chrm);

s32 cellPngDecGetpCAL(CellPngDecMainHandle mainHandle, CellPngDecSubHandle subHandle, vm::ptr<CellPngPCAL> pcal);

s32 cellPngDecGetUnknownChunks(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<vm::bptr<CellPngUnknownChunk>> unknownChunk,
	vm::ptr<u32> unknownChunkNumber);


enum CellPngDecBufferMode
{
	CELL_PNGDEC_LINE_MODE = 1,
};

enum CellPngDecSpuMode
{
	CELL_PNGDEC_RECEIVE_EVENT    = 0,
	CELL_PNGDEC_TRYRECEIVE_EVENT = 1,
};

// Structs
struct CellPngDecStrmInfo
{
	be_t<u32> decodedStrmSize;
};

struct CellPngDecStrmParam
{
	vm::bptr<void> strmPtr;
	be_t<u32> strmSize;
};

typedef s32(*CellPngDecCbControlStream)(
	vm::ptr<CellPngDecStrmInfo> strmInfo,
	vm::ptr<CellPngDecStrmParam> strmParam,
	vm::ptr<void> cbCtrlStrmArg
	);

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

// Callback
typedef s32(*CellPngDecCbControlDisp)(vm::ptr<CellPngDecDispInfo> dispInfo, vm::ptr<CellPngDecDispParam> dispParam, vm::ptr<void> cbCtrlDispArg);

// Structs
struct CellPngDecOpnParam
{
	be_t<u32> selectChunk;
};

struct CellPngDecCbCtrlStrm
{
	vm::bptr<CellPngDecCbControlStream> cbCtrlStrmFunc;
	be_t<vm::ptr<void>> cbCtrlStrmArg;
};

struct CellPngDecExtInfo
{
	be_t<u64> reserved;
};

struct CellPngDecExtInParam
{
	be_t<CellPngDecBufferMode> bufferMode;
	be_t<u32> outputCounts;
	be_t<CellPngDecSpuMode> spuMode;
};

struct CellPngDecExtOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputHeight;
};

struct CellPngDecCbCtrlDisp
{
	vm::bptr<CellPngDecCbControlDisp> cbCtrlDispFunc;
	be_t<vm::ptr<void>> cbCtrlDispArg;
};

// Functions
s32 cellPngDecExtOpen(
	CellPngDecMainHandle mainHandle,
	vm::ptr<u32> subHandle,
	vm::ptr<const CellPngDecSrc> src,
	vm::ptr<CellPngDecOpnInfo> openInfo,
	vm::ptr<const CellPngDecCbCtrlStrm> cbCtrlStrm,
	vm::ptr<const CellPngDecOpnParam> opnParam);

s32 cellPngDecExtReadHeader(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<CellPngDecInfo> info,
	vm::ptr<CellPngDecExtInfo> extInfo);

s32 cellPngDecExtSetParameter(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<const CellPngDecInParam> inParam,
	vm::ptr<CellPngDecOutParam> outParam,
	vm::ptr<const CellPngDecExtInParam> extInParam,
	vm::ptr<CellPngDecExtOutParam> extOutParam);

s32 cellPngDecExtDecodeData(
	CellPngDecMainHandle mainHandle,
	CellPngDecSubHandle subHandle,
	vm::ptr<u8> data,
	vm::ptr<const CellPngDecDataCtrlParam> dataCtrlParam,
	vm::ptr<CellPngDecDataOutInfo> dataOutInfo,
	vm::ptr<const CellPngDecCbCtrlDisp> cbCtrlDisp,
	vm::ptr<CellPngDecDispParam> dispParam);

// Custom structs
struct PngDecoder
{
	vm::ptr<CellPngDecCbControlMalloc> malloc;
	vm::ptr<void> malloc_arg;
	vm::ptr<CellPngDecCbControlFree> free;
	vm::ptr<void> free_arg;
};

struct PngStream
{
	CellPngDecMainHandle dec;

	// old data:
	u32 fd;
	u64 fileSize;
	CellPngDecInfo info;
	CellPngDecOutParam outParam;
	CellPngDecSrc src;

	CellPngDecStrmInfo streamInfo;
	CellPngDecStrmParam streamParam;
};