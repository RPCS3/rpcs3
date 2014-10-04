#pragma once

#include "MemoryBlock.h"

using std::nullptr_t;

#define safe_delete(x) do {delete (x);(x)=nullptr;} while(0)
#define safe_free(x) do {free(x);(x)=nullptr;} while(0)

extern void* const g_base_addr;

enum MemoryType
{
	Memory_PS3,
	Memory_PSV,
	Memory_PSP,
};

enum : u32
{
	RAW_SPU_OFFSET = 0x00100000,
	RAW_SPU_BASE_ADDR = 0xE0000000,
	RAW_SPU_LS_OFFSET = 0x00000000,
	RAW_SPU_PROB_OFFSET = 0x00040000,
};

class MemoryBase
{
	std::vector<MemoryBlock*> MemoryBlocks;
	u32 m_pages[0x100000000 / 4096]; // information about every page

public:
	MemoryBlock* UserMemory;

	DynamicMemoryBlock MainMem;
	DynamicMemoryBlock PRXMem;
	DynamicMemoryBlock RSXCMDMem;
	DynamicMemoryBlock RSXFBMem;
	DynamicMemoryBlock StackMem;
	MemoryBlock* RawSPUMem[(0x100000000 - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET];
	VirtualMemoryBlock RSXIOMem;

	struct
	{
		DynamicMemoryBlock RAM;
		DynamicMemoryBlock Userspace;
	} PSV;

	struct
	{
		DynamicMemoryBlock Scratchpad;
		DynamicMemoryBlock VRAM;
		DynamicMemoryBlock RAM;
		DynamicMemoryBlock Kernel;
		DynamicMemoryBlock Userspace;
	} PSP;

	bool m_inited;

	MemoryBase()
	{
		m_inited = false;
	}

	~MemoryBase()
	{
		Close();
	}

	static void* const GetBaseAddr()
	{
		return g_base_addr;
	}

	__noinline void InvalidAddress(const char* func, const u64 addr);

	void RegisterPages(u64 addr, u32 size);

	void UnregisterPages(u64 addr, u32 size);

	u32 RealToVirtualAddr(const void* addr)
	{
		const u64 res = (u64)addr - (u64)GetBaseAddr();
		
		if ((u32)res == res)
		{
			return (u32)res;
		}
		else
		{
			assert(!addr);
			return 0;
		}
	}

	u32 InitRawSPU(MemoryBlock* raw_spu);

	void CloseRawSPU(MemoryBlock* raw_spu, const u32 num);

	void Init(MemoryType type);

	bool IsGoodAddr(const u32 addr)
	{
		return m_pages[addr / 4096] != 0; // TODO: define page parameters
	}

	bool IsGoodAddr(const u32 addr, const u32 size)
	{
		if (!size || addr + size - 1 < addr)
		{
			return false;
		}
		else
		{
			for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
			{
				if (!m_pages[i]) return false; // TODO: define page parameters
			}
			return true;
		}
	}

	void Close();

	__noinline void WriteMMIO32(u32 addr, const u32 data);

	__noinline u32 ReadMMIO32(u32 addr);

	u32 GetUserMemTotalSize()
	{
		return UserMemory->GetSize();
	}

	u32 GetUserMemAvailSize()
	{
		return UserMemory->GetSize() - UserMemory->GetUsedSize();
	}

	u64 Alloc(const u32 size, const u32 align)
	{
		return UserMemory->AllocAlign(size, align);
	}

	bool Free(const u64 addr)
	{
		return UserMemory->Free(addr);
	}

	bool Map(const u64 addr, const u32 size);

	bool Unmap(const u64 addr);
};

extern MemoryBase Memory;

#include "vm.h"

