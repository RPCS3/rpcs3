#pragma once

namespace vm { using namespace ps3; }

using sys_mempool_t = u32;

struct memory_pool_t
{
	vm::ptr<void> chunk;
	u64 chunk_size;
	u64 block_size;
	u64 ralignment;
	std::vector<vm::ptr<void>> free_blocks;
};
