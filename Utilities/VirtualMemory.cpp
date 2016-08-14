#include "stdafx.h"
#include "Utilities/Log.h"
#include "VirtualMemory.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#endif

namespace memory_helper
{
	void* reserve_memory(size_t size)
	{
#ifdef _WIN32
		return verify(VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS), "reserve_memory" HERE);
#else
		return verify(::mmap(nullptr, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0), "reserve_memory" HERE);
#endif
	}

	void commit_page_memory(void* pointer, size_t size)
	{
#ifdef _WIN32
		verify(HERE), VirtualAlloc(pointer, size, MEM_COMMIT, PAGE_READWRITE);
#else
		verify(HERE), ::mprotect((void*)((u64)pointer & -4096), size, PROT_READ | PROT_WRITE) != -1;
#endif
	}

	void free_reserved_memory(void* pointer, size_t size)
	{
#ifdef _WIN32
		verify(HERE), VirtualFree(pointer, 0, MEM_DECOMMIT);
#else
		verify(HERE), ::mprotect(pointer, size, PROT_NONE) != -1;
#endif
	}
}
