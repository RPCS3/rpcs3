#include "stdafx.h"
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
		return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
#else
		return mmap(nullptr, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
	}

	void commit_page_memory(void* pointer, size_t page_size)
	{
#ifdef _WIN32
		VirtualAlloc((u8*)pointer, page_size, MEM_COMMIT, PAGE_READWRITE);
#else
		mprotect((u8*)pointer, page_size, PROT_READ | PROT_WRITE);
#endif
	}

	void free_reserved_memory(void* pointer, size_t size)
	{
#ifdef _WIN32
		VirtualFree(pointer, 0, MEM_RELEASE);
#else
		munmap(pointer, size);
#endif
	}
}