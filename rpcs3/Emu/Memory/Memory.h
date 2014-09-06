#pragma once

#include "MemoryBlock.h"
#include "Emu/SysCalls/Callback.h"

using std::nullptr_t;

#define safe_delete(x) do {delete (x);(x)=nullptr;} while(0)
#define safe_free(x) do {free(x);(x)=nullptr;} while(0)

extern void* const m_base_addr;

enum MemoryType
{
	Memory_PS3,
	Memory_PSV,
	Memory_PSP,
};

enum : u64
{
	RAW_SPU_OFFSET = 0x0000000000100000,
	RAW_SPU_BASE_ADDR = 0x00000000E0000000,
	RAW_SPU_LS_OFFSET = 0x0000000000000000,
	RAW_SPU_PROB_OFFSET = 0x0000000000040000,
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
	DynamicMemoryBlock MmaperMem;
	DynamicMemoryBlock RSXFBMem;
	DynamicMemoryBlock StackMem;
	MemoryBlock* RawSPUMem[(0x100000000 - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET];
	VirtualMemoryBlock RSXIOMem;

	struct Wrapper32LE
	{
		void Write8(const u32 addr, const u8 data) { *(u8*)((u8*)m_base_addr + addr) = data; }
		void Write16(const u32 addr, const u16 data) { *(u16*)((u8*)m_base_addr + addr) = data; }
		void Write32(const u32 addr, const u32 data) { *(u32*)((u8*)m_base_addr + addr) = data; }
		void Write64(const u32 addr, const u64 data) { *(u64*)((u8*)m_base_addr + addr) = data; }
		void Write128(const u32 addr, const u128 data) { *(u128*)((u8*)m_base_addr + addr) = data; }

		u8 Read8(const u32 addr) { return *(u8*)((u8*)m_base_addr + addr); }
		u16 Read16(const u32 addr) { return *(u16*)((u8*)m_base_addr + addr); }
		u32 Read32(const u32 addr) { return *(u32*)((u8*)m_base_addr + addr); }
		u64 Read64(const u32 addr) { return *(u64*)((u8*)m_base_addr + addr); }
		u128 Read128(const u32 addr) { return *(u128*)((u8*)m_base_addr + addr); }
	};

	struct : Wrapper32LE
	{
		DynamicMemoryBlock RAM;
		DynamicMemoryBlock Userspace;
	} PSV;

	struct : Wrapper32LE
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
		return m_base_addr;
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

	template<typename T> bool IsGoodAddr(const T addr)
	{
		if ((u32)addr != addr || !m_pages[addr / 4096]) // TODO: define page parameters
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	template<typename T> bool IsGoodAddr(const T addr, const u32 size)
	{
		if ((u32)addr != addr || (u64)addr + (u64)size > 0x100000000ull)
		{
			return false;
		}
		else
		{
			for (u32 i = (u32)addr / 4096; i <= ((u32)addr + size - 1) / 4096; i++)
			{
				if (!m_pages[i]) return false; // TODO: define page parameters
			}
			return true;
		}
	}

	void Close();

	//MemoryBase
	template<typename T> void Write8(T addr, const u8 data)
	{
		if ((u32)addr == addr)
		{
			*(u8*)((u8*)GetBaseAddr() + addr) = data;
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u8*)GetBaseAddr() = data;
		}
	}

	template<typename T> void Write16(T addr, const u16 data)
	{
		if ((u32)addr == addr)
		{
			*(u16*)((u8*)GetBaseAddr() + addr) = re16(data);
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u16*)GetBaseAddr() = data;
		}
	}

	__noinline void WriteMMIO32(u32 addr, const u32 data);

	template<typename T> void Write32(T addr, const u32 data)
	{
		if ((u32)addr == addr)
		{
			if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET)
			{
				*(u32*)((u8*)GetBaseAddr() + addr) = re32(data);
			}
			else
			{
				WriteMMIO32((u32)addr, data);
			}
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u32*)GetBaseAddr() = data;
		}
	}

	template<typename T> void Write64(T addr, const u64 data)
	{
		if ((u32)addr == addr)
		{
			*(u64*)((u8*)GetBaseAddr() + addr) = re64(data);
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u64*)GetBaseAddr() = data;
		}
	}

	template<typename T> void Write128(T addr, const u128 data)
	{
		if ((u32)addr == addr)
		{
			*(u128*)((u8*)GetBaseAddr() + addr) = re128(data);
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u128*)GetBaseAddr() = data;
		}
	}

	template<typename T> u8 Read8(T addr)
	{
		if ((u32)addr == addr)
		{
			return *(u8*)((u8*)GetBaseAddr() + addr);
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u8*)GetBaseAddr();
		}
	}

	template<typename T> u16 Read16(T addr)
	{
		if ((u32)addr == addr)
		{
			return re16(*(u16*)((u8*)GetBaseAddr() + addr));
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u16*)GetBaseAddr();
		}
	}

	__noinline u32 ReadMMIO32(u32 addr);

	template<typename T> u32 Read32(T addr)
	{
		if ((u32)addr == addr)
		{
			if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET)
			{
				return re32(*(u32*)((u8*)GetBaseAddr() + addr));
			}
			else
			{
				return ReadMMIO32((u32)addr);
			}
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u32*)GetBaseAddr();
		}
	}

	template<typename T> u64 Read64(T addr)
	{
		if ((u32)addr == addr)
		{
			return re64(*(u64*)((u8*)GetBaseAddr() + addr));
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u64*)GetBaseAddr();
		}
	}

	template<typename T> u128 Read128(T addr)
	{
		if ((u32)addr == addr)
		{
			return re128(*(u128*)((u8*)GetBaseAddr() + addr));
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u128*)GetBaseAddr();
		}
	}

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

