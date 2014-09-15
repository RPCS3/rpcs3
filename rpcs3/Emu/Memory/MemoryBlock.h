#pragma once

#define PAGE_4K(x) (x + 4095) & ~(4095)

//#include <emmintrin.h>

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
	u32 range_size;

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

	u64 FixAddr(const u64 addr) const;

	virtual MemoryBlock* SetRange(const u64 start, const u32 size);
	virtual bool IsMyAddress(const u64 addr);
	virtual bool IsLocked(const u64 addr) { return false; }

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

class DynamicMemoryBlockBase : public MemoryBlock
{
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

	bool Read32(const u64 addr, u32* value);

	bool Write32(const u64 addr, const u32 value);

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

typedef DynamicMemoryBlockBase DynamicMemoryBlock;

