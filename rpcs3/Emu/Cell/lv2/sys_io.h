#pragma once

#include "Emu/Memory/vm_ptr.h"

struct lv2_io_buf
{
	static const u32 id_base = 0x44000000;
	static const u32 id_step = 1;
	static const u32 id_count = 2048;
	SAVESTATE_INIT_POS(41);

	const u32 block_count;
	const u32 block_size;
	const u32 blocks;
	const u32 unk1;

	lv2_io_buf(u32 block_count, u32 block_size, u32 blocks, u32 unk1)
		: block_count(block_count)
		, block_size(block_size)
		, blocks(blocks)
		, unk1(unk1)
	{
	}
};

// SysCalls

error_code sys_io_buffer_create(u32 block_count, u32 block_size, u32 blocks, u32 unk1, vm::ptr<u32> handle);
error_code sys_io_buffer_destroy(u32 handle);
error_code sys_io_buffer_allocate(u32 handle, vm::ptr<u32> block);
error_code sys_io_buffer_free(u32 handle, u32 block);
