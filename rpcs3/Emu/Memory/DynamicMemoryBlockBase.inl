
//DynamicMemoryBlockBase
template<typename PT>
DynamicMemoryBlockBase<PT>::DynamicMemoryBlockBase() : m_max_size(0)
{
}

template<typename PT>
const u32 DynamicMemoryBlockBase<PT>::GetUsedSize() const
{
	u32 size = 0;

	for(u32 i=0; i<m_used_mem.GetCount(); ++i)
	{
		size += m_used_mem[i].size;
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
	for(u32 i=0; i<m_used_mem.GetCount(); ++i)
	{
		if(addr >= m_used_mem[i].addr && addr < m_used_mem[i].addr + m_used_mem[i].size)
		{
			return true;
		}
	}

	return false;
}

template<typename PT>
MemoryBlock* DynamicMemoryBlockBase<PT>::SetRange(const u64 start, const u32 size)
{
	m_max_size = size;
	MemoryBlock::SetRange(start, 0);

	return this;
}

template<typename PT>
void DynamicMemoryBlockBase<PT>::Delete()
{
	m_used_mem.Clear();
	m_max_size = 0;

	MemoryBlock::Delete();
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::Alloc(u64 addr, u32 size)
{
	if(!IsInMyRange(addr, size))
	{
		assert(0);
		return false;
	}

	if(IsMyAddress(addr) || IsMyAddress(addr + size - 1))
	{
		return false;
	}

	for(u32 i=0; i<m_used_mem.GetCount(); ++i)
	{
		if(addr >= m_used_mem[i].addr && addr < m_used_mem[i].addr + m_used_mem[i].size) return false;
	}

	AppendUsedMem(addr, size);

	return true;
}

template<typename PT>
void DynamicMemoryBlockBase<PT>::AppendUsedMem(u64 addr, u32 size)
{
	m_used_mem.Move(new MemBlockInfo(addr, size));
}

template<typename PT>
u64 DynamicMemoryBlockBase<PT>::Alloc(u32 size)
{
	for(u64 addr = MemoryBlock::GetStartAddr(); addr <= MemoryBlock::GetEndAddr() - size;)
	{
		bool is_good_addr = true;

		for(u32 i=0; i<m_used_mem.GetCount(); ++i)
		{
			if((addr >= m_used_mem[i].addr && addr < m_used_mem[i].addr + m_used_mem[i].size) ||
				(m_used_mem[i].addr >= addr && m_used_mem[i].addr < addr + size))
			{
				is_good_addr = false;
				addr = m_used_mem[i].addr + m_used_mem[i].size;
				break;
			}
		}

		if(!is_good_addr) continue;

		AppendUsedMem(addr, size);

		return addr;
	}

	return 0;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::Alloc()
{
	return Alloc(GetSize() - GetUsedSize()) != 0;
}

template<typename PT>
bool DynamicMemoryBlockBase<PT>::Free(u64 addr)
{	
	for(u32 i=0; i<m_used_mem.GetCount(); ++i)
	{
		if(addr == m_used_mem[i].addr)
		{
			m_used_mem.RemoveAt(i);
			return true;
		}
	}

	return false;
}

template<typename PT>
u8* DynamicMemoryBlockBase<PT>::GetMem(u64 addr) const
{
	for(u32 i=0; i<m_used_mem.GetCount(); ++i)
	{
		u64 _addr = MemoryBlock::FixAddr(m_used_mem[i].addr);

		if(addr >= _addr && addr < _addr + m_used_mem[i].size)
		{
			return (u8*)m_used_mem[i].mem + addr - _addr;
		}
	}

	ConLog.Error("GetMem(%llx) from not allocated address.", addr);
	assert(0);
	return nullptr;
}
