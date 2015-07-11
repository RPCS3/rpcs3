#pragma once

namespace vm { using namespace ps3; }

enum : u32
{
	SYS_MEMORY_CONTAINER_ID_INVALID = 0xFFFFFFFF,
};

enum : u64
{
	SYS_MEMORY_ACCESS_RIGHT_NONE    = 0x00000000000000F0ULL,
	SYS_MEMORY_ACCESS_RIGHT_ANY     = 0x000000000000000FULL,
	SYS_MEMORY_ACCESS_RIGHT_PPU_THR = 0x0000000000000008ULL,
	SYS_MEMORY_ACCESS_RIGHT_HANDLER = 0x0000000000000004ULL,
	SYS_MEMORY_ACCESS_RIGHT_SPU_THR = 0x0000000000000002ULL,
	SYS_MEMORY_ACCESS_RIGHT_RAW_SPU = 0x0000000000000001ULL,

	SYS_MEMORY_ATTR_READ_ONLY       = 0x0000000000080000ULL,
	SYS_MEMORY_ATTR_READ_WRITE      = 0x0000000000040000ULL,
};

enum : u64
{
	SYS_MEMORY_PAGE_SIZE_1M   = 0x400ull,
	SYS_MEMORY_PAGE_SIZE_64K  = 0x200ull,
	SYS_MEMORY_PAGE_SIZE_MASK = 0xf00ull,
};

struct sys_memory_info_t
{
	be_t<u32> total_user_memory;
	be_t<u32> available_user_memory;
};


struct sys_page_attr_t
{
	be_t<u64> attribute;
	be_t<u64> access_right;
	be_t<u32> page_size;
	be_t<u32> pad;
};

struct lv2_memory_container_t
{
	const u32 size;	// amount of "physical" memory in this container
	const u32 id;

	// amount of memory allocated
	std::atomic<u32> used{ 0 };
	
	// allocations (addr -> size)
	std::map<u32, u32> allocs;

	lv2_memory_container_t(u32 size);
};

// SysCalls
s32 sys_memory_allocate(u32 size, u64 flags, vm::ptr<u32> alloc_addr);
s32 sys_memory_allocate_from_container(u32 size, u32 cid, u64 flags, vm::ptr<u32> alloc_addr);
s32 sys_memory_free(u32 start_addr);
s32 sys_memory_get_page_attribute(u32 addr, vm::ptr<sys_page_attr_t> attr);
s32 sys_memory_get_user_memory_size(vm::ptr<sys_memory_info_t> mem_info);
s32 sys_memory_container_create(vm::ptr<u32> cid, u32 size);
s32 sys_memory_container_destroy(u32 cid);
s32 sys_memory_container_get_size(vm::ptr<sys_memory_info_t> mem_info, u32 cid);
