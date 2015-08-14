#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"

#include "sysPrxForUser.h"
#include "sys_mempool.h"

extern Module sysPrxForUser;

s32 sys_mempool_allocate_block()
{
	throw EXCEPTION("");
}

s32 sys_mempool_create(vm::ptr<sys_mempool_t> mempool, vm::ptr<void> chunk, const u64 chunk_size, const u64 block_size, const u64 ralignment)
{
	sysPrxForUser.Warning("sys_mempool_create(mempool=*0x%x, chunk=*0x%x, chunk_size=%d, block_size=%d, ralignment=%d)", mempool, chunk, chunk_size, block_size, ralignment);

	if (block_size > chunk_size)
	{
		return CELL_EINVAL;
	}

	u64 alignment = ralignment;
	if (ralignment == 0 || ralignment == 2)
	{
		alignment = 4;
	}

	// Check if alignment is power of two
	if ((alignment & (alignment - 1)) != 0)
	{
		return CELL_EINVAL;
	}

	// Test chunk address aligment
	if (chunk % 8)
	{
		return CELL_EINVAL;
	}

	auto id = Emu.GetIdManager().make<memory_pool_t>();
	*mempool = id;

	auto memory_pool = Emu.GetIdManager().get<memory_pool_t>(id);

	memory_pool->chunk = chunk;
	memory_pool->chunk_size = chunk_size;
	memory_pool->block_size = block_size;
	memory_pool->ralignment = alignment;

	// TODO: check blocks alignment wrt ralignment
	u64 num_blocks = chunk_size / block_size;
	memory_pool->free_blocks.resize(num_blocks);
	for (int i = 0; i < num_blocks; ++i)
	{
		memory_pool->free_blocks[i] = vm::ptr<void>::make(chunk.addr() + i * block_size);
	}
	
	return CELL_OK;
}

void sys_mempool_destroy(sys_mempool_t mempool)
{
	sysPrxForUser.Warning("sys_mempool_destroy(mempool=%d)", mempool);

	Emu.GetIdManager().remove<memory_pool_t>(mempool);
}

s32 sys_mempool_free_block(sys_mempool_t mempool, vm::ptr<void> block)
{
	sysPrxForUser.Warning("sys_mempool_free_block(mempool=%d, block=*0x%x)", mempool, block);

	auto memory_pool = Emu.GetIdManager().get<memory_pool_t>(mempool);
	if (!memory_pool)
	{
		return CELL_EINVAL;
	}

	// Cannot free a block not belonging to this memory pool
	if (block.addr() > memory_pool->chunk.addr() + memory_pool->chunk_size)
	{
		return CELL_EINVAL;
	}
	memory_pool->free_blocks.push_back(block);
	return CELL_OK;
}

u64 sys_mempool_get_count(sys_mempool_t mempool)
{
	sysPrxForUser.Warning("sys_mempool_get_count(mempool=%d)", mempool);

	auto memory_pool = Emu.GetIdManager().get<memory_pool_t>(mempool);
	if (!memory_pool)
	{
		return CELL_EINVAL;
	}

	return memory_pool->free_blocks.size();
}

vm::ptr<void> sys_mempool_try_allocate_block(sys_mempool_t mempool)
{
	sysPrxForUser.Warning("sys_mempool_try_allocate_block(mempool=%d)", mempool);

	auto memory_pool = Emu.GetIdManager().get<memory_pool_t>(mempool);

	if (!memory_pool || memory_pool->free_blocks.size() == 0)
	{
		return vm::null;
	}

	auto block_ptr = memory_pool->free_blocks.back();
	memory_pool->free_blocks.pop_back();
	return block_ptr;
}


void sysPrxForUser_sys_mempool_init()
{
	REG_FUNC(sysPrxForUser, sys_mempool_allocate_block);
	REG_FUNC(sysPrxForUser, sys_mempool_create);
	REG_FUNC(sysPrxForUser, sys_mempool_destroy);
	REG_FUNC(sysPrxForUser, sys_mempool_free_block);
	REG_FUNC(sysPrxForUser, sys_mempool_get_count);
	REG_FUNC(sysPrxForUser, sys_mempool_try_allocate_block);
}
