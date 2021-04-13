#pragma once

#include "Emu/Memory/vm_ptr.h"

// CellFsRingBuffer.copy
enum : s32
{
	CELL_FS_ST_COPY     = 0,
	CELL_FS_ST_COPYLESS = 1,
};

struct CellFsRingBuffer
{
	be_t<u64> ringbuf_size;
	be_t<u64> block_size;
	be_t<u64> transfer_rate;
	be_t<s32> copy;
};

// cellFsStReadGetStatus status
enum : u64
{
	CELL_FS_ST_INITIALIZED     = 0x0001,
	CELL_FS_ST_NOT_INITIALIZED = 0x0002,
	CELL_FS_ST_STOP            = 0x0100,
	CELL_FS_ST_PROGRESS        = 0x0200,
};

enum : s32
{
	CELL_FS_AIO_MAX_FS      = 10, // cellFsAioInit limit
	CELL_FS_AIO_MAX_REQUEST = 32, // cellFsAioRead request limit per mount point
};

struct CellFsAio
{
	be_t<u32> fd;
	be_t<u64> offset;
	vm::bptrb<void> buf;
	be_t<u64> size;
	be_t<u64> user_data;
};
