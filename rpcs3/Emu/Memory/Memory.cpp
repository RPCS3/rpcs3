#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/System.h"
#include "Memory.h"
#include "Emu/Cell/RawSPUThread.h"

#ifndef _WIN32
#include <sys/mman.h>

/* OS X uses MAP_ANON instead of MAP_ANONYMOUS */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#else
#include <Windows.h>
#endif

MemoryBase Memory;

void MemoryBase::InvalidAddress(const char* func, const u64 addr)
{
	LOG_ERROR(MEMORY, "%s(): invalid address (0x%llx)", func, addr);
}

void MemoryBase::RegisterPages(u64 addr, u32 size)
{
	LV2_LOCK();

	//LOG_NOTICE(MEMORY, "RegisterPages(addr=0x%llx, size=0x%x)", addr, size);
	for (u64 i = addr / 4096; i < (addr + size) / 4096; i++)
	{
		if (i >= sizeof(m_pages) / sizeof(m_pages[0]))
		{
			InvalidAddress(__FUNCTION__, i * 4096);
			break;
		}
		if (m_pages[i])
		{
			LOG_ERROR(MEMORY, "Page already registered (addr=0x%llx)", i * 4096);
			Emu.Pause();
		}
		m_pages[i] = 1; // TODO: define page parameters
	}
}

void MemoryBase::UnregisterPages(u64 addr, u32 size)
{
	LV2_LOCK();

	//LOG_NOTICE(MEMORY, "UnregisterPages(addr=0x%llx, size=0x%x)", addr, size);
	for (u64 i = addr / 4096; i < (addr + size) / 4096; i++)
	{
		if (i >= sizeof(m_pages) / sizeof(m_pages[0]))
		{
			InvalidAddress(__FUNCTION__, i * 4096);
			break;
		}
		if (!m_pages[i])
		{
			LOG_ERROR(MEMORY, "Page not registered (addr=0x%llx)", i * 4096);
			Emu.Pause();
		}
		m_pages[i] = 0; // TODO: define page parameters
	}
}

u32 MemoryBase::InitRawSPU(MemoryBlock* raw_spu)
{
	LV2_LOCK();

	u32 index;
	for (index = 0; index < sizeof(RawSPUMem) / sizeof(RawSPUMem[0]); index++)
	{
		if (!RawSPUMem[index])
		{
			RawSPUMem[index] = raw_spu;
			break;
		}
	}

	MemoryBlocks.push_back(raw_spu->SetRange(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index, RAW_SPU_PROB_OFFSET));
	return index;
}

void MemoryBase::CloseRawSPU(MemoryBlock* raw_spu, const u32 num)
{
	LV2_LOCK();

	for (int i = 0; i < MemoryBlocks.size(); ++i)
	{
		if (MemoryBlocks[i] == raw_spu)
		{
			MemoryBlocks.erase(MemoryBlocks.begin() + i);
			break;
		}
	}
	if (num < sizeof(RawSPUMem) / sizeof(RawSPUMem[0])) RawSPUMem[num] = nullptr;
}

void MemoryBase::Init(MemoryType type)
{
	LV2_LOCK();

	if (m_inited) return;
	m_inited = true;

	memset(m_pages, 0, sizeof(m_pages));
	memset(RawSPUMem, 0, sizeof(RawSPUMem));

#ifdef _WIN32
	if (!m_base_addr)
#else
	if ((s64)m_base_addr == (s64)-1)
#endif
	{
		LOG_ERROR(MEMORY, "Initializing memory failed");
		assert(0);
		return;
	}
	else
	{
		LOG_NOTICE(MEMORY, "Initializing memory: m_base_addr = 0x%llx", (u64)m_base_addr);
	}

	switch (type)
	{
	case Memory_PS3:
		MemoryBlocks.push_back(MainMem.SetRange(0x00010000, 0x2FFF0000));
		MemoryBlocks.push_back(UserMemory = PRXMem.SetRange(0x30000000, 0x10000000));
		MemoryBlocks.push_back(RSXCMDMem.SetRange(0x40000000, 0x10000000));
		MemoryBlocks.push_back(MmaperMem.SetRange(0xB0000000, 0x10000000));
		MemoryBlocks.push_back(RSXFBMem.SetRange(0xC0000000, 0x10000000));
		MemoryBlocks.push_back(StackMem.SetRange(0xD0000000, 0x10000000));
		break;

	case Memory_PSV:
		MemoryBlocks.push_back(PSV.RAM.SetRange(0x81000000, 0x10000000));
		MemoryBlocks.push_back(UserMemory = PSV.Userspace.SetRange(0x91000000, 0x10000000));
		break;

	case Memory_PSP:
		MemoryBlocks.push_back(PSP.Scratchpad.SetRange(0x00010000, 0x00004000));
		MemoryBlocks.push_back(PSP.VRAM.SetRange(0x04000000, 0x00200000));
		MemoryBlocks.push_back(PSP.RAM.SetRange(0x08000000, 0x02000000));
		MemoryBlocks.push_back(PSP.Kernel.SetRange(0x88000000, 0x00800000));
		MemoryBlocks.push_back(UserMemory = PSP.Userspace.SetRange(0x08800000, 0x01800000));
		break;
	}

	LOG_NOTICE(MEMORY, "Memory initialized.");
}

void MemoryBase::Close()
{
	LV2_LOCK();

	if (!m_inited) return;
	m_inited = false;

	LOG_NOTICE(MEMORY, "Closing memory...");

	for (auto block : MemoryBlocks)
	{
		block->Delete();
	}

	RSXIOMem.Delete();

	MemoryBlocks.clear();
}

void MemoryBase::WriteMMIO32(u32 addr, const u32 data)
{
	{
		LV2_LOCK();

		if (RawSPUMem[(addr - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET] &&
			((RawSPUThread*)RawSPUMem[(addr - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET])->Write32(addr, data))
		{
			return;
		}
	}

	*(u32*)((u8*)GetBaseAddr() + addr) = re32(data); // provoke error
}

u32 MemoryBase::ReadMMIO32(u32 addr)
{
	u32 res;
	{
		LV2_LOCK();

		if (RawSPUMem[(addr - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET] &&
			((RawSPUThread*)RawSPUMem[(addr - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET])->Read32(addr, &res))
		{
			return res;
		}
	}

	res = re32(*(u32*)((u8*)GetBaseAddr() + addr)); // provoke error
	return res;
}

bool MemoryBase::Map(const u64 addr, const u32 size)
{
	LV2_LOCK();

	if ((u32)addr != addr || (u64)addr + (u64)size > 0x100000000ull)
	{
		return false;
	}
	else
	{
		for (u32 i = (u32)addr / 4096; i <= ((u32)addr + size - 1) / 4096; i++)
		{
			if (m_pages[i]) return false;
		}
	}

	MemoryBlocks.push_back((new MemoryBlock())->SetRange(addr, size));
	LOG_WARNING(MEMORY, "MemoryBase::Map(0x%llx, 0x%x)", addr, size);
	return true;
}

bool MemoryBase::Unmap(const u64 addr)
{
	LV2_LOCK();

	for (u32 i = 0; i < MemoryBlocks.size(); i++)
	{
		if (MemoryBlocks[i]->GetStartAddr() == addr)
		{
			delete MemoryBlocks[i];
			MemoryBlocks.erase(MemoryBlocks.begin() + i);
			return true;
		}
	}
	return false;
}

MemBlockInfo::MemBlockInfo(u64 _addr, u32 _size)
	: MemInfo(_addr, PAGE_4K(_size))
{
	void* real_addr = (void*)((u64)Memory.GetBaseAddr() + _addr);
#ifdef _WIN32
	mem = VirtualAlloc(real_addr, size, MEM_COMMIT, PAGE_READWRITE);
#else
	if (::mprotect(real_addr, size, PROT_READ | PROT_WRITE))
	{
		mem = nullptr;
	}
	else
	{
		mem = real_addr;
	}
#endif
	if (mem != real_addr)
	{
		LOG_ERROR(MEMORY, "Memory allocation failed (addr=0x%llx, size=0x%llx)", addr, size);
		Emu.Pause();
	}
	else
	{
		Memory.RegisterPages(_addr, PAGE_4K(_size));
		memset(mem, 0, size);
	}
}

void MemBlockInfo::Free()
{
	if (mem)
	{
		Memory.UnregisterPages(addr, size);
#ifdef _WIN32
		if (!VirtualFree(mem, size, MEM_DECOMMIT))
#else
		if (::mprotect(mem, size, PROT_NONE))
#endif
		{
			LOG_ERROR(MEMORY, "Memory deallocation failed (addr=0x%llx, size=0x%llx)", addr, size);
			Emu.Pause();
		}
	}
}

//MemoryBlock
MemoryBlock::MemoryBlock() : mem_inf(nullptr)
{
	Init();
}

MemoryBlock::~MemoryBlock()
{
	Delete();
}

void MemoryBlock::Init()
{
	range_start = 0;
	range_size = 0;

	mem = Memory.GetMemFromAddr(0);
}

void MemoryBlock::InitMemory()
{
	if (!range_size)
	{
		mem = Memory.GetMemFromAddr(range_start);
	}
	else
	{
		Free();
		mem_inf = new MemBlockInfo(range_start, range_size);
		mem = (u8*)mem_inf->mem;
	}
}

void MemoryBlock::Free()
{
	if (mem_inf)
	{
		delete mem_inf;
		mem_inf = nullptr;
	}
}

void MemoryBlock::Delete()
{
	Free();
	Init();
}

u64 MemoryBlock::FixAddr(const u64 addr) const
{
	return addr - GetStartAddr();
}

MemoryBlock* MemoryBlock::SetRange(const u64 start, const u32 size)
{
	if (start + size > 0x100000000) return nullptr;

	range_start = start;
	range_size = size;

	InitMemory();
	return this;
}

bool MemoryBlock::IsMyAddress(const u64 addr)
{
	return mem && addr >= GetStartAddr() && addr < GetEndAddr();
}

DynamicMemoryBlockBase::DynamicMemoryBlockBase()
	: MemoryBlock()
	, m_max_size(0)
{
}

const u32 DynamicMemoryBlockBase::GetUsedSize() const
{
	LV2_LOCK();

	u32 size = 0;

	for (u32 i = 0; i<m_allocated.size(); ++i)
	{
		size += m_allocated[i].size;
	}

	return size;
}

bool DynamicMemoryBlockBase::IsInMyRange(const u64 addr)
{
	return addr >= MemoryBlock::GetStartAddr() && addr < MemoryBlock::GetStartAddr() + GetSize();
}

bool DynamicMemoryBlockBase::IsInMyRange(const u64 addr, const u32 size)
{
	return IsInMyRange(addr) && IsInMyRange(addr + size - 1);
}

bool DynamicMemoryBlockBase::IsMyAddress(const u64 addr)
{
	return IsInMyRange(addr);
}

MemoryBlock* DynamicMemoryBlockBase::SetRange(const u64 start, const u32 size)
{
	LV2_LOCK();

	m_max_size = PAGE_4K(size);
	if (!MemoryBlock::SetRange(start, 0))
	{
		assert(0);
		return nullptr;
	}

	return this;
}

void DynamicMemoryBlockBase::Delete()
{
	LV2_LOCK();

	m_allocated.clear();
	m_max_size = 0;

	MemoryBlock::Delete();
}

bool DynamicMemoryBlockBase::AllocFixed(u64 addr, u32 size)
{
	size = PAGE_4K(size + (addr & 4095)); // align size

	addr &= ~4095; // align start address

	if (!IsInMyRange(addr, size))
	{
		assert(0);
		return false;
	}

	LV2_LOCK();

	for (u32 i = 0; i<m_allocated.size(); ++i)
	{
		if (addr >= m_allocated[i].addr && addr < m_allocated[i].addr + m_allocated[i].size) return false;
	}

	AppendMem(addr, size);

	return true;
}

void DynamicMemoryBlockBase::AppendMem(u64 addr, u32 size) /* private */
{
	m_allocated.emplace_back(addr, size);
}

u64 DynamicMemoryBlockBase::AllocAlign(u32 size, u32 align)
{
	size = PAGE_4K(size);
	u32 exsize;

	if (align <= 4096)
	{
		align = 0;
		exsize = size;
	}
	else
	{
		align &= ~4095;
		exsize = size + align - 1;
	}

	LV2_LOCK();

	for (u64 addr = MemoryBlock::GetStartAddr(); addr <= MemoryBlock::GetEndAddr() - exsize;)
	{
		bool is_good_addr = true;

		for (u32 i = 0; i<m_allocated.size(); ++i)
		{
			if ((addr >= m_allocated[i].addr && addr < m_allocated[i].addr + m_allocated[i].size) ||
				(m_allocated[i].addr >= addr && m_allocated[i].addr < addr + exsize))
			{
				is_good_addr = false;
				addr = m_allocated[i].addr + m_allocated[i].size;
				break;
			}
		}

		if (!is_good_addr) continue;

		if (align)
		{
			addr = (addr + (align - 1)) & ~(align - 1);
		}

		//LOG_NOTICE(MEMORY, "AllocAlign(size=0x%x) -> 0x%llx", size, addr);

		AppendMem(addr, size);

		return addr;
	}

	return 0;
}

bool DynamicMemoryBlockBase::Alloc()
{
	return AllocAlign(GetSize() - GetUsedSize()) != 0;
}

bool DynamicMemoryBlockBase::Free(u64 addr)
{
	LV2_LOCK();

	for (u32 num = 0; num < m_allocated.size(); num++)
	{
		if (addr == m_allocated[num].addr)
		{
			//LOG_NOTICE(MEMORY, "Free(0x%llx)", addr);

			m_allocated.erase(m_allocated.begin() + num);
			return true;
		}
	}

	LOG_ERROR(MEMORY, "DynamicMemoryBlock::Free(addr=0x%llx): failed", addr);
	for (u32 i = 0; i < m_allocated.size(); i++)
	{
		LOG_NOTICE(MEMORY, "*** Memory Block: addr = 0x%llx, size = 0x%x", m_allocated[i].addr, m_allocated[i].size);
	}
	return false;
}

u8* DynamicMemoryBlockBase::GetMem(u64 addr) const
{
	return MemoryBlock::GetMem(addr);
}

bool DynamicMemoryBlockBase::IsLocked(u64 addr)
{
	LOG_ERROR(MEMORY, "DynamicMemoryBlockBase::IsLocked() not implemented");
	Emu.Pause();
	return false;
}

bool DynamicMemoryBlockBase::Lock(u64 addr, u32 size)
{
	LOG_ERROR(MEMORY, "DynamicMemoryBlockBase::Lock() not implemented");
	Emu.Pause();
	return false;
}

bool DynamicMemoryBlockBase::Unlock(u64 addr, u32 size)
{
	LOG_ERROR(MEMORY, "DynamicMemoryBlockBase::Unlock() not implemented");
	Emu.Pause();
	return false;
}

VirtualMemoryBlock::VirtualMemoryBlock() : MemoryBlock(), m_reserve_size(0)
{
}

MemoryBlock* VirtualMemoryBlock::SetRange(const u64 start, const u32 size)
{
	range_start = start;
	range_size = size;

	return this;
}

bool VirtualMemoryBlock::IsInMyRange(const u64 addr)
{
	return addr >= GetStartAddr() && addr < GetStartAddr() + GetSize() - GetReservedAmount();
}

bool VirtualMemoryBlock::IsInMyRange(const u64 addr, const u32 size)
{
	return IsInMyRange(addr) && IsInMyRange(addr + size - 1);
}

bool VirtualMemoryBlock::IsMyAddress(const u64 addr)
{
	for(u32 i=0; i<m_mapped_memory.size(); ++i)
	{
		if(addr >= m_mapped_memory[i].addr && addr < m_mapped_memory[i].addr + m_mapped_memory[i].size)
		{
			return true;
		}
	}

	return false;
}

u64 VirtualMemoryBlock::Map(u64 realaddr, u32 size, u64 addr)
{
	if(addr)
	{
		if(!IsInMyRange(addr, size) && (IsMyAddress(addr) || IsMyAddress(addr + size - 1)))
			return 0;

		m_mapped_memory.emplace_back(addr, realaddr, size);
		return addr;
	}
	else
	{
		for(u64 addr = GetStartAddr(); addr <= GetEndAddr() - GetReservedAmount() - size;)
		{
			bool is_good_addr = true;

			// check if address is already mapped
			for(u32 i=0; i<m_mapped_memory.size(); ++i)
			{
				if((addr >= m_mapped_memory[i].addr && addr < m_mapped_memory[i].addr + m_mapped_memory[i].size) ||
					(m_mapped_memory[i].addr >= addr && m_mapped_memory[i].addr < addr + size))
				{
					is_good_addr = false;
					addr = m_mapped_memory[i].addr + m_mapped_memory[i].size;
					break;
				}
			}

			if(!is_good_addr) continue;

			m_mapped_memory.emplace_back(addr, realaddr, size);

			return addr;
		}

		return 0;
	}
}

u32 VirtualMemoryBlock::UnmapRealAddress(u64 realaddr)
{
	for(u32 i=0; i<m_mapped_memory.size(); ++i)
	{
		if(m_mapped_memory[i].realAddress == realaddr && IsInMyRange(m_mapped_memory[i].addr, m_mapped_memory[i].size))
		{
			u32 size = m_mapped_memory[i].size;
			m_mapped_memory.erase(m_mapped_memory.begin() + i);
			return size;
		}
	}

	return 0;
}

u32 VirtualMemoryBlock::UnmapAddress(u64 addr)
{
	for(u32 i=0; i<m_mapped_memory.size(); ++i)
	{
		if(m_mapped_memory[i].addr == addr && IsInMyRange(m_mapped_memory[i].addr, m_mapped_memory[i].size))
		{
			u32 size = m_mapped_memory[i].size;
			m_mapped_memory.erase(m_mapped_memory.begin() + i);
			return size;
		}
	}

	return 0;
}

bool VirtualMemoryBlock::Read32(const u64 addr, u32* value)
{
	u64 realAddr;
	if (!getRealAddr(addr, realAddr))
		return false;
	*value = Memory.Read32(realAddr);
	return true;
}

bool VirtualMemoryBlock::Write32(const u64 addr, const u32 value)
{
	u64 realAddr;
	if(!getRealAddr(addr, realAddr))
		return false;
	Memory.Write32(realAddr, value);
	return true;
}

bool VirtualMemoryBlock::getRealAddr(u64 addr, u64& result)
{
	for(u32 i=0; i<m_mapped_memory.size(); ++i)
	{
		if(addr >= m_mapped_memory[i].addr && addr < m_mapped_memory[i].addr + m_mapped_memory[i].size)
		{
			result = m_mapped_memory[i].realAddress + (addr - m_mapped_memory[i].addr);
			return true;
		}
	}

	return false;
}

u64 VirtualMemoryBlock::getMappedAddress(u64 realAddress)
{
	for(u32 i=0; i<m_mapped_memory.size(); ++i)
	{
		if(realAddress >= m_mapped_memory[i].realAddress && realAddress < m_mapped_memory[i].realAddress + m_mapped_memory[i].size)
		{
			return m_mapped_memory[i].addr + (realAddress - m_mapped_memory[i].realAddress);
		}
	}

	return 0;
}

void VirtualMemoryBlock::Delete()
{
	m_mapped_memory.clear();

	MemoryBlock::Delete();
}

bool VirtualMemoryBlock::Reserve(u32 size)
{
	if(size + GetReservedAmount() > GetEndAddr() - GetStartAddr())
		return false;

	m_reserve_size += size;
	return true;
}

bool VirtualMemoryBlock::Unreserve(u32 size)
{
	if(size > GetReservedAmount())
		return false;

	m_reserve_size -= size;
	return true;
}

u32 VirtualMemoryBlock::GetReservedAmount()
{
	return m_reserve_size;
}
