#include "stdafx.h"

namespace vm
{
	//TODO
	bool check_addr(u32 addr)
	{
		return false;
	}

	bool map(u32 addr, u32 size, u32 flags)
	{
		return false;
	}

	bool unmap(u32 addr, u32 size, u32 flags)
	{
		return false;
	}

	u32 alloc(u32 size)
	{
		return 0;
	}

	void unalloc(u32 addr)
	{
	}

	u32 read32(u32 addr)
	{
		return 0;
	}

	bool read32(u32 addr, u32& value)
	{
		return false;
	}

	bool write32(u32 addr, u32 value)
	{
		return false;
	}
}