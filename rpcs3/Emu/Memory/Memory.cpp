#include "stdafx.h"
#include "Utilities/Log.h"
#include "Memory.h"

MemoryBase Memory;

std::mutex g_memory_mutex;

void MemoryBase::Init(MemoryType type)
{
	if (m_inited) return;
	m_inited = true;

	LOG_NOTICE(MEMORY, "Initializing memory: g_base_addr = 0x%llx, g_priv_addr = 0x%llx", (u64)vm::g_base_addr, (u64)vm::g_priv_addr);

#ifdef _WIN32
	if (!vm::g_base_addr || !vm::g_priv_addr)
#else
	if ((s64)vm::g_base_addr == (s64)-1 || (s64)vm::g_priv_addr == (s64)-1)
#endif
	{
		LOG_ERROR(MEMORY, "Initializing memory failed");
		return;
	}

	switch (type)
	{
	case Memory_PS3:
		MemoryBlocks.push_back(MainMem.SetRange(0x00010000, 0x1FFF0000));
		MemoryBlocks.push_back(UserMemory = Userspace.SetRange(0x20000000, 0x10000000));
		MemoryBlocks.push_back(RSXFBMem.SetRange(0xC0000000, 0x10000000));
		MemoryBlocks.push_back(StackMem.SetRange(0xD0000000, 0x10000000));
		break;

	case Memory_PSV:
		MemoryBlocks.push_back(PSV.RAM.SetRange(0x81000000, 0x10000000));
		MemoryBlocks.push_back(UserMemory = PSV.Userspace.SetRange(0x91000000, 0x2F000000));
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

bool MemoryBase::Map(const u32 addr, const u32 size)
{
	assert(size && (size | addr) % 4096 == 0);

	std::lock_guard<std::mutex> lock(g_memory_mutex);

	for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
	{
		if (vm::check_addr(i * 4096, 4096))
		{
			return false;
		}
	}

	MemoryBlocks.push_back((new MemoryBlock())->SetRange(addr, size));

	LOG_WARNING(MEMORY, "Memory mapped at 0x%x: size=0x%x", addr, size);
	return true;
}

bool MemoryBase::Unmap(const u32 addr)
{
	std::lock_guard<std::mutex> lock(g_memory_mutex);

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

MemBlockInfo::MemBlockInfo(u32 addr, u32 size)
	: MemInfo(addr, size)
{
	vm::page_map(addr, size, vm::page_readable | vm::page_writable | vm::page_executable);
}

void MemBlockInfo::Free()
{
	if (addr && size)
	{
		vm::page_unmap(addr, size);
		addr = size = 0;
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
}

void MemoryBlock::InitMemory()
{
	if (range_size)
	{
		Free();
		mem_inf = new MemBlockInfo(range_start, range_size);
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

MemoryBlock* MemoryBlock::SetRange(const u32 start, const u32 size)
{
	range_start = start;
	range_size = size;

	InitMemory();
	return this;
}

DynamicMemoryBlockBase::DynamicMemoryBlockBase()
	: MemoryBlock()
	, m_max_size(0)
{
}

const u32 DynamicMemoryBlockBase::GetUsedSize() const
{
	std::lock_guard<std::mutex> lock(g_memory_mutex);

	u32 size = 0;

	for (u32 i = 0; i<m_allocated.size(); ++i)
	{
		size += m_allocated[i].size;
	}

	return size;
}

bool DynamicMemoryBlockBase::IsInMyRange(const u32 addr, const u32 size)
{
	return addr >= MemoryBlock::GetStartAddr() && addr + size - 1 <= MemoryBlock::GetEndAddr();
}

MemoryBlock* DynamicMemoryBlockBase::SetRange(const u32 start, const u32 size)
{
	std::lock_guard<std::mutex> lock(g_memory_mutex);

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
	std::lock_guard<std::mutex> lock(g_memory_mutex);

	m_allocated.clear();
	m_max_size = 0;

	MemoryBlock::Delete();
}

bool DynamicMemoryBlockBase::AllocFixed(u32 addr, u32 size)
{
	assert(size);

	size = PAGE_4K(size + (addr & 4095)); // align size

	addr &= ~4095; // align start address

	if (!IsInMyRange(addr, size))
	{
		assert(0);
		return false;
	}

	std::lock_guard<std::mutex> lock(g_memory_mutex);

	for (u32 i = 0; i<m_allocated.size(); ++i)
	{
		if (addr >= m_allocated[i].addr && addr <= m_allocated[i].addr + m_allocated[i].size - 1) return false;
	}

	AppendMem(addr, size);

	return true;
}

void DynamicMemoryBlockBase::AppendMem(u32 addr, u32 size) /* private */
{
	m_allocated.emplace_back(addr, size);
}

u32 DynamicMemoryBlockBase::AllocAlign(u32 size, u32 align)
{
	assert(size && align);

	if (!MemoryBlock::GetStartAddr())
	{
		LOG_ERROR(MEMORY, "DynamicMemoryBlockBase::AllocAlign(size=0x%x, align=0x%x): memory block not initialized", size, align);
		return 0;
	}

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

	std::lock_guard<std::mutex> lock(g_memory_mutex);

	for (u32 addr = MemoryBlock::GetStartAddr(); addr <= MemoryBlock::GetEndAddr() - exsize;)
	{
		bool is_good_addr = true;

		for (u32 i = 0; i<m_allocated.size(); ++i)
		{
			if ((addr >= m_allocated[i].addr && addr <= m_allocated[i].addr + m_allocated[i].size - 1) ||
				(m_allocated[i].addr >= addr && m_allocated[i].addr <= addr + exsize - 1))
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

		//LOG_NOTICE(MEMORY, "AllocAlign(size=0x%x) -> 0x%x", size, addr);

		AppendMem(addr, size);

		return addr;
	}

	return 0;
}

bool DynamicMemoryBlockBase::Alloc()
{
	return AllocAlign(GetSize() - GetUsedSize()) != 0;
}

bool DynamicMemoryBlockBase::Free(u32 addr)
{
	std::lock_guard<std::mutex> lock(g_memory_mutex);

	for (u32 num = 0; num < m_allocated.size(); num++)
	{
		if (addr == m_allocated[num].addr)
		{
			//LOG_NOTICE(MEMORY, "Free(0x%x)", addr);

			m_allocated.erase(m_allocated.begin() + num);
			return true;
		}
	}

	LOG_ERROR(MEMORY, "DynamicMemoryBlock::Free(addr=0x%x): failed", addr);
	for (u32 i = 0; i < m_allocated.size(); i++)
	{
		LOG_NOTICE(MEMORY, "*** Memory Block: addr = 0x%x, size = 0x%x", m_allocated[i].addr, m_allocated[i].size);
	}
	return false;
}

VirtualMemoryBlock::VirtualMemoryBlock() : MemoryBlock(), m_reserve_size(0)
{
}

MemoryBlock* VirtualMemoryBlock::SetRange(const u32 start, const u32 size)
{
	range_start = start;
	range_size = size;

	return this;
}

bool VirtualMemoryBlock::IsInMyRange(const u32 addr, const u32 size)
{
	return addr >= GetStartAddr() && addr + size - 1 <= GetEndAddr() - GetReservedAmount();
}

u32 VirtualMemoryBlock::Map(u32 realaddr, u32 size)
{
	assert(size);

	for (u32 addr = GetStartAddr(); addr <= GetEndAddr() - GetReservedAmount() - size;)
	{
		bool is_good_addr = true;

		// check if address is already mapped
		for (u32 i = 0; i<m_mapped_memory.size(); ++i)
		{
			if ((addr >= m_mapped_memory[i].addr && addr < m_mapped_memory[i].addr + m_mapped_memory[i].size) ||
				(m_mapped_memory[i].addr >= addr && m_mapped_memory[i].addr < addr + size))
			{
				is_good_addr = false;
				addr = m_mapped_memory[i].addr + m_mapped_memory[i].size;
				break;
			}
		}

		if (!is_good_addr) continue;

		m_mapped_memory.emplace_back(addr, realaddr, size);

		return addr;
	}

	return 0;
}

bool VirtualMemoryBlock::Map(u32 realaddr, u32 size, u32 addr)
{
	assert(size);

	if (!IsInMyRange(addr, size))
	{
		return false;
	}

	for (u32 i = 0; i<m_mapped_memory.size(); ++i)
	{
		if (addr >= m_mapped_memory[i].addr && addr + size - 1 <= m_mapped_memory[i].addr + m_mapped_memory[i].size - 1)
		{
			return false;
		}
	}

	m_mapped_memory.emplace_back(addr, realaddr, size);
	return true;
}

bool VirtualMemoryBlock::UnmapRealAddress(u32 realaddr, u32& size)
{
	for (u32 i = 0; i<m_mapped_memory.size(); ++i)
	{
		if (m_mapped_memory[i].realAddress == realaddr && IsInMyRange(m_mapped_memory[i].addr, m_mapped_memory[i].size))
		{
			size = m_mapped_memory[i].size;
			m_mapped_memory.erase(m_mapped_memory.begin() + i);
			return true;
		}
	}

	return false;
}

bool VirtualMemoryBlock::UnmapAddress(u32 addr, u32& size)
{
	for (u32 i = 0; i<m_mapped_memory.size(); ++i)
	{
		if (m_mapped_memory[i].addr == addr && IsInMyRange(m_mapped_memory[i].addr, m_mapped_memory[i].size))
		{
			size = m_mapped_memory[i].size;
			m_mapped_memory.erase(m_mapped_memory.begin() + i);
			return true;
		}
	}

	return false;
}

bool VirtualMemoryBlock::Read32(const u32 addr, u32* value)
{
	u32 realAddr;
	if (!getRealAddr(addr, realAddr))
		return false;
	*value = vm::read32(realAddr);
	return true;
}

bool VirtualMemoryBlock::Write32(const u32 addr, const u32 value)
{
	u32 realAddr;
	if (!getRealAddr(addr, realAddr))
		return false;
	vm::write32(realAddr, value);
	return true;
}

bool VirtualMemoryBlock::getRealAddr(u32 addr, u32& result)
{
	for (u32 i = 0; i<m_mapped_memory.size(); ++i)
	{
		if (addr >= m_mapped_memory[i].addr && addr < m_mapped_memory[i].addr + m_mapped_memory[i].size)
		{
			result = m_mapped_memory[i].realAddress + (addr - m_mapped_memory[i].addr);
			return true;
		}
	}

	return false;
}

u32 VirtualMemoryBlock::getMappedAddress(u32 realAddress)
{
	for (u32 i = 0; i<m_mapped_memory.size(); ++i)
	{
		if (realAddress >= m_mapped_memory[i].realAddress && realAddress < m_mapped_memory[i].realAddress + m_mapped_memory[i].size)
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
	if (size + GetReservedAmount() > GetEndAddr() - GetStartAddr())
		return false;

	m_reserve_size += size;
	return true;
}

bool VirtualMemoryBlock::Unreserve(u32 size)
{
	if (size > GetReservedAmount())
		return false;

	m_reserve_size -= size;
	return true;
}

u32 VirtualMemoryBlock::GetReservedAmount()
{
	return m_reserve_size;
}
