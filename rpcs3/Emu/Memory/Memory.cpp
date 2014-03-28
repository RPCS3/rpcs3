#include "stdafx.h"
#include "Memory.h"
#include "MemoryBlock.h"
#include <atomic>

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

	mem = nullptr;
}

void MemoryBlock::InitMemory()
{
	if(!range_size) return;

	if(mem) safe_free(mem);
	mem = (u8*)malloc(range_size);
	memset(mem, 0, range_size);
}

void MemoryBlock::Delete()
{
	if(mem) safe_free(mem);
	Init();
}

u64 MemoryBlock::FixAddr(const u64 addr) const
{
	return addr - GetStartAddr();
}

bool MemoryBlock::GetMemFromAddr(void* dst, const u64 addr, const u32 size)
{
	if(!IsMyAddress(addr) || FixAddr(addr) + size > GetSize()) return false;

	// mem cpy(dst, GetMem(FixAddr(addr)), size);
	return Memory.CopyToReal(dst, (u32)addr, size);
}

bool MemoryBlock::SetMemFromAddr(void* src, const u64 addr, const u32 size)
{
	if(!IsMyAddress(addr) || FixAddr(addr) + size > GetSize()) return false;

	// mem cpy(GetMem(FixAddr(addr)), src, size);
	return Memory.CopyFromReal((u32)addr, src, size);
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

__forceinline const u8 MemoryBlock::FastRead8(const u64 addr) const
{
	return *GetMem(addr);
}

__forceinline const u16 MemoryBlock::FastRead16(const u64 addr) const
{
	volatile const u16 data = *(u16*)GetMem(addr);
	return re(data);
}

__forceinline const u32 MemoryBlock::FastRead32(const u64 addr) const
{
	volatile const u32 data = *(u32*)GetMem(addr);
	return re(data);
}

__forceinline const u64 MemoryBlock::FastRead64(const u64 addr) const
{
	volatile const u64 data = *(u64*)GetMem(addr);
	return re(data);
}

__forceinline const u128 MemoryBlock::FastRead128(const u64 addr)
{
	volatile const u128 data = *(u128*)GetMem(addr);
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

	//*value = std::atomic_load((volatile std::atomic<u8>*)GetMem(FixAddr(addr)));
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

	//se_t<u16>::func(*value, std::atomic_load((volatile std::atomic<u16>*)GetMem(FixAddr(addr))));
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

	//se_t<u32>::func(*value, std::atomic_load((volatile std::atomic<u32>*)GetMem(FixAddr(addr))));
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

	//se_t<u64>::func(*value, std::atomic_load((volatile std::atomic<u64>*)GetMem(FixAddr(addr))));
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

	//u64 f_addr = FixAddr(addr);
	//se_t<u64>::func(value->lo, std::atomic_load((volatile std::atomic<u64>*)GetMem(f_addr)));
	//se_t<u64>::func(value->hi, std::atomic_load((volatile std::atomic<u64>*)GetMem(f_addr + 8)));
	*value = FastRead128(FixAddr(addr));
	return true;
}

__forceinline void MemoryBlock::FastWrite8(const u64 addr, const u8 value)
{
	*GetMem(addr) = value;
}

__forceinline void MemoryBlock::FastWrite16(const u64 addr, const u16 value)
{
	*(u16*)GetMem(addr) = re(value);
}

__forceinline void MemoryBlock::FastWrite32(const u64 addr, const u32 value)
{
	*(u32*)GetMem(addr) = re(value);
}

__forceinline void MemoryBlock::FastWrite64(const u64 addr, const u64 value)
{
	*(u64*)GetMem(addr) = re(value);
}

__forceinline void MemoryBlock::FastWrite128(const u64 addr, const u128 value)
{
	u128 res;
	res.lo = re(value.hi);
	res.hi = re(value.lo);
	*(u128*)GetMem(addr) = res;
}

bool MemoryBlock::Write8(const u64 addr, const u8 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	//std::atomic_store((std::atomic<u8>*)GetMem(FixAddr(addr)), value);
	FastWrite8(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write16(const u64 addr, const u16 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	//u16 re_value;
	//se_t<u16>::func(re_value, value);
	//std::atomic_store((std::atomic<u16>*)GetMem(FixAddr(addr)), re_value);
	FastWrite16(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write32(const u64 addr, const u32 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	//u32 re_value;
	//se_t<u32>::func(re_value, value);
	//std::atomic_store((std::atomic<u32>*)GetMem(FixAddr(addr)), re_value);
	FastWrite32(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write64(const u64 addr, const u64 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	//u64 re_value;
	//se_t<u64>::func(re_value, value);
	//std::atomic_store((std::atomic<u64>*)GetMem(FixAddr(addr)), re_value);
	FastWrite64(FixAddr(addr), value);
	return true;
}

bool MemoryBlock::Write128(const u64 addr, const u128 value)
{
	if(!IsMyAddress(addr) || IsLocked(addr)) return false;

	//u64 f_addr = FixAddr(addr);
	//u64 re_value;
	//se_t<u64>::func(re_value, value.lo);
	//std::atomic_store((std::atomic<u64>*)GetMem(f_addr), re_value);
	//se_t<u64>::func(re_value, value.hi);
	//std::atomic_store((std::atomic<u64>*)GetMem(f_addr + 8), re_value);
	FastWrite128(FixAddr(addr), value);
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

//NullMemoryBlock
bool NullMemoryBlock::Read8(const u64 addr, u8* )
{
	ConLog.Error("Read8 from null block: [%08llx]", addr);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
	return false;
}

bool NullMemoryBlock::Read16(const u64 addr, u16* )
{
	ConLog.Error("Read16 from null block: [%08llx]", addr);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
	return false;
}

bool NullMemoryBlock::Read32(const u64 addr, u32* )
{
	ConLog.Error("Read32 from null block: [%08llx]", addr);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
	return false;
}

bool NullMemoryBlock::Read64(const u64 addr, u64* )
{
	ConLog.Error("Read64 from null block: [%08llx]", addr);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
	return false;
}

bool NullMemoryBlock::Read128(const u64 addr, u128* )
{
	ConLog.Error("Read128 from null block: [%08llx]", addr);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write8(const u64 addr, const u8 value)
{
	ConLog.Error("Write8 to null block: [%08llx]: %x", addr, value);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write16(const u64 addr, const u16 value)
{
	ConLog.Error("Write16 to null block: [%08llx]: %x", addr, value);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write32(const u64 addr, const u32 value)
{
	ConLog.Error("Write32 to null block: [%08llx]: %x", addr, value);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write64(const u64 addr, const u64 value)
{
	ConLog.Error("Write64 to null block: [%08llx]: %llx", addr, value);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
	return false;
}

bool NullMemoryBlock::Write128(const u64 addr, const u128 value)
{
	ConLog.Error("Write128 to null block: [%08llx]: %llx_%llx", addr, value.hi, value.lo);
	if (!Ini.CPUIgnoreRWErrors.GetValue())
		Emu.Pause();
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
	u8 res;
	GetMemByAddr(addr).Read8(addr, &res);
	return res;
}

u16 MemoryBase::Read16(u64 addr)
{
	u16 res;
	GetMemByAddr(addr).Read16(addr, &res);
	return res;
}

u32 MemoryBase::Read32(u64 addr)
{
	u32 res;
	GetMemByAddr(addr).Read32(addr, &res);
	return res;
}

u64 MemoryBase::Read64(u64 addr)
{
	u64 res;
	GetMemByAddr(addr).Read64(addr, &res);
	return res;
}

u128 MemoryBase::Read128(u64 addr)
{
	u128 res;
	GetMemByAddr(addr).Read128(addr, &res);
	return res;
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
	return addr >= GetStartAddr() && addr < GetStartAddr() + GetSize() - GetResevedAmount();
}

bool VirtualMemoryBlock::IsInMyRange(const u64 addr, const u32 size)
{
	return IsInMyRange(addr) && IsInMyRange(addr + size - 1);
}

bool VirtualMemoryBlock::IsMyAddress(const u64 addr)
{
	for(u32 i=0; i<m_mapped_memory.GetCount(); ++i)
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

		m_mapped_memory.Move(new VirtualMemInfo(addr, realaddr, size));
		return addr;
	}
	else
	{
		for(u64 addr = GetStartAddr(); addr <= GetEndAddr() - GetResevedAmount() - size;)
		{
			bool is_good_addr = true;

			// check if address is already mapped
			for(u32 i=0; i<m_mapped_memory.GetCount(); ++i)
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

			m_mapped_memory.Move(new VirtualMemInfo(addr, realaddr, size));

			return addr;
		}

		return 0;
	}
}

u32 VirtualMemoryBlock::UnmapRealAddress(u64 realaddr)
{
	for(u32 i=0; i<m_mapped_memory.GetCount(); ++i)
	{
		if(m_mapped_memory[i].realAddress == realaddr && IsInMyRange(m_mapped_memory[i].addr, m_mapped_memory[i].size))
		{
			u32 size = m_mapped_memory[i].size;
			m_mapped_memory.RemoveAt(i);
			return size;
		}
	}

	return 0;
}

u32 VirtualMemoryBlock::UnmapAddress(u64 addr)
{
	for(u32 i=0; i<m_mapped_memory.GetCount(); ++i)
	{
		if(m_mapped_memory[i].addr == addr && IsInMyRange(m_mapped_memory[i].addr, m_mapped_memory[i].size))
		{
			u32 size = m_mapped_memory[i].size;
			m_mapped_memory.RemoveAt(i);
			return size;
		}
	}

	return 0;
}

bool VirtualMemoryBlock::Read8(const u64 addr, u8* value)
{
	u64 realAddr;
	*value = Memory.Read8(realAddr = getRealAddr(addr));
	return realAddr != 0;
}

bool VirtualMemoryBlock::Read16(const u64 addr, u16* value)
{
	u64 realAddr;
	*value = Memory.Read16(realAddr = getRealAddr(addr));
	return realAddr != 0;
}

bool VirtualMemoryBlock::Read32(const u64 addr, u32* value)
{
	u64 realAddr;
	*value = Memory.Read32(realAddr = getRealAddr(addr));
	return realAddr != 0;
}

bool VirtualMemoryBlock::Read64(const u64 addr, u64* value)
{
	u64 realAddr;
	*value = Memory.Read64(realAddr = getRealAddr(addr));
	return realAddr != 0;
}

bool VirtualMemoryBlock::Read128(const u64 addr, u128* value)
{
	u64 realAddr;
	*value = Memory.Read128(realAddr = getRealAddr(addr));
	return realAddr != 0;
}

bool VirtualMemoryBlock::Write8(const u64 addr, const u8 value)
{
	u64 realAddr;
	Memory.Write8(realAddr = getRealAddr(addr), value);
	return realAddr != 0;
}

bool VirtualMemoryBlock::Write16(const u64 addr, const u16 value)
{
	u64 realAddr;
	Memory.Write16(realAddr = getRealAddr(addr), value);
	return realAddr != 0;
}

bool VirtualMemoryBlock::Write32(const u64 addr, const u32 value)
{
	u64 realAddr;
	Memory.Write32(realAddr = getRealAddr(addr), value);
	return realAddr != 0;
}

bool VirtualMemoryBlock::Write64(const u64 addr, const u64 value)
{
	u64 realAddr;
	Memory.Write64(realAddr = getRealAddr(addr), value);
	return realAddr != 0;
}

bool VirtualMemoryBlock::Write128(const u64 addr, const u128 value)
{
	u64 realAddr;
	Memory.Write128(realAddr = getRealAddr(addr), value);
	return realAddr != 0;
}

u64 VirtualMemoryBlock::getRealAddr(u64 addr)
{
	for(u32 i=0; i<m_mapped_memory.GetCount(); ++i)
	{
		if(addr >= m_mapped_memory[i].addr && addr < m_mapped_memory[i].addr + m_mapped_memory[i].size)
		{
			return m_mapped_memory[i].realAddress + (addr - m_mapped_memory[i].addr);
		}
	}

	return 0;
}

u64 VirtualMemoryBlock::getMappedAddress(u64 realAddress)
{
	for(u32 i=0; i<m_mapped_memory.GetCount(); ++i)
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
	m_mapped_memory.Clear();

	MemoryBlock::Delete();
}

bool VirtualMemoryBlock::Reserve(u32 size)
{
	if(size + GetResevedAmount() > GetEndAddr() - GetStartAddr())
		return false;

	m_reserve_size += size;
	return true;
}

bool VirtualMemoryBlock::Unreserve(u32 size)
{
	if(size > GetResevedAmount())
		return false;

	m_reserve_size -= size;
	return true;
}

u32 VirtualMemoryBlock::GetResevedAmount()
{
	return m_reserve_size;
}