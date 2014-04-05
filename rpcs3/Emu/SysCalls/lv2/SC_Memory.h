#pragma once

#define SYS_MEMORY_CONTAINER_ID_INVALID     0xFFFFFFFF
#define SYS_MEMORY_ACCESS_RIGHT_NONE        0x00000000000000F0ULL
#define SYS_MEMORY_ACCESS_RIGHT_PPU_THREAD  0x0000000000000008ULL
#define SYS_MEMORY_ACCESS_RIGHT_HANDLER     0x0000000000000004ULL
#define SYS_MEMORY_ACCESS_RIGHT_SPU_THREAD  0x0000000000000002ULL
#define SYS_MEMORY_ACCESS_RIGHT_SPU_RAW     0x0000000000000001ULL
#define SYS_MEMORY_ATTR_READ_ONLY           0x0000000000080000ULL
#define SYS_MEMORY_ATTR_READ_WRITE          0x0000000000040000ULL
#define SYS_MMAPPER_FIXED_ADDR              0xB0000000
#define SYS_MMAPPER_FIXED_SIZE              0x10000000
#define SYS_VM_TEST_INVALID                 0x0000ULL
#define SYS_VM_TEST_UNUSED                  0x0001ULL
#define SYS_VM_TEST_ALLOCATED               0x0002ULL
#define SYS_VM_TEST_STORED                  0x0004ULL

enum
{
	SYS_MEMORY_PAGE_SIZE_1M = 0x400,
	SYS_MEMORY_PAGE_SIZE_64K = 0x200,
};

struct sys_memory_info
{
	u32 total_user_memory;
	u32 available_user_memory;
};


struct sys_page_attr_t
{
	u64 attribute;
	u64 access_right;
	u32 page_size;
	u32 pad;
};

struct MemoryContainerInfo
{
	u64 addr;
	u32 size;

	MemoryContainerInfo(u64 addr, u32 size)
		: addr(addr)
		, size(size)
	{
	}
};

struct mmapper_info
{
	u64 addr;
	u32 size;
	u32 flags;

	mmapper_info(u64 _addr, u32 _size, u32 _flags)
		: addr(_addr)
		, size(_size)
		, flags(_flags)
	{
	}

	mmapper_info()
	{
	}
};

struct sys_vm_statistics {
	u64 vm_crash_ppu;
	u64 vm_crash_spu;
	u64 vm_read;
	u64 vm_write;
	u32 physical_mem_size;
	u32 physical_mem_used;
	u64 timestamp;
};