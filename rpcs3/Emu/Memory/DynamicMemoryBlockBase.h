#pragma once
//DynamicMemoryBlockBase
template<typename PT>
DynamicMemoryBlockBase<PT>::DynamicMemoryBlockBase()
	: PT()
	, m_max_size(0)
{
}

template<typename PT>
const u32 DynamicMemoryBlockBase<PT>::GetUsedSize() const
{
	std::lock_guard<std::mutex> lock(m_lock);

	u32 size = 0;

	for (u32 i = 0; i<m_allocated.size(); ++i)
	{
		size += m_allocated[i].size;
	}

	return size;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::IsInMyRange(const u64 addr)
{
	return addr >= MemoryBlock::GetStartAddr() && addr < MemoryBlock::GetStartAddr() + GetSize();
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::IsInMyRange(const u64 addr, const u32 size)
{
	return IsInMyRange(addr) && IsInMyRange(addr + size - 1);
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::IsMyAddress(const u64 addr)
{
	if (!IsInMyRange(addr)) return false;

	return Memory.IsGoodAddr(addr);
}

template<typename PT>
MemoryBlock* DynamicMemoryBlockBase<PT>::SetRange(const u64 start, const u32 size)
{
	std::lock_guard<std::mutex> lock(m_lock);

	m_max_size = PAGE_4K(size);
	if (!MemoryBlock::SetRange(start, 0))
	{
		assert(0);
		return nullptr;
	}

	return this;
}

template<typename PT>
void DynamicMemoryBlockBase<PT>::Delete()
{
	std::lock_guard<std::mutex> lock(m_lock);

	m_allocated.clear();
	m_max_size = 0;

	MemoryBlock::Delete();
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::AllocFixed(u64 addr, u32 size)
{
	size = PAGE_4K(size + (addr & 4095)); // align size

	addr &= ~4095; // align start address

	if (!IsInMyRange(addr, size))
	{
		assert(0);
		return false;
	}

	if (IsMyAddress(addr) || IsMyAddress(addr + size - 1))
	{
		assert(0);
		return false;
	}

	std::lock_guard<std::mutex> lock(m_lock);

	for (u32 i = 0; i<m_allocated.size(); ++i)
	{
		if (addr >= m_allocated[i].addr && addr < m_allocated[i].addr + m_allocated[i].size) return false;
	}

	AppendMem(addr, size);

	return true;
}

template<typename PT>
void DynamicMemoryBlockBase<PT>::AppendMem(u64 addr, u32 size) /* private */
{
	m_allocated.emplace_back(addr, size);
}

template<typename PT>
u64 DynamicMemoryBlockBase<PT>::AllocAlign(u32 size, u32 align)
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

	std::lock_guard<std::mutex> lock(m_lock);

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

template<typename PT>
bool DynamicMemoryBlockBase<PT>::Alloc()
{
	return AllocAlign(GetSize() - GetUsedSize()) != 0;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::Free(u64 addr)
{
	std::lock_guard<std::mutex> lock(m_lock);

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
	assert(0);
	return false;
}

template<typename PT>
u8* DynamicMemoryBlockBase<PT>::GetMem(u64 addr) const
{
	if (addr < GetSize() && Memory.IsGoodAddr(addr + GetStartAddr())) return mem + addr;

	LOG_ERROR(MEMORY, "GetMem(0x%llx) from not allocated address.", addr);
	assert(0);
	return nullptr;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::IsLocked(u64 addr)
{
	// TODO
	LOG_ERROR(MEMORY, "IsLocked(0x%llx) not implemented", addr);
	assert(0);
	return false;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::Lock(u64 addr, u32 size)
{
	// TODO
	LOG_ERROR(MEMORY, "Lock(0x%llx, 0x%x) not implemented", addr, size);
	assert(0);
	return false;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::Unlock(u64 addr, u32 size)
{
	// TODO
	LOG_ERROR(MEMORY, "Unlock(0x%llx, 0x%x) not implemented", addr, size);
	assert(0);
	return false;
}
