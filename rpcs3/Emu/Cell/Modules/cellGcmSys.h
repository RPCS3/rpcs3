#pragma once

#include "Emu/RSX/GCM.h"
#include "Emu/Memory/vm_ptr.h"

enum CellGcmError : u32
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

void InitOffsetTable();
