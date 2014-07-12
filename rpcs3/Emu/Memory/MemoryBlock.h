#pragma once

#define PAGE_4K(x) (x + 4095) & ~(4095)

union u128
{
	struct
	{
		u64 hi;
		u64 lo;
	};

	u64 _u64[2];
	u32 _u32[4];
	u16 _u16[8];
	u8  _u8[16];

	operator u64() const { return _u64[0]; }
	operator u32() const { return _u32[0]; }
	operator u16() const { return _u16[0]; }
	operator u8()  const { return _u8[0];  }

	operator bool() const { return _u64[0] != 0 || _u64[1] != 0; }

	static u128 From128( u64 hi, u64 lo )
	{
		u128 ret = {hi, lo};
		return ret;
	}

	static u128 From64( u64 src )
	{
		u128 ret = {0, src};
		return ret;
	}

	static u128 From32( u32 src )
	{
		u128 ret;
		ret._u32[0] = src;
		ret._u32[1] = 0;
		ret._u32[2] = 0;
		ret._u32[3] = 0;
		return ret;
	}

	static u128 FromBit ( u32 bit )
	{
		u128 ret;
		if (bit < 64)
		{
			ret.hi = 0;
			ret.lo = (u64)1 << bit;
		}
		else if (bit < 128)
		{
			ret.hi = (u64)1 << (bit - 64);
			ret.lo = 0;
		}
		else
		{
			ret.hi = 0;
			ret.lo = 0;
		}
		return ret;
	}

	bool operator == ( const u128& right ) const
	{
		return (lo == right.lo) && (hi == right.hi);
	}

	bool operator != ( const u128& right ) const
	{
		return (lo != right.lo) || (hi != right.hi);
	}

	u128 operator | ( const u128& right ) const
	{
		return From128(hi | right.hi, lo | right.lo);
	}

	u128 operator & ( const u128& right ) const
	{
		return From128(hi & right.hi, lo & right.lo);
	}

	u128 operator ^ ( const u128& right ) const
	{
		return From128(hi ^ right.hi, lo ^ right.lo);
	}

	u128 operator ~ () const
	{
		return From128(~hi, ~lo);
	}
};

union s128
{
	struct
	{
		s64 hi;
		s64 lo;
	};

	u64 _i64[2];
	u32 _i32[4];
	u16 _i16[8];
	u8  _i8[16];

	operator s64() const { return _i64[0]; }
	operator s32() const { return _i32[0]; }
	operator s16() const { return _i16[0]; }
	operator s8()  const { return _i8[0];  }

	operator bool() const { return _i64[0] != 0 || _i64[1] != 0; }

	static s128 From64( s64 src )
	{
		s128 ret = {src, 0};
		return ret;
	}

	static s128 From32( s32 src )
	{
		s128 ret;
		ret._i32[0] = src;
		ret._i32[1] = 0;
		ret.hi = 0;
		return ret;
	}

	bool operator == ( const s128& right ) const
	{
		return (lo == right.lo) && (hi == right.hi);
	}

	bool operator != ( const s128& right ) const
	{
		return (lo != right.lo) || (hi != right.hi);
	}
};

#include <memory>
#include <emmintrin.h>

//TODO: SSE style
/*
struct u128
{
	__m128 m_val;

	u128 GetValue128()
	{
		u128 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}

	u64 GetValue64()
	{
		u64 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}

	u32 GetValue32()
	{
		u32 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}

	u16 GetValue16()
	{
		u16 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}

	u8 GetValue8()
	{
		u8 ret;
		_mm_store_ps( (float*)&ret, m_val );
		return ret;
	}
};
*/


struct MemInfo
{
	u64 addr;
	u32 size;

	MemInfo(u64 _addr, u32 _size)
		: addr(_addr)
		, size(_size)
	{
	}

	MemInfo() : addr(0), size(0)
	{
	}
};

struct MemBlockInfo : public MemInfo
{
	void *mem;

	MemBlockInfo(u64 _addr, u32 _size);

	void Free();

	MemBlockInfo(MemBlockInfo &other) = delete;

	MemBlockInfo(MemBlockInfo &&other)
		: MemInfo(other.addr,other.size)
		, mem(other.mem)
	{
		other.mem = nullptr;
	}

	MemBlockInfo& operator =(MemBlockInfo &other) = delete;

	MemBlockInfo& operator =(MemBlockInfo &&other)
	{
		this->Free();
		this->addr = other.addr;
		this->size = other.size;
		this->mem = other.mem;
		other.mem = nullptr;
		return *this;
	}

	~MemBlockInfo()
	{
		Free();
		mem = nullptr;
	}
};

struct VirtualMemInfo : public MemInfo
{
	u64 realAddress;

	VirtualMemInfo(u64 _addr, u64 _realaddr, u32 _size)
		: MemInfo(_addr, _size)
		, realAddress(_realaddr)
	{
	}

	VirtualMemInfo()
		: MemInfo(0, 0)
		, realAddress(0)
	{
	}
};

class MemoryBlock
{
protected:
	u8* mem;
	u64 range_start;
	u64 range_size;

public:
	MemoryBlock();
	virtual ~MemoryBlock();

private:
	MemBlockInfo* mem_inf;
	void Init();
	void Free();
	void InitMemory();

public:
	virtual void Delete();

	virtual bool IsNULL() { return false; }
	virtual bool IsMirror() { return false; }

	u64 FixAddr(const u64 addr) const;

	bool GetMemFromAddr(void* dst, const u64 addr, const u32 size);
	bool SetMemFromAddr(void* src, const u64 addr, const u32 size);
	bool GetMemFFromAddr(void* dst, const u64 addr);
	u8* GetMemFromAddr(const u64 addr);

	virtual MemoryBlock* SetRange(const u64 start, const u32 size);
	virtual bool IsMyAddress(const u64 addr);
	virtual bool IsLocked(const u64 addr) { return false; }

	template <typename T>
	__forceinline const T FastRead(const u64 addr) const;

	virtual bool Read8(const u64 addr, u8* value);
	virtual bool Read16(const u64 addr, u16* value);
	virtual bool Read32(const u64 addr, u32* value);
	virtual bool Read64(const u64 addr, u64* value);
	virtual bool Read128(const u64 addr, u128* value);

	template <typename T>
	__forceinline void FastWrite(const u64 addr, const T value);

	virtual bool Write8(const u64 addr, const u8 value);
	virtual bool Write16(const u64 addr, const u16 value);
	virtual bool Write32(const u64 addr, const u32 value);
	virtual bool Write64(const u64 addr, const u64 value);
	virtual bool Write128(const u64 addr, const u128 value);

	const u64 GetStartAddr() const { return range_start; }
	const u64 GetEndAddr() const { return GetStartAddr() + GetSize() - 1; }
	virtual const u32 GetSize() const { return range_size; }
	virtual const u32 GetUsedSize() const { return GetSize(); }
	u8* GetMem() const { return mem; }
	virtual u8* GetMem(u64 addr) const { return mem + addr; }

	virtual bool AllocFixed(u64 addr, u32 size) { return false; }
	virtual u64 AllocAlign(u32 size, u32 align = 1) { return 0; }
	virtual bool Alloc() { return false; }
	virtual bool Free(u64 addr) { return false; }
	virtual bool Lock(u64 addr, u32 size) { return false; }
	virtual bool Unlock(u64 addr, u32 size) { return false; }
};

class MemoryBlockLE : public MemoryBlock
{
public:
	virtual bool Read8(const u64 addr, u8* value) override;
	virtual bool Read16(const u64 addr, u16* value) override;
	virtual bool Read32(const u64 addr, u32* value) override;
	virtual bool Read64(const u64 addr, u64* value) override;
	virtual bool Read128(const u64 addr, u128* value) override;

	virtual bool Write8(const u64 addr, const u8 value) override;
	virtual bool Write16(const u64 addr, const u16 value) override;
	virtual bool Write32(const u64 addr, const u32 value) override;
	virtual bool Write64(const u64 addr, const u64 value) override;
	virtual bool Write128(const u64 addr, const u128 value) override;
};

class MemoryMirror : public MemoryBlock
{
public:
	virtual bool IsMirror() { return true; }

	virtual MemoryBlock* SetRange(const u64 start, const u32 size)
	{
		range_start = start;
		range_size = size;

		return this;
	}

	void SetMemory(u8* memory)
	{
		mem = memory;
	}

	MemoryBlock* SetRange(u8* memory, const u64 start, const u32 size)
	{
		SetMemory(memory);
		return SetRange(start, size);
	}
};

class NullMemoryBlock : public MemoryBlock
{
public:
	virtual bool IsNULL() { return true; }
	virtual bool IsMyAddress(const u64 addr) { return true; }

	virtual bool Read8(const u64 addr, u8* value);
	virtual bool Read16(const u64 addr, u16* value);
	virtual bool Read32(const u64 addr, u32* value);
	virtual bool Read64(const u64 addr, u64* value);
	virtual bool Read128(const u64 addr, u128* value);

	virtual bool Write8(const u64 addr, const u8 value);
	virtual bool Write16(const u64 addr, const u16 value);
	virtual bool Write32(const u64 addr, const u32 value);
	virtual bool Write64(const u64 addr, const u64 value);
	virtual bool Write128(const u64 addr, const u128 value);
};

template<typename PT>
class DynamicMemoryBlockBase : public PT
{
	mutable std::mutex m_lock;
	std::vector<MemBlockInfo> m_allocated; // allocation info
	u32 m_max_size;

public:
	DynamicMemoryBlockBase();

	const u32 GetSize() const { return m_max_size; }
	const u32 GetUsedSize() const;

	virtual bool IsInMyRange(const u64 addr);
	virtual bool IsInMyRange(const u64 addr, const u32 size);
	virtual bool IsMyAddress(const u64 addr);
	virtual bool IsLocked(const u64 addr);

	virtual MemoryBlock* SetRange(const u64 start, const u32 size);

	virtual void Delete();

	virtual bool AllocFixed(u64 addr, u32 size);
	virtual u64 AllocAlign(u32 size, u32 align = 1);
	virtual bool Alloc();
	virtual bool Free(u64 addr);
	virtual bool Lock(u64 addr, u32 size);
	virtual bool Unlock(u64 addr, u32 size);

	virtual u8* GetMem(u64 addr) const;

private:
	void AppendMem(u64 addr, u32 size);
};

class VirtualMemoryBlock : public MemoryBlock
{
	std::vector<VirtualMemInfo> m_mapped_memory;
	u32 m_reserve_size;

public:
	VirtualMemoryBlock();

	virtual MemoryBlock* SetRange(const u64 start, const u32 size);
	virtual bool IsInMyRange(const u64 addr);
	virtual bool IsInMyRange(const u64 addr, const u32 size);
	virtual bool IsMyAddress(const u64 addr);
	virtual void Delete();

	// maps real address to virtual address space, returns the mapped address or 0 on failure (if no address is specified the
	// first mappable space is used)
	virtual u64 Map(u64 realaddr, u32 size, u64 addr = 0);

	// Unmap real address (please specify only starting point, no midway memory will be unmapped), returns the size of the unmapped area
	virtual u32 UnmapRealAddress(u64 realaddr);

	// Unmap address (please specify only starting point, no midway memory will be unmapped), returns the size of the unmapped area
	virtual u32 UnmapAddress(u64 addr);

	// Reserve a certain amount so no one can use it, returns true on succces, false on failure
	virtual bool Reserve(u32 size);

	// Unreserve a certain amount of bytes, returns true on succcess, false if size is bigger than the reserved amount
	virtual bool Unreserve(u32 size);

	// Return the total amount of reserved memory
	virtual u32 GetReservedAmount();

	virtual bool Read8(const u64 addr, u8* value);
	virtual bool Read16(const u64 addr, u16* value);
	virtual bool Read32(const u64 addr, u32* value);
	virtual bool Read64(const u64 addr, u64* value);
	virtual bool Read128(const u64 addr, u128* value);

	virtual bool Write8(const u64 addr, const u8 value);
	virtual bool Write16(const u64 addr, const u16 value);
	virtual bool Write32(const u64 addr, const u32 value);
	virtual bool Write64(const u64 addr, const u64 value);
	virtual bool Write128(const u64 addr, const u128 value);

	// try to get the real address given a mapped address
	// return true for success
	bool getRealAddr(u64 addr, u64& result);

	u64 RealAddr(u64 addr)
	{
		u64 realAddr = 0;
		getRealAddr(addr, realAddr);
		return realAddr;
	}

	// return the mapped address given a real address, if not mapped return 0
	u64 getMappedAddress(u64 realAddress);
};

#include "DynamicMemoryBlockBase.h"

typedef DynamicMemoryBlockBase<MemoryBlock> DynamicMemoryBlock;
typedef DynamicMemoryBlockBase<MemoryBlockLE> DynamicMemoryBlockLE;

