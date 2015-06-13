#pragma once

// Return Codes
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
	CELL_GIFDEC_FILE   = 0, // Input from a file
	CELL_GIFDEC_BUFFER = 1, // Input from a buffer
};

enum CellGifDecSpuThreadEna
{
	CELL_GIFDEC_SPU_THREAD_DISABLE = 0, // Do not use SPU threads
	CELL_GIFDEC_SPU_THREAD_ENABLE  = 1, // Use SPU threads
};

enum CellGifDecRecordType
{
	CELL_GIFDEC_RECORD_TYPE_IMAGE_DESC = 1, // Image data block
	CELL_GIFDEC_RECORD_TYPE_EXTENSION  = 2, // Extension block
	CELL_GIFDEC_RECORD_TYPE_TERMINATE  = 3, // Trailer block
};

enum CellGifDecColorSpace
{
	CELL_GIFDEC_RGBA = 10, // RGBA
	CELL_GIFDEC_ARGB = 20, // ARGB
};

enum CellGifDecCommand
{
	CELL_GIFDEC_CONTINUE = 0, // Continue decoding
	CELL_GIFDEC_STOP     = 1, // Force decoding to stop
};

enum CellGifDecDecodeStatus
{
	CELL_GIFDEC_DEC_STATUS_FINISH = 0, // Decoding finished
	CELL_GIFDEC_DEC_STATUS_STOP   = 1, // Decoding was stopped
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
	vm::bptr<const char> fileName;
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
	be_t<u32> outputColorSpace;	// CellGifDecColorSpace
	be_t<u32> useMemorySpace;
};

struct CellGifDecExtension
{
	u8 label;
	vm::ptr<u8> data;
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

//Custom structs
struct CellGifDecSubHandle
{
	u32 fd;
	u64 fileSize;
	CellGifDecInfo info;
	CellGifDecOutParam outParam;
	CellGifDecSrc src;
};
