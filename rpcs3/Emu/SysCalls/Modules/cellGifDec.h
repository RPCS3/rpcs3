#pragma once


//Return Codes
enum
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

enum CellGifDecStreamSrcSel
{
	CELL_GIFDEC_FILE   = 0, //Input from a file
	CELL_GIFDEC_BUFFER = 1, //Input from a buffer
};

enum CellGifDecColorSpace
{
	CELL_GIFDEC_RGBA = 10,
	CELL_GIFDEC_ARGB = 20,
};

enum CellGifDecRecordType
{
	CELL_GIFDEC_RECORD_TYPE_IMAGE_DESC = 1, // Image data block
	CELL_GIFDEC_RECORD_TYPE_EXTENSION  = 2, // Extension block
	CELL_GIFDEC_RECORD_TYPE_TERMINATE  = 3, // Trailer block
};

enum CellGifDecDecodeStatus
{
	CELL_GIFDEC_DEC_STATUS_FINISH = 0, //Decoding finished
	CELL_GIFDEC_DEC_STATUS_STOP   = 1, //Decoding halted
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

struct CellGifDecSrc
{
	be_t<u32> srcSelect;
	be_t<u32> fileName;
	be_t<s64> fileOffset;
	be_t<u64> fileSize;
	be_t<u32> streamPtr;
	be_t<u32> streamSize;
	be_t<u32> spuThreadEnable;
};

struct CellGifDecInParam
{
	be_t<u32> commandPtr;
	be_t<u32> colorSpace; // CellGifDecColorSpace
	be_t<u8> outputColorAlpha1;
	be_t<u8> outputColorAlpha2;
	be_t<u8> reserved[2];
};

struct CellGifDecOutParam
{
	be_t<u64> outputWidthByte;
	be_t<u32> outputWidth;
	be_t<u32> outputHeight;
	be_t<u32> outputComponents;
	be_t<u32> outputBitDepth;
	be_t<u32> outputColorSpace;	// CellGifDecColorSpace
	be_t<u32> useMemorySpace;
};

struct CellGifDecExtension
{
	be_t<u8> label;
	be_t<u32> data;
};

struct CellGifDecDataOutInfo
{
	be_t<u32> recordType;
	CellGifDecExtension outExtension;
	be_t<u32> status;
};

struct CellGifDecOpnInfo
{
	be_t<u32> initSpaceAllocated;
};

struct CellGifDecDataCtrlParam
{
	be_t<u64> outputBytesPerLine;
};

struct CellGifDecSubHandle //Custom struct
{
	u32 fd;
	u64 fileSize;
	CellGifDecInfo info;
	CellGifDecOutParam outParam;
};
