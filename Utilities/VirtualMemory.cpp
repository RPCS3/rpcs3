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
		if (ret == NULL)
		{
			LOG_ERROR(HLE, "reserve_memory VirtualAlloc failed.");
			return (void*)VM_FAILURE;
		}
#else
		void* ret = mmap(nullptr, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
		if (ret == (void*)VM_FAILURE)
		{
			LOG_ERROR(HLE, "reserve_memory mmap failed.");
		}
#endif
		return ret;
	}

	s32 commit_page_memory(void* pointer, size_t page_size)
	{
#ifdef _WIN32
		if (VirtualAlloc((u8*)pointer, page_size, MEM_COMMIT, PAGE_READWRITE) == NULL)
		{
			LOG_ERROR(HLE, "commit_page_memory VirtualAlloc failed.");
			return VM_FAILURE;
		}
#else
		s32 ret = mprotect((u8*)pointer, page_size, PROT_READ | PROT_WRITE);
		if (ret < VM_SUCCESS)
		{
			LOG_ERROR(HLE, "commit_page_memory mprotect failed. (%d)", ret);
			return VM_FAILURE;
		}
#endif
		return VM_SUCCESS;
	}

	s32 free_reserved_memory(void* pointer, size_t size)
	{
#ifdef _WIN32
		if (VirtualFree(pointer, 0, MEM_RELEASE) == 0)
		{
			LOG_ERROR(HLE, "free_reserved_memory VirtualFree failed.");
			return VM_FAILURE;
		}
#else
		if (munmap(pointer, size) != VM_SUCCESS)
		{
			LOG_ERROR(HLE, "free_reserved_memory munmap failed.");
			return VM_FAILURE;
		}
#endif
		return VM_SUCCESS;
	}
}
