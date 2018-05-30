#pragma once

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

class VirtualMemoryBlock
{
	std::vector<VirtualMemInfo> m_mapped_memory;
	u32 m_reserve_size = 0;
	u32 m_range_start = 0;
	u32 m_range_size = 0;

public:
	VirtualMemoryBlock() = default;

	VirtualMemoryBlock* SetRange(const u32 start, const u32 size);
	void Clear() { m_mapped_memory.clear(); m_reserve_size = 0; m_range_start = 0; m_range_size = 0; }
	u32 GetStartAddr() const { return m_range_start; }
	u32 GetSize() const { return m_range_size; }
	bool IsInMyRange(const u32 addr, const u32 size);

	// maps real address to virtual address space
	bool Map(u32 realaddr, u32 size, u32 addr);

	// Unmap real address (please specify only starting point, no midway memory will be unmapped), returns the size of the unmapped area
	bool UnmapRealAddress(u32 realaddr, u32& size);

	// Unmap address (please specify only starting point, no midway memory will be unmapped), returns the size of the unmapped area
	bool UnmapAddress(u32 addr, u32& size);

	// Reserve a certain amount so no one can use it, returns true on success, false on failure
	bool Reserve(u32 size);

	// Unreserve a certain amount of bytes, returns true on success, false if size is bigger than the reserved amount
	bool Unreserve(u32 size);

	// Return the total amount of reserved memory
	u32 GetReservedAmount();

	// Return the start of the mapped space
	u32 GetRangeStart();

	// Return the end of the mapped space
	u32 GetRangeEnd();

	bool Read32(const u32 addr, u32* value);

	bool Write32(const u32 addr, const u32 value);

	// try to get the real address given a mapped address
	// return true for success
	bool getRealAddr(u32 addr, u32& result);

	u32 RealAddr(u32 addr)
	{
		u32 realAddr = 0;

		if (!getRealAddr(addr, realAddr))
		{
			LOG_ERROR(HLE, "Getting the real address failed. (addr=0x%x)", addr);
			return 0;
		}

		return realAddr;
	}

	// return the mapped address given a real address, if not mapped return minus one
	s32 getMappedAddress(u32 realAddress);
};
