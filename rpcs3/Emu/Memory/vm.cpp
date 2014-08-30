#include "stdafx.h"

namespace vm
{
	bool check_addr(u32 addr)
	{
		// Checking address before using it is unsafe.
		// The only safe way to check it is to protect both actions (checking and using) with mutex that is used for mapping/allocation.
		return false;
	}

	//TODO
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

	void write32(u32 addr, u32 value)
	{
	}
}