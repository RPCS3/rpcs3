#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/ErrorCodes.h"

#include "sys_io.h"

LOG_CHANNEL(sys_io);

error_code sys_io_buffer_create(u32 block_count, u32 block_size, u32 blocks, u32 unk1, vm::ptr<u32> handle)
{
	sys_io.todo("sys_io_buffer_create(block_count=0x%x, block_size=0x%x, blocks=0x%x, unk1=0x%x, handle=*0x%x)", block_count, block_size, blocks, unk1, handle);

	return CELL_OK;
}

error_code sys_io_buffer_destroy(u32 handle)
{
	sys_io.todo("sys_io_buffer_destroy(handle=0x%x)", handle);

	return CELL_OK;
}

error_code sys_io_buffer_allocate(u32 handle, vm::ptr<u32> block)
{
	sys_io.todo("sys_io_buffer_allocate(handle=0x%x, block=*0x%x)", handle, block);

	return CELL_OK;
}

error_code sys_io_buffer_free(u32 handle, u32 block)
{
	sys_io.todo("sys_io_buffer_free(handle=0x%x, block=0x%x)", handle, block);

	return CELL_OK;
}
