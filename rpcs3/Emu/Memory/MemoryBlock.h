#pragma once

#define PAGE_4K(x) (x + 4095) & ~(4095)

struct MemInfo
{
	u32 addr;
	u32 size;

	MemInfo(u32 addr, u32 size)
		: addr(addr)
		, size(size)
	{
	}

	MemInfo()
		: addr(0)
		, size(0)
	{
	}
};

struct MemBlockInfo : public MemInfo
{
	MemBlockInfo(u32 addr, u32 size);

	void Free();

	MemBlockInfo(MemBlockInfo &other) = delete;

	MemBlockInfo(MemBlockInfo &&other)
		: MemInfo(other.addr,other.size)
	{
		other.addr = 0;
		other.size = 0;
	}

	MemBlockInfo& operator =(MemBlockInfo &other) = delete;

	MemBlockInfo& operator =(MemBlockInfo &&other)
	{
		Free();
		this->addr = other.addr;
		this->size = other.size;
		other.addr = 0;
		other.size = 0;
		return *this;
	}

	~MemBlockInfo()
	{
		Free();
	}
};

struct VirtualMemInfo : public MemInfo
{
	u32 realAddress;

	VirtualMemInfo(u32 addr, u32 realaddr, u32 size)
		: MemInfo(addr, size)
		, realAddress(realaddr)
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
	u32 range_start;
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

	virtual MemoryBlock* SetRange(const u32 start, const u32 size);

	const u32 GetStartAddr() const { return range_start; }
	const u32 GetEndAddr() const { return GetStartAddr() + GetSize() - 1; }
	virtual const u32 GetSize() const { return range_size; }
	virtual const u32 GetUsedSize() const { return GetSize(); }

	virtual bool AllocFixed(u32 addr, u32 size) { return false; }
	virtual u32 AllocAlign(u32 size, u32 align = 1) { return 0; }
	virtual bool Alloc() { return false; }
	virtual bool Free(u32 addr) { return false; }
};

class DynamicMemoryBlockBase : public MemoryBlock
{
	std::vector<MemBlockInfo> m_allocated; // allocation info
	u32 m_max_size;

public:
	DynamicMemoryBlockBase();

	const u32 GetSize() const { return m_max_size; }
	const u32 GetUsedSize() const;

	virtual bool IsInMyRange(const u32 addr, const u32 size = 1);

	virtual MemoryBlock* SetRange(const u32 start, const u32 size);

	virtual void Delete();

	virtual bool AllocFixed(u32 addr, u32 size);
	virtual u32 AllocAlign(u32 size, u32 align = 1);
	virtual bool Alloc();
	virtual bool Free(u32 addr);

private:
	void AppendMem(u32 addr, u32 size);
};

class VirtualMemoryBlock : public MemoryBlock
{
	std::vector<VirtualMemInfo> m_mapped_memory;
	u32 m_reserve_size;

public:
	VirtualMemoryBlock();

	virtual MemoryBlock* SetRange(const u32 start, const u32 size);
	virtual bool IsInMyRange(const u32 addr, const u32 size = 1);
	virtual void Delete();

	// maps real address to virtual address space, returns the mapped address or 0 on failure (if no address is specified the
	// first mappable space is used)
	virtual bool Map(u32 realaddr, u32 size, u32 addr);
	virtual u32 Map(u32 realaddr, u32 size);

	// Unmap real address (please specify only starting point, no midway memory will be unmapped), returns the size of the unmapped area
	virtual bool UnmapRealAddress(u32 realaddr, u32& size);

	// Unmap address (please specify only starting point, no midway memory will be unmapped), returns the size of the unmapped area
	virtual bool UnmapAddress(u32 addr, u32& size);

	// Reserve a certain amount so no one can use it, returns true on succces, false on failure
	virtual bool Reserve(u32 size);

	// Unreserve a certain amount of bytes, returns true on succcess, false if size is bigger than the reserved amount
	virtual bool Unreserve(u32 size);

	// Return the total amount of reserved memory
	virtual u32 GetReservedAmount();

	bool Read32(const u32 addr, u32* value);

	bool Write32(const u32 addr, const u32 value);

	// try to get the real address given a mapped address
	// return true for success
	bool getRealAddr(u32 addr, u32& result);

	u32 RealAddr(u32 addr)
	{
		u32 realAddr = 0;
		getRealAddr(addr, realAddr);
		return realAddr;
	}

	// return the mapped address given a real address, if not mapped return 0
	u32 getMappedAddress(u32 realAddress);
};

typedef DynamicMemoryBlockBase DynamicMemoryBlock;
