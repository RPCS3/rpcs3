#pragma once

#include "Emu/Memory/vm_ptr.h"

// Return Codes
enum
{
	CELL_BGDL_UTIL_ERROR_BUSY         = 0x8002ce01,
	CELL_BGDL_UTIL_ERROR_INTERNAL     = 0x8002ce02,
	CELL_BGDL_UTIL_ERROR_PARAM        = 0x8002ce03,
	CELL_BGDL_UTIL_ERROR_ACCESS_ERROR = 0x8002ce04,
	CELL_BGDL_UTIL_ERROR_INITIALIZE   = 0x8002ce05,
};

enum CellBGDLState : s32
{
	CELL_BGDL_STATE_ERROR = 0,
	CELL_BGDL_STATE_PAUSE,
	CELL_BGDL_STATE_READY,
	CELL_BGDL_STATE_RUN,
	CELL_BGDL_STATE_COMPLETE,
};

enum CellBGDLMode : s32
{
	CELL_BGDL_MODE_AUTO = 0,
	CELL_BGDL_MODE_ALWAYS_ALLOW,
};

struct CellBGDLInfo
{
	be_t<u64> received_size;
	be_t<u64> content_size;
	be_t<s32> state; // CellBGDLState
	vm::bptr<void> reserved;
};
