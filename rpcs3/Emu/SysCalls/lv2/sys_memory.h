#pragma once

#define SYS_MEMORY_CONTAINER_ID_INVALID     0xFFFFFFFF
#define SYS_MEMORY_ACCESS_RIGHT_NONE        0x00000000000000F0ULL
#define SYS_MEMORY_ACCESS_RIGHT_PPU_THREAD  0x0000000000000008ULL
#define SYS_MEMORY_ACCESS_RIGHT_HANDLER     0x0000000000000004ULL
#define SYS_MEMORY_ACCESS_RIGHT_SPU_THREAD  0x0000000000000002ULL
#define SYS_MEMORY_ACCESS_RIGHT_SPU_RAW     0x0000000000000001ULL
#define SYS_MEMORY_ATTR_READ_ONLY           0x0000000000080000ULL
#define SYS_MEMORY_ATTR_READ_WRITE          0x0000000000040000ULL

enum
{
	SYS_MEMORY_PAGE_SIZE_1M = 0x400,
	SYS_MEMORY_PAGE_SIZE_64K = 0x200,
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

struct MemoryContainerInfo
{
	u32 addr;
	u32 size;

	MemoryContainerInfo(u32 addr, u32 size)
		: addr(addr)
		, size(size)
	{
	}
};

// SysCalls
s32 sys_memory_allocate(u32 size, u32 flags, u32 alloc_addr_addr);
s32 sys_memory_allocate_from_container(u32 size, u32 cid, u32 flags, u32 alloc_addr_addr);
s32 sys_memory_free(u32 start_addr);
s32 sys_memory_get_page_attribute(u32 addr, vm::ptr<sys_page_attr_t> attr);
s32 sys_memory_get_user_memory_size(vm::ptr<sys_memory_info_t> mem_info);
s32 sys_memory_container_create(vm::ptr<be_t<u32>> cid, u32 yield_size);
s32 sys_memory_container_destroy(u32 cid);
s32 sys_memory_container_get_size(vm::ptr<sys_memory_info_t> mem_info, u32 cid);
