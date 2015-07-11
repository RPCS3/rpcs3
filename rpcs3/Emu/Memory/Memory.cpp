#include "stdafx.h"
#include "Utilities/Log.h"
#include "Memory.h"

VirtualMemoryBlock RSXIOMem;

VirtualMemoryBlock* VirtualMemoryBlock::SetRange(const u32 start, const u32 size)
{
	m_range_start = start;
	m_range_size = size;

	return this;
}

bool VirtualMemoryBlock::IsInMyRange(const u32 addr, const u32 size)
{
	return addr >= m_range_start && addr + size - 1 <= m_range_start + m_range_size - 1 - GetReservedAmount();
}

u32 VirtualMemoryBlock::Map(u32 realaddr, u32 size)
{
	assert(size);

	for (u32 addr = m_range_start; addr <= m_range_start + m_range_size - 1 - GetReservedAmount() - size;)
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
	*value = vm::ps3::read32(realAddr);
	return true;
}

bool VirtualMemoryBlock::Write32(const u32 addr, const u32 value)
{
	u32 realAddr;
	if (!getRealAddr(addr, realAddr))
		return false;
	vm::ps3::write32(realAddr, value);
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

bool VirtualMemoryBlock::Reserve(u32 size)
{
	if (size + GetReservedAmount() > m_range_size)
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
