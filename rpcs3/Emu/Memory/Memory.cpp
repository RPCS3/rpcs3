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

	mem = nullptr;
}

void MemoryBlock::InitMemory()
{
	if(!range_size) return;

	safe_delete(mem);
	mem = (u8*)malloc(range_size);
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
	if(!IsMyAddress(addr) || FixAddr(addr) + size > GetSize()) return false;

	memcpy(dst, GetMem(FixAddr(addr)), size);

	return true;
}

bool MemoryBlock::SetMemFromAddr(void* src, const u64 addr, const u32 size)
{
	if(!IsMyAddress(addr) || FixAddr(addr) + size > GetSize()) return false;

	memcpy(GetMem(FixAddr(addr)), src, size);

	return true;
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
	ret.lo = FastRead64(addr);
	ret.hi = FastRead64(addr + 8);
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
	*GetMem(addr) = value;
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
	FastWrite64(addr, value.lo);
	FastWrite64(addr+8, value.hi);
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
