#pragma once

#define PAGE_4K(x) (x + 4095) & ~(4095)

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
	void* mem;

	MemBlockInfo(u64 _addr, u32 _size)
		: MemInfo(_addr, PAGE_4K(_size))
		, mem(_aligned_malloc(PAGE_4K(_size), 128))
	{
		if(!mem)
		{
			ConLog.Error("Not enought free memory.");
			assert(0);
		}
		
		memset(mem, 0, size);
	}

	~MemBlockInfo()
	{
		_aligned_free(mem);
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
	void Init();
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

	__forceinline const u8 FastRead8(const u64 addr) const;
	__forceinline const u16 FastRead16(const u64 addr) const;
	__forceinline const u32 FastRead32(const u64 addr) const;
	__forceinline const u64 FastRead64(const u64 addr) const;
	__forceinline const u128 FastRead128(const u64 addr);

	virtual bool Read8(const u64 addr, u8* value);
	virtual bool Read16(const u64 addr, u16* value);
	virtual bool Read32(const u64 addr, u32* value);
	virtual bool Read64(const u64 addr, u64* value);
	virtual bool Read128(const u64 addr, u128* value);

	__forceinline void FastWrite8(const u64 addr, const u8 value);
	__forceinline void FastWrite16(const u64 addr, const u16 value);
	__forceinline void FastWrite32(const u64 addr, const u32 value);
	__forceinline void FastWrite64(const u64 addr, const u64 value);
	__forceinline void FastWrite128(const u64 addr, const u128 value);

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
	Array<MemBlockInfo> m_allocated; // allocation info
	Array<u8*> m_pages; // real addresses of every 4096 byte pages (array size should be fixed)
	Array<u8*> m_locked; // locked pages should be moved here
	
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
	Array<VirtualMemInfo> m_mapped_memory;
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
	virtual u32 GetResevedAmount();

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

	// return the real address given a mapped address, if not mapped return 0
	u64 getRealAddr(u64 addr);

	// return the mapped address given a real address, if not mapped return 0
	u64 getMappedAddress(u64 realAddress);
};

#include "DynamicMemoryBlockBase.inl"

typedef DynamicMemoryBlockBase<MemoryBlock> DynamicMemoryBlock;
typedef DynamicMemoryBlockBase<MemoryBlockLE> DynamicMemoryBlockLE;
