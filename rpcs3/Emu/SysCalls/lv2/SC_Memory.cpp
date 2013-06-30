#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
/*
struct MemContiner
{
	u8* continer;
	u32 num;

	void Create(u32 size)
	{
		continer = new u8[size];
	}

	void Delete()
	{
		if(continer != NULL) free(continer);
	}

	~MemContiner()
	{
		Delete();
	}
};

class MemContiners : private SysCallBase
{
	SysCallsArraysList<MemContiner> continers;

public:
	MemContiners() : SysCallBase("MemContainers")
	{
	}

	u64 AddContiner(const u64 size)
	{
		const u64 id = continers.Add();
		bool error;
		MemContiner& data = *continers.GetDataById(id, &error);
		if(error)
		{
			ConLog.Error("%s error: id [%d] is not found!", module_name, id);
			return 0;
		}

		data.Create(size);

		return id;
	}

	void DeleteContiner(const u64 id)
	{
		bool error;
		MemContiner& data = *continers.GetDataById(id, &error);
		if(error)
		{
			ConLog.Error("%s error: id [%d] is not found!", module_name, id);
			return;
		}
		data.Delete();
		continers.RemoveById(id);
	}
};

MemContiners continers;
*/
/*
int SysCalls::lv2MemContinerCreate(PPUThread& CPU)
{
	u64& continer = CPU.GPR[3];
	u32 size = CPU.GPR[4];

	ConLog.Warning("lv2MemContinerCreate[size: 0x%x]", size);
	//continer = continers.AddContiner(size);
	return 0;
}

int SysCalls::lv2MemContinerDestroy(PPUThread& CPU)
{
	u32 container = CPU.GPR[3];
	ConLog.Warning("lv2MemContinerDestroy[container: 0x%x]", container);
	//continers.DeleteContiner(container);
	return 0;
}*/
/*
static const u32 max_user_mem = 0x0d500000; //100mb
u32 free_user_mem = max_user_mem;
static u64 addr_user_mem = 0;

struct MemoryInfo
{
	u32 free_user_mem;
	u32 aviable_user_mem;
};

enum
{
	SYS_MEMORY_PAGE_SIZE_1M = 0x400,
	SYS_MEMORY_PAGE_SIZE_64K = 0x200,
};

int SysCalls::sys_memory_allocate(u32 size, u64 flags, u64 alloc_addr)
{
	//int sys_memory_allocate(size_t size, uint64_t flags, sys_addr_t * alloc_addr); 

	const u64 size = CPU.GPR[3];
	const u64 flags = CPU.GPR[4];
	const u64 alloc_addr = CPU.GPR[5];

	ConLog.Write("lv2MemAllocate: size: 0x%llx, flags: 0x%llx, alloc_addr: 0x%llx", size, flags, alloc_addr);

	//u32 addr = 0;
	switch(flags)
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(size & 0xfffff) return CELL_EALIGN;
		//addr = Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(size & 0xffff) return CELL_EALIGN;
		//addr = Memory.Alloc(size, 0x10000);
	break;

	default: return CELL_EINVAL;
	}

	u32 num = Memory.MemoryBlocks.GetCount();
	Memory.MemoryBlocks.Add(new MemoryBlock());
	Memory.MemoryBlocks[num].SetRange(Memory.MemoryBlocks[num - 1].GetEndAddr(), size);

	Memory.Write32(alloc_addr, Memory.MemoryBlocks[num].GetStartAddr());
	ConLog.Write("Test...");
	Memory.Write32(Memory.MemoryBlocks[num].GetStartAddr(), 0xfff);
	if(Memory.Read32(Memory.MemoryBlocks[num].GetStartAddr()) != 0xfff)
	{
		ConLog.Write("Test faild");
	}
	else
	{
		ConLog.Write("Test OK");
		Memory.Write32(Memory.MemoryBlocks[num].GetStartAddr(), 0x0);
	}

	return CELL_OK;
}

int SysCalls::sys_memory_get_user_memory_size(PPUThread& CPU)
{
	ConLog.Write("lv2MemGetUserMemorySize: r3=0x%llx", CPU.GPR[3]);
	//int sys_memory_get_user_memory_size(sys_memory_info_t * mem_info); 
	MemoryInfo& memoryinfo = *(MemoryInfo*)Memory.GetMemFromAddr(CPU.GPR[3]);
	memoryinfo.aviable_user_mem = Memory.Reverse32(free_user_mem);
	memoryinfo.free_user_mem = Memory.Reverse32(free_user_mem);
	return CELL_OK;
}*/

SysCallBase sc_mem("memory");

enum
{
	SYS_MEMORY_PAGE_SIZE_1M = 0x400,
	SYS_MEMORY_PAGE_SIZE_64K = 0x200,
};

int sys_memory_container_create(u32 cid_addr, u32 yield_size)
{
	sc_mem.Warning("TODO: sys_memory_container_create(cid_addr=0x%x,yield_size=0x%x)", cid_addr, yield_size);
	return CELL_OK;
}

int sys_memory_container_destroy(u32 cid)
{
	sc_mem.Warning("TODO: sys_memory_container_destroy(cid=0x%x)", cid);
	return CELL_OK;
}

int sys_memory_allocate(u32 size, u32 flags, u32 alloc_addr_addr)
{
	//0x30000100;
	sc_mem.Log("sys_memory_allocate(size=0x%x, flags=0x%x)", size, flags);
	u32 addr;
	switch(flags)
	{
	case SYS_MEMORY_PAGE_SIZE_1M:
		if(size & 0xfffff) return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x100000);
	break;

	case SYS_MEMORY_PAGE_SIZE_64K:
		if(size & 0xffff) return CELL_EALIGN;
		addr = Memory.Alloc(size, 0x10000);
	break;

	default: return CELL_EINVAL;
	}

	if(!addr) return CELL_ENOMEM;
	sc_mem.Log("Memory allocated! [addr: 0x%x, size: 0x%x]", addr, size);
	Memory.Write32(alloc_addr_addr, addr);

	return CELL_OK;
}

int sys_memory_free(u32 start_addr)
{
	sc_mem.Log("sys_memory_free(start_addr=0x%x)", start_addr);

	if(!Memory.Free(start_addr)) return CELL_EFAULT;

	return CELL_OK;
}

struct mmapper_info
{
	u32 size;
	u32 flags;

	mmapper_info(u32 _size, u32 _flags)
		: size(_size)
		, flags(_flags)
	{
	}

	mmapper_info()
	{
	}
};

int sys_mmapper_allocate_address(u32 size, u64 flags, u32 alignment, u32 alloc_addr)
{
	sc_mem.Warning("sys_mmapper_allocate_address(size=0x%x, flags=0x%llx, alignment=0x%x, alloc_addr=0x%x)", size, flags, alignment, alloc_addr);

	Memory.Write32(alloc_addr, Memory.Alloc(size, 4));

	return CELL_OK;
}

int sys_mmapper_allocate_memory(u32 size, u64 flags, u32 mem_id_addr)
{
	sc_mem.Warning("sys_mmapper_allocate_memory(size=0x%x, flags=0x%llx, mem_id_addr=0x%x)", size, flags, mem_id_addr);

	if(!Memory.IsGoodAddr(mem_id_addr)) return CELL_EFAULT;
	Memory.Write32(mem_id_addr, sc_mem.GetNewId(new mmapper_info(size, flags)));

	return CELL_OK;
}

int sys_mmapper_map_memory(u32 start_addr, u32 mem_id, u64 flags)
{
	sc_mem.Warning("sys_mmapper_map_memory(start_addr=0x%x, mem_id=0x%x, flags=0x%llx)", start_addr, mem_id, flags);

	mmapper_info* info;
	if(!sc_mem.CheckId(mem_id, info)) return CELL_ESRCH;

	//Memory.MemoryBlocks.Add((new MemoryBlock())->SetRange(start_addr, info->size));

	return CELL_OK;
}

struct sys_memory_info
{
	u32 total_user_memory;
	u32 available_user_memory;
};

int sys_memory_get_user_memory_size(u32 mem_info_addr)
{
	sys_memory_info info;
	info.total_user_memory = re(Memory.GetUserMemTotalSize());
	info.available_user_memory = re(Memory.GetUserMemAvailSize());
	Memory.WriteData(mem_info_addr, info);
	return CELL_OK;
}