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
		void* ret = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
		ENSURES(ret != NULL);
#else
		void* ret = mmap(nullptr, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
		ENSURES(ret != 0);
#endif
		return ret;
	}

	void commit_page_memory(void* pointer, size_t size)
	{
#ifdef _WIN32
		VERIFY(VirtualAlloc(pointer, size, MEM_COMMIT, PAGE_READWRITE) != NULL);
#else
		VERIFY(mprotect((void*)((u64)pointer & -4096), size, PROT_READ | PROT_WRITE) != -1);
#endif
	}

	void free_reserved_memory(void* pointer, size_t size)
	{
#ifdef _WIN32
		VERIFY(VirtualFree(pointer, 0, MEM_DECOMMIT) != 0);
#else
		VERIFY(mprotect(pointer, size, PROT_NONE) != -1);
#endif
	}
}
