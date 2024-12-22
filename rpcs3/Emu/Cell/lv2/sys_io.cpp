#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/ErrorCodes.h"

#include "sys_io.h"

LOG_CHANNEL(sys_io);

error_code sys_io_buffer_create(u32 block_count, u32 block_size, u32 blocks, u32 unk1, vm::ptr<u32> handle)
{
	sys_io.todo("sys_io_buffer_create(block_count=0x%x, block_size=0x%x, blocks=0x%x, unk1=0x%x, handle=*0x%x)", block_count, block_size, blocks, unk1, handle);

	if (!handle)
	{
		return CELL_EFAULT;
	}

	if (auto io = idm::make<lv2_io_buf>(block_count, block_size, blocks, unk1))
	{
		*handle = io;
		return CELL_OK;
	}

	return CELL_ESRCH;
}

error_code sys_io_buffer_destroy(u32 handle)
{
	sys_io.todo("sys_io_buffer_destroy(handle=0x%x)", handle);

	idm::remove<lv2_io_buf>(handle);

	return CELL_OK;
}

error_code sys_io_buffer_allocate(u32 handle, vm::ptr<u32> block)
{
	sys_io.todo("sys_io_buffer_allocate(handle=0x%x, block=*0x%x)", handle, block);

	if (!block)
	{
		return CELL_EFAULT;
	}

	if (auto io = idm::get_unlocked<lv2_io_buf>(handle))
	{
		// no idea what we actually need to allocate
		if (u32 addr = vm::alloc(io->block_count * io->block_size, vm::main))
		{
			*block = addr;
			return CELL_OK;
		}

		return CELL_ENOMEM;
	}

	return CELL_ESRCH;
}

error_code sys_io_buffer_free(u32 handle, u32 block)
{
	sys_io.todo("sys_io_buffer_free(handle=0x%x, block=0x%x)", handle, block);

	const auto io = idm::get_unlocked<lv2_io_buf>(handle);

	if (!io)
	{
		return CELL_ESRCH;
	}

	vm::dealloc(block);

	return CELL_OK;
}
