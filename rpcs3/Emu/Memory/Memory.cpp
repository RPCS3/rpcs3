#include "stdafx.h"
#include "Memory.h"
#include "MemoryBlock.h"

MemoryBase Memory;

//MemoryBlock
MemoryBlock::MemoryBlock()
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

	mem = NULL;
}

void MemoryBlock::InitMemory()
{
	safe_delete(mem);
	mem = new u8[range_size];
	memset(mem, 0, range_size);
}

void MemoryBlock::Delete()
{
	safe_delete(mem);
	Init();
}

u64 MemoryBlock::FixAddr(const u64 addr) const
{
	return addr - GetStartAddr();
}

bool MemoryBlock::GetMemFromAddr(void* dst, const u64 addr, const u32 size)
{
	if(!IsMyAddress(addr)) return false;
	if(FixAddr(addr) + size > GetSize()) return false;
	memcpy(dst, &mem[FixAddr(addr)], size);
	return true;
}

bool MemoryBlock::SetMemFromAddr(void* src, const u64 addr, const u32 size)
{
	if(!IsMyAddress(addr)) return false;
	if(FixAddr(addr) + size > GetSize()) return false;
	memcpy(&mem[FixAddr(addr)], src, size);
	return true;
}

bool MemoryBlock::GetMemFFromAddr(void* dst, const u64 addr)
{
	if(!IsMyAddress(addr)) return false;
	dst = &mem[FixAddr(addr)];
	return true;
}

u8* MemoryBlock::GetMemFromAddr(const u64 addr)
{
	if(!IsMyAddress(addr)) return NULL;

	return &mem[FixAddr(addr)];
}

MemoryBlock* MemoryBlock::SetRange(const u64 start, const u32 size)
{
	range_start = start;
	range_size = size;

	InitMemory();
	return this;
}

bool MemoryBlock::SetNewSize(const u32 size)
{
	if(range_size >= size) return false;

	u8* new_mem = (u8*)realloc(mem, size);
	if(!new_mem)
	{
		ConLog.Error("Not enought free memory");
		Emu.Pause();
		return false;
	}

	mem = new_mem;
	range_size = size;

	return true;
}

bool MemoryBlock::IsMyAddress(const u64 addr)
{
	return addr >= GetStartAddr() && addr < GetEndAddr();
}

__forceinline const u8 MemoryBlock::FastRead8(const u64 addr) const
{
	return mem[addr];
}

__forceinline const u16 MemoryBlock::FastRead16(const u64 addr) const
{
	return ((u16)FastRead8(addr) << 8) | (u16)FastRead8(addr + 1);
}

__forceinline const u32 MemoryBlock::FastRead32(const u64 addr) const
{
	return ((u32)FastRead16(addr) << 16) | (u32)FastRead16(addr + 2);
}

__forceinline const u64 MemoryBlock::FastRead64(const u64 addr) const
{
	return ((u64)FastRead32(addr) << 32) | (u64)FastRead32(addr + 4);
}

__forceinline const u128 MemoryBlock::FastRead128(const u64 addr)
{
	u128 ret;
	ret.hi = FastRead64(addr);
	ret.lo = FastRead64(addr + 8);
	return ret;
}

bool MemoryBlock::Read8(const u64 addr, u8* value)
{
	if(!IsMyAddress(addr))
	{
		*value = 0;
		return false;
	}

	*value = FastRead8(FixAddr(addr));
	return true;
}

bool MemoryBlock::Read16(const u64 addr, u16* value)
{
	if(!IsMyAddress(addr))
	{
		*value = 0;
		return false;
	}

	*value = FastRead16(FixAddr(addr));
	return true;
}

bool MemoryBlock::Read32(const u64 addr, u32* value)
{
	if(!IsMyAddress(addr))
	{
		*value = 0;
		return false;
	}

	*value = FastRead32(FixAddr(addr));
	return true;
}

bool MemoryBlock::Read64(const u64 addr, u64* value)
{
	if(!IsMyAddress(addr))
	{
		*value = 0;
		return false;
	}

	*value = FastRead64(FixAddr(addr));
	return true;
}

bool MemoryBlock::Read128(const u64 addr, u128* value)
{
	if(!IsMyAddress(addr))
	{
		*value = u128::From32(0);
		return false;
	}

	*value = FastRead128(FixAddr(addr));
	return true;
}

__forceinline void MemoryBlock::FastWrite8(const u64 addr, const u8 value)
{
	mem[addr] = value;
}

__forceinline void MemoryBlock::FastWrite16(const u64 addr, const u16 value)
{
	FastWrite8(addr, (u8)(value >> 8));
	FastWrite8(addr+1, (u8)value);
}

__forceinline void MemoryBlock::FastWrite32(const u64 addr, const u32 value)
{
	FastWrite16(addr, (u16)(value >> 16));
	FastWrite16(addr+2, (u16)value);
}

__forceinline void MemoryBlock::FastWrite64(const u64 addr, const u64 value)
{
	FastWrite32(addr, (u32)(value >> 32));
	FastWrite32(addr+4, (u32)value);
}

__forceinline void MemoryBlock::FastWrite128(const u64 addr, const u128 value)
{
	FastWrite64(addr, value.hi);
	FastWrite64(addr+8, value.lo);
}

bool MemoryBlock::Write8(const u64 addr, const u8 value)
{
	if(!IsMyAddress(addr)) return false;

	FastWrite8(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write16(const u64 addr, const u16 value)
{
	if(!IsMyAddress(addr)) return false;

	FastWrite16(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write32(const u64 addr, const u32 value)
{
	if(!IsMyAddress(addr)) return false;

	FastWrite32(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write64(const u64 addr, const u64 value)
{
	if(!IsMyAddress(addr)) return false;

	FastWrite64(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write128(const u64 addr, const u128 value)
{
	if(!IsMyAddress(addr)) return false;

	FastWrite128(FixAddr(addr), value);
	return true;
}

//NullMemoryBlock
bool NullMemoryBlock::Read8(const u64 addr, u8* WXUNUSED(value))
{
	ConLog.Error("Read8 from null block: [%08llx]", addr);
	Emu.Pause();
	return false;
}

bool NullMemoryBlock::Read16(const u64 addr, u16* WXUNUSED(value))
{
	ConLog.Error("Read16 from null block: [%08llx]", addr);
	Emu.Pause();
	return false;
}

bool NullMemoryBlock::Read32(const u64 addr, u32* WXUNUSED(value))
{
	ConLog.Error("Read32 from null block: [%08llx]", addr);
	Emu.Pause();
	return false;
}

bool NullMemoryBlock::Read64(const u64 addr, u64* WXUNUSED(value))
{
	ConLog.Error("Read64 from null block: [%08llx]", addr);
	Emu.Pause();
	return false;
}

bool NullMemoryBlock::Read128(const u64 addr, u128* WXUNUSED(value))
{
	ConLog.Error("Read128 from null block: [%08llx]", addr);
	Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write8(const u64 addr, const u8 value)
{
	ConLog.Error("Write8 to null block: [%08llx]: %x", addr, value);
	Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write16(const u64 addr, const u16 value)
{
	ConLog.Error("Write16 to null block: [%08llx]: %x", addr, value);
	Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write32(const u64 addr, const u32 value)
{
	ConLog.Error("Write32 to null block: [%08llx]: %x", addr, value);
	Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write64(const u64 addr, const u64 value)
{
	ConLog.Error("Write64 to null block: [%08llx]: %llx", addr, value);
	Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write128(const u64 addr, const u128 value)
{
	ConLog.Error("Write128 to null block: [%08llx]: %llx_%llx", addr, value.hi, value.lo);
	Emu.Pause();
	return false;
}

//DynamicMemoryBlock
DynamicMemoryBlock::DynamicMemoryBlock() : m_point(0)
	, m_max_size(0)
{
}

bool DynamicMemoryBlock::IsInMyRange(const u64 addr)
{
	return addr >= GetStartAddr() && addr < GetStartAddr() + GetSize();
}

bool DynamicMemoryBlock::IsInMyRange(const u64 addr, const u32 size)
{
	return IsInMyRange(addr) && IsInMyRange(addr + size - 1);
}

bool DynamicMemoryBlock::IsMyAddress(const u64 addr)
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

MemoryBlock* DynamicMemoryBlock::SetRange(const u64 start, const u32 size)
{
	m_max_size = size;
	MemoryBlock::SetRange(start, 4);
	m_point = GetStartAddr();

	return this;
}

void DynamicMemoryBlock::Delete()
{
	m_used_mem.Clear();
	m_free_mem.Clear();
	m_point = 0;
	m_max_size = 0;

	MemoryBlock::Delete();
}

void DynamicMemoryBlock::UpdateSize(u64 addr, u32 size)
{
	u32 used_size = addr + size - GetStartAddr();
	if(used_size > GetUsedSize()) SetNewSize(used_size);
}

void DynamicMemoryBlock::CombineFreeMem()
{
	if(m_free_mem.GetCount() < 2) return;

	for(u32 i1=0; i1<m_free_mem.GetCount(); ++i1)
	{
		MemBlockInfo& u1 = m_free_mem[i1];
		for(u32 i2=i1+1; i2<m_free_mem.GetCount(); ++i2)
		{
			const MemBlockInfo u2 = m_free_mem[i2];
			if(u1.addr + u1.size != u2.addr) continue;
			u1.size += u2.size;
			m_free_mem.RemoveAt(i2);
			break;
		}
	}
}

bool DynamicMemoryBlock::Alloc(u64 addr, u32 size)
{
	if(!IsInMyRange(addr, size) || IsMyAddress(addr) || IsMyAddress(addr + size - 1))
	{
		assert(0);
		return false;
	}

	if(addr == m_point)
	{
		UpdateSize(m_point, size);

		m_used_mem.AddCpy(MemBlockInfo(m_point, size));
		memset(mem + (m_point - GetStartAddr()), 0, size);

		m_point += size;

		return true;
	}

	if(addr > m_point)
	{
		u64 free_mem_addr = GetStartAddr();

		if(free_mem_addr != addr)
		{
			for(u32 i=0; i<m_free_mem.GetCount(); ++i)
			{
				if(m_free_mem[i].addr >= free_mem_addr && m_free_mem[i].addr < addr)
				{
					free_mem_addr = m_free_mem[i].addr + m_free_mem[i].size;
				}
			}

			for(u32 i=0; i<m_used_mem.GetCount(); ++i)
			{
				if(m_used_mem[i].addr >= free_mem_addr && m_used_mem[i].addr < addr)
				{
					free_mem_addr = m_used_mem[i].addr + m_used_mem[i].size;
				}
			}

			m_free_mem.AddCpy(MemBlockInfo(free_mem_addr, addr - GetStartAddr()));
		}

		UpdateSize(addr, size);

		m_used_mem.AddCpy(MemBlockInfo(addr, size));
		memset(mem + (addr - GetStartAddr()), 0, size);

		m_point = addr + size;

		return true;
	}

	for(u32 i=0; i<m_free_mem.GetCount(); ++i)
	{
		if(addr < m_free_mem[i].addr || addr >= m_free_mem[i].addr + m_free_mem[i].size
			|| m_free_mem[i].size < size) continue;

		if(m_free_mem[i].addr != addr)
		{
			m_free_mem.AddCpy(MemBlockInfo(m_free_mem[i].addr, addr - m_free_mem[i].addr));
		}

		if(m_free_mem[i].size != size)
		{
			m_free_mem.AddCpy(MemBlockInfo(m_free_mem[i].addr + size, m_free_mem[i].size - size));
		}

		m_free_mem.RemoveAt(i);
		m_used_mem.AddCpy(MemBlockInfo(addr, size));

		memset(mem + (addr - GetStartAddr()), 0, size);
		return true;
	}

	return false;
}

u64 DynamicMemoryBlock::Alloc(u32 size)
{
	for(u32 i=0; i<m_free_mem.GetCount(); ++i)
	{
		if(m_free_mem[i].size < size) continue;

		const u32 addr = m_free_mem[i].addr;

		if(m_free_mem[i].size != size)
		{
			m_free_mem.AddCpy(MemBlockInfo(addr + size, m_free_mem[i].size - size));
		}

		m_free_mem.RemoveAt(i);
		m_used_mem.AddCpy(MemBlockInfo(addr, size));

		memset(mem + (addr - GetStartAddr()), 0, size);
		return addr;
	}

	UpdateSize(m_point, size);

	MemBlockInfo res(m_point, size);
	m_used_mem.AddCpy(res);
	memset(mem + (m_point - GetStartAddr()), 0, size);

	m_point += size;

	return res.addr;
}

bool DynamicMemoryBlock::Alloc()
{
	return Alloc(GetSize() - GetUsedSize()) != 0;
}

bool DynamicMemoryBlock::Free(u64 addr)
{
	for(u32 i=0; i<m_used_mem.GetCount(); ++i)
	{
		if(addr == m_used_mem[i].addr)
		{
			m_free_mem.AddCpy(MemBlockInfo(m_used_mem[i].addr, m_used_mem[i].size));
			m_used_mem.RemoveAt(i);
			CombineFreeMem();
			return true;
		}
	}

	return false;
}

//MemoryBase
void MemoryBase::Write8(u64 addr, const u8 data)
{
	GetMemByAddr(addr).Write8(addr, data);
}

void MemoryBase::Write16(u64 addr, const u16 data)
{
	GetMemByAddr(addr).Write16(addr, data);
}

void MemoryBase::Write32(u64 addr, const u32 data)
{
	GetMemByAddr(addr).Write32(addr, data);
}

void MemoryBase::Write64(u64 addr, const u64 data)
{
	GetMemByAddr(addr).Write64(addr, data);
}

void MemoryBase::Write128(u64 addr, const u128 data)
{
	GetMemByAddr(addr).Write128(addr, data);
}

bool MemoryBase::Write8NN(u64 addr, const u8 data)
{
	if(!IsGoodAddr(addr)) return false;
	Write8(addr, data);
	return true;
}

bool MemoryBase::Write16NN(u64 addr, const u16 data)
{
	if(!IsGoodAddr(addr, 2)) return false;
	Write16(addr, data);
	return true;
}

bool MemoryBase::Write32NN(u64 addr, const u32 data)
{
	if(!IsGoodAddr(addr, 4)) return false;
	Write32(addr, data);
	return true;
}

bool MemoryBase::Write64NN(u64 addr, const u64 data)
{
	if(!IsGoodAddr(addr, 8)) return false;
	Write64(addr, data);
	return true;
}

bool MemoryBase::Write128NN(u64 addr, const u128 data)
{
	if(!IsGoodAddr(addr, 16)) return false;
	Write128(addr, data);
	return true;
}

u8 MemoryBase::Read8(u64 addr)
{
	MemoryBlock& mem = GetMemByAddr(addr);
	if(mem.IsNULL())
	{
		mem.Read8(addr, NULL);
		return 0;
	}
	return mem.FastRead8(mem.FixAddr(addr));
}

u16 MemoryBase::Read16(u64 addr)
{
	MemoryBlock& mem = GetMemByAddr(addr);
	if(mem.IsNULL())
	{
		mem.Read16(addr, NULL);
		return 0;
	}
	return mem.FastRead16(mem.FixAddr(addr));
}

u32 MemoryBase::Read32(u64 addr)
{
	MemoryBlock& mem = GetMemByAddr(addr);
	if(mem.IsNULL())
	{
		mem.Read32(addr, NULL);
		return 0;
	}
	return mem.FastRead32(mem.FixAddr(addr));
}

u64 MemoryBase::Read64(u64 addr)
{
	MemoryBlock& mem = GetMemByAddr(addr);
	if(mem.IsNULL())
	{
		mem.Read64(addr, NULL);
		return 0;
	}
	return mem.FastRead64(mem.FixAddr(addr));
}

u128 MemoryBase::Read128(u64 addr)
{
	MemoryBlock& mem = GetMemByAddr(addr);
	if(mem.IsNULL())
	{
		mem.Read128(addr, NULL);
		return u128::From32(0);
	}
	return mem.FastRead128(mem.FixAddr(addr));
}
