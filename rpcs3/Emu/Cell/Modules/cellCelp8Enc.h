#pragma once

struct CellSpurs;

// libCelp8Enc = 0x806140a1 - 0x806140bf

// typedef void* CellCelp8EncHandle;

// Return Codes
enum CellCelp8EncError : u32
{
	CELL_CELP8ENC_ERROR_FAILED      = 0x806140a1,
	CELL_CELP8ENC_ERROR_SEQ         = 0x806140a2,
	CELL_CELP8ENC_ERROR_ARG         = 0x806140a3,
	CELL_CELP8ENC_ERROR_CORE_FAILED = 0x806140b1,
	CELL_CELP8ENC_ERROR_CORE_SEQ    = 0x806140b2,
	CELL_CELP8ENC_ERROR_CORE_ARG    = 0x806140b3,
};

// Definitions
enum CELL_CELP8ENC_MPE_CONFIG
{
	CELL_CELP8ENC_MPE_CONFIG_0 = 0,
	CELL_CELP8ENC_MPE_CONFIG_2 = 2,
	CELL_CELP8ENC_MPE_CONFIG_6 = 6,
	CELL_CELP8ENC_MPE_CONFIG_9 = 9,
	CELL_CELP8ENC_MPE_CONFIG_12 = 12,
	CELL_CELP8ENC_MPE_CONFIG_15 = 15,
	CELL_CELP8ENC_MPE_CONFIG_18 = 18,
	CELL_CELP8ENC_MPE_CONFIG_21 = 21,
	CELL_CELP8ENC_MPE_CONFIG_24 = 24,
	CELL_CELP8ENC_MPE_CONFIG_26 = 26,
};

enum CELL_CELP8ENC_SAMPEL_RATE
{
	CELL_CELP8ENC_FS_8kHz = 1,
};

enum CELL_CELP8ENC_EXCITATION_MODE
{
	CELL_CELP8ENC_EXCITATION_MODE_MPE = 0,
};

enum CELL_CELP8ENC_WORD_SZ
{
	CELL_CELP8ENC_WORD_SZ_FLOAT,
};

struct CellCelp8EncAttr
{
	be_t<u32> workMemSize;
	be_t<u32> celpEncVerUpper;
	be_t<u32> celpEncVerLower;
};

struct CellCelp8EncResource
{
	be_t<u32> totalMemSize;
	vm::bptr<void> startAddr;
	be_t<u32> ppuThreadPriority;
	be_t<u32> spuThreadPriority;
	be_t<u32/*usz*/> ppuThreadStackSize;
};

struct CellCelp8EncParam
{
	be_t<u32> excitationMode;
	be_t<u32> sampleRate;
	be_t<u32> configuration;
	be_t<u32> wordSize;
	vm::bptr<u8> outBuff;
	be_t<u32> outSize;
};

struct CellCelp8EncAuInfo
{
	vm::bptr<void> startAddr;
	be_t<u32> size;
};

struct CellCelp8EncPcmInfo
{
	vm::bptr<void> startAddr;
	be_t<u32> size;
};

struct CellCelp8EncResourceEx
{
	be_t<u32> totalMemSize;
	vm::bptr<void> startAddr;
	vm::bptr<CellSpurs> spurs;
	u8 priority[8];
	be_t<u32> maxContention;
};
