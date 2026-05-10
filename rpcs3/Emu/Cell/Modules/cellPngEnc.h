#pragma once

// Error Codes
enum CellPngEncError
{
	CELL_PNGENC_ERROR_ARG              = 0x80611291,
	CELL_PNGENC_ERROR_SEQ              = 0x80611292,
	CELL_PNGENC_ERROR_BUSY             = 0x80611293,
	CELL_PNGENC_ERROR_EMPTY            = 0x80611294,
	CELL_PNGENC_ERROR_RESET            = 0x80611295,
	CELL_PNGENC_ERROR_FATAL            = 0x80611296,
	CELL_PNGENC_ERROR_STREAM_ABORT     = 0x806112A1,
	CELL_PNGENC_ERROR_STREAM_SKIP      = 0x806112A2,
	CELL_PNGENC_ERROR_STREAM_OVERFLOW  = 0x806112A3,
	CELL_PNGENC_ERROR_STREAM_FILE_OPEN = 0x806112A4,
};

enum CellPngEncColorSpace
{
    CELL_PNGENC_COLOR_SPACE_GRAYSCALE       = 1,
    CELL_PNGENC_COLOR_SPACE_RGB             = 2,
    CELL_PNGENC_COLOR_SPACE_PALETTE         = 4,
    CELL_PNGENC_COLOR_SPACE_GRAYSCALE_ALPHA = 9,
    CELL_PNGENC_COLOR_SPACE_RGBA            = 10,
    CELL_PNGENC_COLOR_SPACE_ARGB            = 20
};

enum CellPngEncCompressionLevel
{
    CELL_PNGENC_COMPR_LEVEL_0,
    CELL_PNGENC_COMPR_LEVEL_1,
    CELL_PNGENC_COMPR_LEVEL_2,
    CELL_PNGENC_COMPR_LEVEL_3,
    CELL_PNGENC_COMPR_LEVEL_4,
    CELL_PNGENC_COMPR_LEVEL_5,
    CELL_PNGENC_COMPR_LEVEL_6,
    CELL_PNGENC_COMPR_LEVEL_7,
    CELL_PNGENC_COMPR_LEVEL_8,
    CELL_PNGENC_COMPR_LEVEL_9
};

enum CellPngEncFilterType
{
    CELL_PNGENC_FILTER_TYPE_NONE  = 0x08,
    CELL_PNGENC_FILTER_TYPE_SUB   = 0x10,
    CELL_PNGENC_FILTER_TYPE_UP    = 0x20,
    CELL_PNGENC_FILTER_TYPE_AVG   = 0x40,
    CELL_PNGENC_FILTER_TYPE_PAETH = 0x80,
    CELL_PNGENC_FILTER_TYPE_ALL   = 0xF8
};

enum CellPngEncChunkType
{
    CELL_PNGENC_CHUNK_TYPE_PLTE,
    CELL_PNGENC_CHUNK_TYPE_TRNS,
    CELL_PNGENC_CHUNK_TYPE_CHRM,
    CELL_PNGENC_CHUNK_TYPE_GAMA,
    CELL_PNGENC_CHUNK_TYPE_ICCP,
    CELL_PNGENC_CHUNK_TYPE_SBIT,
    CELL_PNGENC_CHUNK_TYPE_SRGB,
    CELL_PNGENC_CHUNK_TYPE_TEXT,
    CELL_PNGENC_CHUNK_TYPE_BKGD,
    CELL_PNGENC_CHUNK_TYPE_HIST,
    CELL_PNGENC_CHUNK_TYPE_PHYS,
    CELL_PNGENC_CHUNK_TYPE_SPLT,
    CELL_PNGENC_CHUNK_TYPE_TIME,
    CELL_PNGENC_CHUNK_TYPE_OFFS,
    CELL_PNGENC_CHUNK_TYPE_PCAL,
    CELL_PNGENC_CHUNK_TYPE_SCAL,
    CELL_PNGENC_CHUNK_TYPE_UNKNOWN
};

enum CellPngEncLocation
{
    CELL_PNGENC_LOCATION_FILE,
    CELL_PNGENC_LOCATION_BUFFER
};

//typedef void *CellPngEncHandle;

struct CellPngEncExParam // Size 8
{
	be_t<s32> index;
	vm::bptr<s32> value;
};

struct CellPngEncConfig // Size 28
{
	be_t<u32> maxWidth;
	be_t<u32> maxHeight;
	be_t<u32> maxBitDepth;
	b8 enableSpu;
	u8 padding[3];
	be_t<u32> addMemSize;
	vm::bptr<CellPngEncExParam> exParamList;
	be_t<u32> exParamNum;
};

struct CellPngEncAttr // Size 16
{
	be_t<u32> memSize; // usz
	u8 cmdQueueDepth;
	u8 padding[3];
	be_t<u32> versionUpper;
	be_t<u32> versionLower;
};

struct CellPngEncResource // Size 16
{
	vm::bptr<void> memAddr;
	be_t<u32> memSize; // usz
	be_t<s32> ppuThreadPriority;
	be_t<s32> spuThreadPriority;
};

struct CellPngEncResourceEx // Size 24
{
	vm::bptr<void> memAddr;
	be_t<u32> memSize; // usz
	be_t<s32> ppuThreadPriority;
	vm::bptr<void> spurs; // CellSpurs
	u8 priority[8];
};

struct CellPngEncPicture // Size 40
{
	be_t<u32> width;
	be_t<u32> height;
	be_t<u32> pitchWidth;
	be_t<u32> colorSpace; // CellPngEncColorSpace
	be_t<u32> bitDepth;
	b8 packedPixel;
	u8 padding[3]; // TODO: is this correct?
	vm::bptr<void> pictureAddr;
	be_t<u64> userData;
};

struct CellPngEncAncillaryChunk // Size 8
{
	be_t<u32> chunkType; // CellPngEncChunkType
	vm::bptr<void> chunkData;
};

struct CellPngEncEncodeParam // Size 24
{
	b8 enableSpu;
	u8 padding[3];
	be_t<u32> encodeColorSpace; // CellPngEncColorSpace
	be_t<u32> compressionLevel; // CellPngEncCompressionLevel
	be_t<u32> filterType;
	vm::bptr<CellPngEncAncillaryChunk> ancillaryChunkList;
	be_t<u32> ancillaryChunkNum;
};

struct CellPngEncOutputParam // Size 16
{
	be_t<u32> location; // CellPngEncLocation
	vm::bcptr<char> streamFileName;
	vm::bptr<void> streamAddr;
	be_t<u32> limitSize; // usz
};

struct CellPngEncStreamInfo // Size 40
{
	be_t<s32> state;
	be_t<u32> location; // CellPngEncLocation
	vm::bcptr<char> streamFileName;
	vm::bptr<void> streamAddr;
	be_t<u32> limitSize; // usz
	be_t<u32> streamSize; // usz
	be_t<u32> processedLine;
	be_t<u64> userData;
	// TODO: where are the missing 4 bytes?
};
