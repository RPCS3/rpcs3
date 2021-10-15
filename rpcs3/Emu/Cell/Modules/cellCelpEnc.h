#pragma once

struct CellSpurs;

// libCelpEnc = 0x80614001 - 0x806140ff

// typedef void* CellCelpEncHandle;

// Return Codes
enum CellCelpEncError : u32
{
	CELL_CELPENC_ERROR_FAILED      = 0x80614001,
	CELL_CELPENC_ERROR_SEQ         = 0x80614002,
	CELL_CELPENC_ERROR_ARG         = 0x80614003,
	CELL_CELPENC_ERROR_CORE_FAILED = 0x80614081,
	CELL_CELPENC_ERROR_CORE_SEQ    = 0x80614082,
	CELL_CELPENC_ERROR_CORE_ARG    = 0x80614083,
};

// Definitions
enum CELL_CELPENC_RPE_CONFIG
{
	CELL_CELPENC_RPE_CONFIG_0,
	CELL_CELPENC_RPE_CONFIG_1,
	CELL_CELPENC_RPE_CONFIG_2,
	CELL_CELPENC_RPE_CONFIG_3,
};

enum CELL_CELPENC_SAMPEL_RATE
{
	CELL_CELPENC_FS_16kHz = 2,
};

enum CELL_CELPENC_EXCITATION_MODE
{
	CELL_CELPENC_EXCITATION_MODE_RPE = 1,
};

enum CELL_CELPENC_WORD_SZ
{
	CELL_CELPENC_WORD_SZ_INT16_LE,
	CELL_CELPENC_WORD_SZ_FLOAT
};

struct CellCelpEncAttr
{
	be_t<u32> workMemSize;
	be_t<u32> celpEncVerUpper;
	be_t<u32> celpEncVerLower;
};

struct CellCelpEncResource
{
	be_t<u32> totalMemSize;
	vm::bptr<void> startAddr;
	be_t<u32> ppuThreadPriority;
	be_t<u32> spuThreadPriority;
	be_t<u32/*usz*/> ppuThreadStackSize;
};

struct CellCelpEncParam
{
	be_t<u32> excitationMode;
	be_t<u32> sampleRate;
	be_t<u32> configuration;
	be_t<u32> wordSize;
	vm::bptr<u8> outBuff;
	be_t<u32> outSize;
};

struct CellCelpEncAuInfo
{
	vm::bptr<void> startAddr;
	be_t<u32> size;
};

struct CellCelpEncPcmInfo
{
	vm::bptr<void> startAddr;
	be_t<u32> size;
};

struct CellCelpEncResourceEx
{
	be_t<u32> totalMemSize;
	vm::bptr<void> startAddr;
	vm::bptr<CellSpurs> spurs;
	u8 priority[8];
	be_t<u32> maxContention;
};
