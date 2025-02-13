#include "stdafx.h"

#include "Utilities/StrUtil.h"

#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_cond.h"

LOG_CHANNEL(sysPrxForUser);

using sys_mempool_t = u32;

struct memory_pool_t
{
	static const u32 id_base = 1;
	static const u32 id_step = 1;
	static const u32 id_count = 1023;
	SAVESTATE_INIT_POS(21);

	u32 mutexid;
	u32 condid;

	vm::ptr<void> chunk;
	u64 chunk_size;
	u64 block_size;
	u64 ralignment;
	std::vector<vm::ptr<void>> free_blocks;
};

error_code sys_mempool_create(ppu_thread& ppu, vm::ptr<sys_mempool_t> mempool, vm::ptr<void> chunk, const u64 chunk_size, const u64 block_size, const u64 ralignment)
{
	sysPrxForUser.warning("sys_mempool_create(mempool=*0x%x, chunk=*0x%x, chunk_size=%d, block_size=%d, ralignment=%d)", mempool, chunk, chunk_size, block_size, ralignment);

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
	if (!chunk.aligned(8))
	{
		return CELL_EINVAL;
	}

	auto id = idm::make<memory_pool_t>();
	*mempool = id;

	auto memory_pool = idm::get_unlocked<memory_pool_t>(id);

	memory_pool->chunk = chunk;
	memory_pool->chunk_size = chunk_size;
	memory_pool->block_size = block_size;
	memory_pool->ralignment = alignment;

	// TODO: check blocks alignment wrt ralignment
	u64 num_blocks = chunk_size / block_size;
	memory_pool->free_blocks.resize(num_blocks);
	for (u32 i = 0; i < num_blocks; ++i)
	{
		memory_pool->free_blocks[i] = vm::ptr<void>::make(chunk.addr() + i * static_cast<u32>(block_size));
	}

	// Create synchronization variables
	vm::var<u32> mutexid;
	vm::var<sys_mutex_attribute_t> attr;
	attr->protocol = SYS_SYNC_PRIORITY;
	attr->recursive = SYS_SYNC_NOT_RECURSIVE;
	attr->pshared = SYS_SYNC_NOT_PROCESS_SHARED;
	attr->adaptive = SYS_SYNC_NOT_ADAPTIVE;
	attr->ipc_key = 0; // No idea what this is
	attr->flags = 0; //  Also no idea what this is.
	strcpy_trunc(attr->name, "mp_m" + std::to_string(*mempool));

	error_code ret = sys_mutex_create(ppu, mutexid, attr);
	if (ret != 0)
	{ // TODO: Better exception handling.
		fmt::throw_exception("mempool %x failed to create mutex", mempool);
	}
	memory_pool->mutexid = *mutexid;

	vm::var<u32> condid;
	vm::var<sys_cond_attribute_t> condAttr;
	condAttr->pshared = SYS_SYNC_NOT_PROCESS_SHARED;
	condAttr->flags = 0; // No idea what this is
	condAttr->ipc_key = 0; // Also no idea what this is
	strcpy_trunc(condAttr->name, "mp_c" + std::to_string(*mempool));

	ret = sys_cond_create(ppu, condid, *mutexid, condAttr);
	if (ret != CELL_OK)
	{  // TODO: Better exception handling.
		fmt::throw_exception("mempool %x failed to create condition variable", mempool);
	}
	memory_pool->condid = *condid;

	return CELL_OK;
}

void sys_mempool_destroy(ppu_thread& ppu, sys_mempool_t mempool)
{
	sysPrxForUser.warning("sys_mempool_destroy(mempool=%d)", mempool);

	auto memory_pool = idm::get_unlocked<memory_pool_t>(mempool);
	if (memory_pool)
	{
		u32 condid = memory_pool->condid;
		u32 mutexid = memory_pool->mutexid;

		sys_mutex_lock(ppu, memory_pool->mutexid, 0);
		idm::remove_verify<memory_pool_t>(mempool, std::move(memory_pool));
		sys_mutex_unlock(ppu, mutexid);
		sys_mutex_destroy(ppu, mutexid);
		sys_cond_destroy(ppu, condid);
	}
	else
	{
		sysPrxForUser.error("Trying to destroy an already destroyed mempool=%d", mempool);
	}
}

error_code sys_mempool_free_block(ppu_thread& ppu, sys_mempool_t mempool, vm::ptr<void> block)
{
	sysPrxForUser.warning("sys_mempool_free_block(mempool=%d, block=*0x%x)", mempool, block);

	auto memory_pool = idm::get_unlocked<memory_pool_t>(mempool);
	if (!memory_pool)
	{
		return CELL_EINVAL;
	}

	sys_mutex_lock(ppu, memory_pool->mutexid, 0);

	// Cannot free a block not belonging to this memory pool
	if (block.addr() > memory_pool->chunk.addr() + memory_pool->chunk_size)
	{
		sys_mutex_unlock(ppu, memory_pool->mutexid);
		return CELL_EINVAL;
	}
	memory_pool->free_blocks.push_back(block);
	sys_cond_signal(ppu, memory_pool->condid);
	sys_mutex_unlock(ppu, memory_pool->mutexid);
	return CELL_OK;
}

u64 sys_mempool_get_count(ppu_thread& ppu, sys_mempool_t mempool)
{
	sysPrxForUser.warning("sys_mempool_get_count(mempool=%d)", mempool);

	auto memory_pool = idm::get_unlocked<memory_pool_t>(mempool);
	if (!memory_pool)
	{
		return CELL_EINVAL;
	}
	sys_mutex_lock(ppu, memory_pool->mutexid, 0);
	u64 ret = memory_pool->free_blocks.size();
	sys_mutex_unlock(ppu, memory_pool->mutexid);
	return ret;
}

vm::ptr<void> sys_mempool_allocate_block(ppu_thread& ppu, sys_mempool_t mempool)
{
	sysPrxForUser.warning("sys_mempool_allocate_block(mempool=%d)", mempool);

	auto memory_pool = idm::get_unlocked<memory_pool_t>(mempool);
	if (!memory_pool)
	{	// if the memory pool gets deleted-- is null, clearly it's impossible to allocate memory.
		return vm::null;
	}
	sys_mutex_lock(ppu, memory_pool->mutexid, 0);

	while (memory_pool->free_blocks.empty()) // while is to guard against spurious wakeups
	{
		sys_cond_wait(ppu, memory_pool->condid, 0);
		memory_pool = idm::get_unlocked<memory_pool_t>(mempool);
		if (!memory_pool)  // in case spurious wake up was from delete, don't die by accessing a freed pool.
		{ // No need to unlock as if the pool is freed, the lock was freed as well.
			return vm::null;
		}
	}

	auto block_ptr = memory_pool->free_blocks.back();
	memory_pool->free_blocks.pop_back();
	sys_mutex_unlock(ppu, memory_pool->mutexid);
	return block_ptr;
}

vm::ptr<void> sys_mempool_try_allocate_block(ppu_thread& ppu, sys_mempool_t mempool)
{
	sysPrxForUser.warning("sys_mempool_try_allocate_block(mempool=%d)", mempool);

	auto memory_pool = idm::get_unlocked<memory_pool_t>(mempool);

	if (!memory_pool || memory_pool->free_blocks.empty())
	{
		return vm::null;
	}

	sys_mutex_lock(ppu, memory_pool->mutexid, 0);

	auto block_ptr = memory_pool->free_blocks.back();
	memory_pool->free_blocks.pop_back();
	sys_mutex_unlock(ppu, memory_pool->mutexid);
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
