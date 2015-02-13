#pragma once
#include "Emu/RSX/GCM.h"

enum
{
	CELL_GCM_ERROR_FAILURE           = 0x802100ff,
	CELL_GCM_ERROR_NO_IO_PAGE_TABLE  = 0x80210001,
	CELL_GCM_ERROR_INVALID_ENUM      = 0x80210002,
	CELL_GCM_ERROR_INVALID_VALUE     = 0x80210003,
	CELL_GCM_ERROR_INVALID_ALIGNMENT = 0x80210004,
	CELL_GCM_ERROR_ADDRESS_OVERWRAP  = 0x80210005,
};

struct CellGcmOffsetTable
{
	vm::bptr<u16> ioAddress;
	vm::bptr<u16> eaAddress;
};

// Auxiliary functions
s32 gcmMapEaIoAddress(u32 ea, u32 io, u32 size, bool is_strict);

// Syscall
s32 cellGcmCallback(vm::ptr<CellGcmContextData> context, u32 count);
