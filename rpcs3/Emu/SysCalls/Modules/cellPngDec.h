#pragma once

//Return Codes
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

enum CellPngDecColorSpace
{
	CELL_PNGDEC_GRAYSCALE       = 1,
	CELL_PNGDEC_RGB             = 2,
	CELL_PNGDEC_PALETTE         = 4,
	CELL_PNGDEC_GRAYSCALE_ALPHA = 9,
	CELL_PNGDEC_RGBA            = 10,
	CELL_PNGDEC_ARGB            = 20,
};

enum CellPngDecDecodeStatus
{
	CELL_PNGDEC_DEC_STATUS_FINISH = 0, //Decoding finished
	CELL_PNGDEC_DEC_STATUS_STOP   = 1, //Decoding halted
};

enum CellPngDecStreamSrcSel
{
	CELL_PNGDEC_FILE   = 0,
	CELL_PNGDEC_BUFFER = 1,
};

enum CellPngDecInterlaceMode
{
	CELL_PNGDEC_NO_INTERLACE = 0,
	CELL_PNGDEC_ADAM7_INTERLACE = 1,
};

enum CellPngDecOutputMode
{
	CELL_PNGDEC_TOP_TO_BOTTOM = 0,
	CELL_PNGDEC_BOTTOM_TO_TOP = 1,
};


// Structs
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
	be_t<u32> colorSpace;         // CellPngDecColorSpace
	be_t<u32> bitDepth;
	be_t<u32> interlaceMethod;    // CellPngDecInterlaceMode
	be_t<u32> chunkInformation;
};

struct CellPngDecSrc
{
	be_t<u32> srcSelect;          // CellPngDecStreamSrcSel
	be_t<u32> fileName_addr;      // const char*
	be_t<s64> fileOffset;
	be_t<u32> fileSize;
	be_t<u32> streamPtr;
	be_t<u32> streamSize;
	be_t<u32> spuThreadEnable;    // CellPngDecSpuThreadEna
};

struct CellPngDecInParam
{
	be_t<u32> commandPtr;
	be_t<u32> outputMode;         // CellPngDecOutputMode
	be_t<u32> outputColorSpace;   // CellPngDecColorSpace
	be_t<u32> outputBitDepth;
	be_t<u32> outputPackFlag;     // CellPngDecPackFlag
	be_t<u32> outputAlphaSelect;  // CellPngDecAlphaSelect
	be_t<u32> outputColorAlpha;
};

struct CellPngDecOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputWidth;
	be_t<u32> outputHeight;
	be_t<u32> outputComponents;
	be_t<u32> outputBitDepth;
	be_t<u32> outputMode;         // CellPngDecOutputMode
	be_t<u32> outputColorSpace;   // CellPngDecColorSpace
	be_t<u32> useMemorySpace;
};

struct CellPngDecStrmInfo
{
	be_t<u32> decodedStrmSize;
};

struct CellPngDecStrmParam
{
	be_t<u32> strmPtr;
	be_t<u32> strmSize;
};

struct CellPngDecCbCtrlStrm
{
	vm::bptr<void(*)(vm::ptr<CellPngDecStrmInfo> strmInfo, vm::ptr<CellPngDecStrmParam> strmParam, u32 cbCtrlStrmArg)> cbCtrlStrmFunc;
	be_t<u32> cbCtrlStrmArg;
};

struct CellPngDecCbCtrlDisp
{
	be_t<u32> cbCtrlDispFunc_addr;
	be_t<u32> cbCtrlDispArg;
};

struct CellPngDecDispParam
{
	be_t<u32> nextOutputImage_addr;
};

struct CellPngDecExtInfo
{
	be_t<u64> reserved;
};

struct CellPngDecExtInParam
{
	be_t<u32> bufferMode; // CellPngDecBufferMode
	be_t<u32> outputCounts;
	be_t<u32> spuMode; // CellPngDecSpuMode
};

struct CellPngDecExtOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputHeight;
};

struct CellPngDecOpnParam
{
	be_t<u32> selectChunk;
};


// Custom structs
struct CellPngDecSubHandle
{
	be_t<u32> fd;
	be_t<u64> fileSize;
	CellPngDecInfo info;
	CellPngDecOutParam outParam;
	CellPngDecSrc src;
};

struct CellPngDecMainHandle
{
	be_t<u32> mainHandle;
	be_t<u32> threadInParam;
	be_t<u32> threadOutParam;
};
