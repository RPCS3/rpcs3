#include "stdafx.h"
#include <atomic>

#include "Utilities/Log.h"
#include "Memory.h"

MemoryBase Memory;

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

	mem = nullptr;
}

void MemoryBlock::InitMemory()
{
	if (!range_size) return;

	Free();
	mem_inf = new MemBlockInfo(range_start, range_size);
	mem = (u8*)mem_inf->mem;
}

void MemoryBlock::Free()
{
	if (mem_inf)
	{
		delete mem_inf;
		mem_inf = nullptr;
	}
	mem = nullptr;
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

bool MemoryBlock::GetMemFromAddr(void* dst, const u64 addr, const u32 size)
{
	if(!IsMyAddress(addr) || FixAddr(addr) + size > GetSize()) return false;

	return Memory.CopyToReal(dst, addr, size);
}

bool MemoryBlock::SetMemFromAddr(void* src, const u64 addr, const u32 size)
{
	if(!IsMyAddress(addr) || FixAddr(addr) + size > GetSize()) return false;

	return Memory.CopyFromReal(addr, src, size);
}

bool MemoryBlock::GetMemFFromAddr(void* dst, const u64 addr)
{
	if(!IsMyAddress(addr)) return false;

	dst = GetMem(FixAddr(addr));

	return true;
}

u8* MemoryBlock::GetMemFromAddr(const u64 addr)
{
	if(!IsMyAddress(addr) || IsNULL()) return nullptr;

	return GetMem(FixAddr(addr));
}

MemoryBlock* MemoryBlock::SetRange(const u64 start, const u32 size)
{
	range_start = start;
	range_size = size;

	InitMemory();
	return this;
}

bool MemoryBlock::IsMyAddress(const u64 addr)
{
	return mem && addr >= GetStartAddr() && addr < GetEndAddr();
}

template <typename T>
__forceinline const T MemoryBlock::FastRead(const u64 addr) const
{
	volatile const T data = *(const T *)GetMem(addr);
	return re(data);
}

template <>
__forceinline const u128 MemoryBlock::FastRead<u128>(const u64 addr) const
{
	volatile const u128 data = *(const u128 *)GetMem(addr);
	u128 ret;
	ret.lo = re(data.hi);
	ret.hi = re(data.lo);
	return ret;
}

bool MemoryBlock::Read8(const u64 addr, u8* value)
{
	if(!IsMyAddress(addr))
	{
		*value = 0;
		return false;
	}

	*value = FastRead<u8>(FixAddr(addr));
	return true;
}

bool MemoryBlock::Read16(const u64 addr, u16* value)
{
	if(!IsMyAddress(addr))
	{
		*value = 0;
		return false;
	}

	*value = FastRead<u16>(FixAddr(addr));
	return true;
}

bool MemoryBlock::Read32(const u64 addr, u32* value)
{
	if(!IsMyAddress(addr))
	{
		*value = 0;
		return false;
	}

	*value = FastRead<u32>(FixAddr(addr));
	return true;
}

bool MemoryBlock::Read64(const u64 addr, u64* value)
{
	if(!IsMyAddress(addr))
	{
		*value = 0;
		return false;
	}

	*value = FastRead<u64>(FixAddr(addr));
	return true;
}

bool MemoryBlock::Read128(const u64 addr, u128* value)
{
	if(!IsMyAddress(addr))
	{
		*value = u128::From32(0);
		return false;
	}

	*value = FastRead<u128>(FixAddr(addr));
	return true;
}

template <typename T>
__forceinline void MemoryBlock::FastWrite(const u64 addr, const T value)
{
	*(T *)GetMem(addr) = re(value);
}

template <>
__forceinline void MemoryBlock::FastWrite<u128>(const u64 addr, const u128 value)
{
	u128 res;
	res.lo = re(value.hi);
	res.hi = re(value.lo);
	*(u128*)GetMem(addr) = res;
}

bool MemoryBlock::Write8(const u64 addr, const u8 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	FastWrite<u8>(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write16(const u64 addr, const u16 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	FastWrite<u16>(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write32(const u64 addr, const u32 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	FastWrite<u32>(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write64(const u64 addr, const u64 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	FastWrite<u64>(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write128(const u64 addr, const u128 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	FastWrite<u128>(FixAddr(addr), value);
	return true;
}

bool MemoryBlockLE::Read8(const u64 addr, u8* value)
{
	if(!IsMyAddress(addr)) return false;

	*value = *(u8*)GetMem(FixAddr(addr));
	return true;
}

bool MemoryBlockLE::Read16(const u64 addr, u16* value)
{
	if(!IsMyAddress(addr)) return false;

	*value = *(u16*)GetMem(FixAddr(addr));
	return true;
}

bool MemoryBlockLE::Read32(const u64 addr, u32* value)
{
	if(!IsMyAddress(addr)) return false;

	*value = *(u32*)GetMem(FixAddr(addr));
	return true;
}

bool MemoryBlockLE::Read64(const u64 addr, u64* value)
{
	if(!IsMyAddress(addr)) return false;

	*value = *(u64*)GetMem(FixAddr(addr));
	return true;
}

bool MemoryBlockLE::Read128(const u64 addr, u128* value)
{
	if(!IsMyAddress(addr)) return false;

	*value = *(u128*)GetMem(FixAddr(addr));
	return true;
}

bool MemoryBlockLE::Write8(const u64 addr, const u8 value)
{
	if(!IsMyAddress(addr)) return false;

	*(u8*)GetMem(FixAddr(addr)) = value;
	return true;
}

bool MemoryBlockLE::Write16(const u64 addr, const u16 value)
{
	if(!IsMyAddress(addr)) return false;

	*(u16*)GetMem(FixAddr(addr)) = value;
	return true;
}

bool MemoryBlockLE::Write32(const u64 addr, const u32 value)
{
	if(!IsMyAddress(addr)) return false;

	*(u32*)GetMem(FixAddr(addr)) = value;
	return true;
}

bool MemoryBlockLE::Write64(const u64 addr, const u64 value)
{
	if(!IsMyAddress(addr)) return false;

	*(u64*)GetMem(FixAddr(addr)) = value;
	return true;
}

bool MemoryBlockLE::Write128(const u64 addr, const u128 value)
{
	if(!IsMyAddress(addr)) return false;

	*(u128*)GetMem(FixAddr(addr)) = value;
	return true;
}

//MemoryBase
void MemoryBase::Write8(u64 addr, const u8 data)
{
	*(u8*)((u64)GetBaseAddr() + (u32)addr) = data;
}

void MemoryBase::Write16(u64 addr, const u16 data)
{
	*(u16*)((u64)GetBaseAddr() + (u32)addr) = re16(data);
}

void MemoryBase::Write32(u64 addr, const u32 data)
{
	if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET || addr >= 0x100000000 || !RawSPUMem[(addr - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET])
	{
		*(u32*)((u64)GetBaseAddr() + (u32)addr) = re32(data);
	}
	else
	{
		RawSPUMem[(addr - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET]->Write32(addr, data);
	}
}

void MemoryBase::Write64(u64 addr, const u64 data)
{
	*(u64*)((u64)GetBaseAddr() + (u32)addr) = re64(data);
}

void MemoryBase::Write128(u64 addr, const u128 data)
{
	*(u128*)((u64)GetBaseAddr() + (u32)addr) = re128(data);
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
	return *(u8*)((u64)GetBaseAddr() + (u32)addr);
}

u16 MemoryBase::Read16(u64 addr)
{
	return re16(*(u16*)((u64)GetBaseAddr() + (u32)addr));
}

u32 MemoryBase::Read32(u64 addr)
{
	if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET || addr >= 0x100000000 || !RawSPUMem[(addr - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET])
	{
		return re32(*(u32*)((u64)GetBaseAddr() + (u32)addr));
	}
	else
	{
		u32 res;
		RawSPUMem[(addr - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET]->Read32(addr, &res);
		return res;
	}
}

u64 MemoryBase::Read64(u64 addr)
{
	return re64(*(u64*)((u64)GetBaseAddr() + (u32)addr));
}

u128 MemoryBase::Read128(u64 addr)
{
	return re128(*(u128*)((u64)GetBaseAddr() + (u32)addr));
}

template<> __forceinline u64 MemoryBase::ReverseData<1>(u64 val) { return val; }
template<> __forceinline u64 MemoryBase::ReverseData<2>(u64 val) { return Reverse16(val); }
template<> __forceinline u64 MemoryBase::ReverseData<4>(u64 val) { return Reverse32(val); }
template<> __forceinline u64 MemoryBase::ReverseData<8>(u64 val) { return Reverse64(val); }

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

bool VirtualMemoryBlock::Read8(const u64 addr, u8* value)
{
	u64 realAddr;
	if(!getRealAddr(addr, realAddr))
		return false;
	*value = Memory.Read8(realAddr);
	return true;
}

bool VirtualMemoryBlock::Read16(const u64 addr, u16* value)
{
	u64 realAddr;
	if(!getRealAddr(addr, realAddr))
		return false;
	*value = Memory.Read16(realAddr);
	return true;
}

bool VirtualMemoryBlock::Read32(const u64 addr, u32* value)
{
	u64 realAddr;
	if (!getRealAddr(addr, realAddr))
		return false;
	*value = Memory.Read32(realAddr);
	return true;
}

bool VirtualMemoryBlock::Read64(const u64 addr, u64* value)
{
	u64 realAddr;
	if(!getRealAddr(addr, realAddr))
		return false;
	*value = Memory.Read64(realAddr);
	return true;
}

bool VirtualMemoryBlock::Read128(const u64 addr, u128* value)
{
	u64 realAddr;
	if(!getRealAddr(addr, realAddr))
		return false;
	*value = Memory.Read128(realAddr);
	return true;
}

bool VirtualMemoryBlock::Write8(const u64 addr, const u8 value)
{
	u64 realAddr;
	if(!getRealAddr(addr, realAddr))
		return false;
	Memory.Write8(realAddr, value);
	return true;
}

bool VirtualMemoryBlock::Write16(const u64 addr, const u16 value)
{
	u64 realAddr;
	if(!getRealAddr(addr, realAddr))
		return false;
	Memory.Write16(realAddr, value);
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

bool VirtualMemoryBlock::Write64(const u64 addr, const u64 value)
{
	u64 realAddr;
	if(!getRealAddr(addr, realAddr))
		return false;
	Memory.Write64(realAddr, value);
	return true;
}

bool VirtualMemoryBlock::Write128(const u64 addr, const u128 value)
{
	u64 realAddr;
	if(!getRealAddr(addr, realAddr))
		return false;
	Memory.Write128(realAddr, value);
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
