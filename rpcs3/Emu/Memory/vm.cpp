#include "stdafx.h"
#include "Memory.h"

#ifdef _WIN32
#include <Windows.h>

void* const m_base_addr = VirtualAlloc(nullptr, 0x100000000, MEM_RESERVE, PAGE_NOACCESS);
#else
#include <sys/mman.h>

/* OS X uses MAP_ANON instead of MAP_ANONYMOUS */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

void* const m_base_addr = ::mmap(nullptr, 0x100000000, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
#endif

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
}