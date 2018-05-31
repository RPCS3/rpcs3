#include "stdafx.h"
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
	return addr >= m_range_start && addr + size <= m_range_start + m_range_size - GetReservedAmount();
}

bool VirtualMemoryBlock::Map(u32 realaddr, u32 size, u32 addr)
{
	if (!size || !IsInMyRange(addr, size))
	{
		return false;
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

s32 VirtualMemoryBlock::getMappedAddress(u32 realAddress)
{
	for (u32 i = 0; i<m_mapped_memory.size(); ++i)
	{
		if (realAddress >= m_mapped_memory[i].realAddress && realAddress < m_mapped_memory[i].realAddress + m_mapped_memory[i].size)
		{
			return m_mapped_memory[i].addr + (realAddress - m_mapped_memory[i].realAddress);
		}
	}

	return -1;
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

u32 VirtualMemoryBlock::GetRangeStart()
{
	return m_range_start;
}

u32 VirtualMemoryBlock::GetRangeEnd()
{
	return m_range_start + m_range_size - GetReservedAmount();
}
