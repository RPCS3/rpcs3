
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

	const u32 index = MemoryBlock::FixAddr(addr) >> 12;

	return m_pages[index] && !m_locked[index];
}

template<typename PT>
MemoryBlock* DynamicMemoryBlockBase<PT>::SetRange(const u64 start, const u32 size)
{
	std::lock_guard<std::mutex> lock(m_lock);

	m_max_size = PAGE_4K(size);
	MemoryBlock::SetRange(start, 0);

	const u32 page_count = m_max_size >> 12;
	m_pages.resize(page_count);
	m_locked.resize(page_count);
	memset(m_pages.data(), 0, sizeof(u8*) * page_count);
	memset(m_locked.data(), 0, sizeof(u8*) * page_count);

	return this;
}

template<typename PT>
void DynamicMemoryBlockBase<PT>::Delete()
{
	std::lock_guard<std::mutex> lock(m_lock);

	m_allocated.clear();
	m_max_size = 0;

	m_pages.clear();
	m_locked.clear();

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
	//u8* pointer = (u8*)m_allocated[m_allocated.Move(new MemBlockInfo(addr, size))].mem;
	m_allocated.emplace_back(addr, size);
	u8* pointer = (u8*) m_allocated.back().mem;

	const u32 first = MemoryBlock::FixAddr(addr) >> 12;

	const u32 last = first + ((size - 1) >> 12);

	for (u32 i = first; i <= last; i++)
	{
		m_pages[i] = pointer;
		m_locked[i] = nullptr;
		pointer += 4096;
	}
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
			/* if(IsLocked(m_allocated[num].addr)) return false; */

			const u32 first = MemoryBlock::FixAddr(addr) >> 12;

			const u32 last = first + ((m_allocated[num].size - 1) >> 12);

			// check if locked:
			for (u32 i = first; i <= last; i++)
			{
				if (!m_pages[i] || m_locked[i]) return false;
			}

			// clear pointers:
			for (u32 i = first; i <= last; i++)
			{
				m_pages[i] = nullptr;
				m_locked[i] = nullptr;
			}

			m_allocated.erase(m_allocated.begin() + num);
			return true;
		}
	}

	ConLog.Error("DynamicMemoryBlock::Free(addr=0x%llx): failed", addr);
	for (u32 i = 0; i < m_allocated.size(); i++)
	{
		ConLog.Write("*** Memory Block: addr = 0x%llx, size = 0x%x", m_allocated[i].addr, m_allocated[i].size);
	}
	return false;
}

template<typename PT>
u8* DynamicMemoryBlockBase<PT>::GetMem(u64 addr) const // lock-free, addr is fixed
{
	const u32 index = addr >> 12;

	if (index < m_pages.size())
	{
		if (u8* res = m_pages[index])
		{
			return res + (addr & 4095);
		}
	}

	ConLog.Error("GetMem(%llx) from not allocated address.", addr);
	assert(0);
	return nullptr;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::IsLocked(u64 addr) // lock-free
{
	if (IsInMyRange(addr))
	{
		const u32 index = MemoryBlock::FixAddr(addr) >> 12;

		if (index < m_locked.size())
		{
			if (m_locked[index]) return true;
		}
	}

	return false;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::Lock(u64 addr, u32 size)
{
	size = PAGE_4K(size); // align size

	addr &= ~4095; // align start address

	if (!IsInMyRange(addr, size))
	{
		assert(0);
		return false;
	}

	if (IsMyAddress(addr) || IsMyAddress(addr + size - 1))
	{
		return false;
	}

	const u32 first = MemoryBlock::FixAddr(addr) >> 12;

	const u32 last = first + ((size - 1) >> 12);

	for (u32 i = first; i <= last; i++)
	{
		if (u8* pointer = m_pages[i])
		{
			m_locked[i] = pointer;
			m_pages[i] = nullptr;
		}
		else // already locked or empty
		{
		}
	}

	return true;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::Unlock(u64 addr, u32 size)
{
	size = PAGE_4K(size); // align size

	addr &= ~4095; // align start address

	if (!IsInMyRange(addr, size))
	{
		assert(0);
		return false;
	}

	if (IsMyAddress(addr) || IsMyAddress(addr + size - 1))
	{
		return false;
	}

	const u32 first = MemoryBlock::FixAddr(addr) >> 12;

	const u32 last = first + ((size - 1) >> 12);

	for (u32 i = first; i <= last; i++)
	{
		if (u8* pointer = m_locked[i])
		{
			m_pages[i] = pointer;
			m_locked[i] = nullptr;
		}
		else // already unlocked or empty
		{
		}
	}

	return true;
}
