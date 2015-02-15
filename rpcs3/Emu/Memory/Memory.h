#pragma once

#include "MemoryBlock.h"

using std::nullptr_t;

#define safe_delete(x) do {delete (x);(x)=nullptr;} while(0)
#define safe_free(x) do {free(x);(x)=nullptr;} while(0)

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

public:
	MemoryBlock* UserMemory;

	DynamicMemoryBlock MainMem;
	DynamicMemoryBlock Userspace;
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

	void RegisterPages(u32 addr, u32 size);

	void UnregisterPages(u32 addr, u32 size);

	u32 InitRawSPU(MemoryBlock* raw_spu);

	void CloseRawSPU(MemoryBlock* raw_spu, const u32 num);

	void Init(MemoryType type);

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

	u32 Alloc(const u32 size, const u32 align)
	{
		return UserMemory->AllocAlign(size, align);
	}

	bool Free(const u32 addr)
	{
		return UserMemory->Free(addr);
	}

	bool Map(const u32 addr, const u32 size);

	bool Unmap(const u32 addr);
};

extern MemoryBase Memory;

#include "vm.h"

