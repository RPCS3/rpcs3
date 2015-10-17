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
		return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
#else
		return mmap(nullptr, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
	}

	s32 commit_page_memory(void* pointer, size_t page_size)
	{
#ifdef _WIN32
		if (VirtualAlloc((u8*)pointer, page_size, MEM_COMMIT, PAGE_READWRITE) == NULL)
		{
			LOG_ERROR(HLE, "commit_page_memory VirtualAlloc failed.");
			return -1;
		}
#else
		s32 ret = mprotect((u8*)pointer, page_size, PROT_READ | PROT_WRITE)
		if (ret < 0)
		{
			LOG_ERROR(HLE, "commit_page_memory mprotect failed. (%d)", ret);
			return -1;
		}
#endif
		return 0;
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
