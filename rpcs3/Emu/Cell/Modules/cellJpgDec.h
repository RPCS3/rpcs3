#pragma once

//Return Codes
enum CellJpgDecError : u32
{
	CELL_JPGDEC_ERROR_HEADER        = 0x80611101,
	CELL_JPGDEC_ERROR_STREAM_FORMAT = 0x80611102,
	CELL_JPGDEC_ERROR_ARG           = 0x80611103,
	CELL_JPGDEC_ERROR_SEQ           = 0x80611104,
	CELL_JPGDEC_ERROR_BUSY          = 0x80611105,
	CELL_JPGDEC_ERROR_FATAL         = 0x80611106,
	CELL_JPGDEC_ERROR_OPEN_FILE     = 0x80611107,
	CELL_JPGDEC_ERROR_SPU_UNSUPPORT = 0x80611108,
	CELL_JPGDEC_ERROR_CB_PARAM      = 0x80611109,
};

enum CellJpgDecColorSpace
{
	CELL_JPG_UNKNOWN                 = 0,
	CELL_JPG_GRAYSCALE               = 1,
	CELL_JPG_RGB                     = 2,
	CELL_JPG_YCbCr                   = 3,
	CELL_JPG_RGBA                    = 10,
	CELL_JPG_UPSAMPLE_ONLY           = 11,
	CELL_JPG_ARGB                    = 20,
	CELL_JPG_GRAYSCALE_TO_ALPHA_RGBA = 40,
	CELL_JPG_GRAYSCALE_TO_ALPHA_ARGB = 41,
};

enum CellJpgDecStreamSrcSel
{
	CELL_JPGDEC_FILE   = 0,
	CELL_JPGDEC_BUFFER = 1,
};

enum CellJpgDecDecodeStatus
{
	CELL_JPGDEC_DEC_STATUS_FINISH = 0, // Decoding finished
	CELL_JPGDEC_DEC_STATUS_STOP   = 1, // Decoding halted
};

enum CellJpgDecOutputMode
{
	CELL_JPGDEC_TOP_TO_BOTTOM = 0, // Top left to bottom right
	CELL_JPGDEC_BOTTOM_TO_TOP = 1, // Bottom left to top right
};

struct CellJpgDecInfo
{
	be_t<u32> imageWidth;
	be_t<u32> imageHeight;
	be_t<u32> numComponents;
	be_t<u32> colorSpace;      // CellJpgDecColorSpace
};

struct CellJpgDecSrc
{
	be_t<u32> srcSelect;       // CellJpgDecStreamSrcSel
	vm::bcptr<char> fileName;
	be_t<u64> fileOffset;      // int64_t
	be_t<u32> fileSize;
	be_t<u32> streamPtr;
	be_t<u32> streamSize;
	be_t<u32> spuThreadEnable; // CellJpgDecSpuThreadEna
};

struct CellJpgDecInParam
{
	be_t<u32> commandPtr;
	be_t<u32> downScale;
	be_t<u32> method;           // CellJpgDecMethod
	be_t<u32> outputMode;       // CellJpgDecOutputMode
	be_t<u32> outputColorSpace; // CellJpgDecColorSpace
	u8 outputColorAlpha;
	u8 reserved[3];
};

struct CellJpgDecOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputWidth;
	be_t<u32> outputHeight;
	be_t<u32> outputComponents;
	be_t<u32> outputMode;       // CellJpgDecOutputMode
	be_t<u32> outputColorSpace; // CellJpgDecColorSpace
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

// Custom structs
struct CellJpgDecSubHandle
{
	static const u32 id_base = 1;
	static const u32 id_step = 1;
	static const u32 id_count = 1023;
	SAVESTATE_INIT_POS(35);

	u32 fd;
	u64 fileSize;
	CellJpgDecInfo info;
	CellJpgDecOutParam outParam;
	CellJpgDecSrc src;
};
