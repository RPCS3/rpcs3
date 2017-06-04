#include "virtual_memory.h"
#include "fmt.h"
#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#endif

namespace common
{
	namespace vm
	{
		void* map(size_t size)
		{
#ifdef _WIN32
			return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
#else
			return mmap(nullptr, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
		}

		void commit_page(void* pointer, size_t page_size)
		{
#ifdef _WIN32
			if (VirtualAlloc(pointer, page_size, MEM_COMMIT, PAGE_READWRITE) == nullptr)
#else
			if (mprotect((unsigned char*)pointer, page_size, PROT_READ | PROT_WRITE) != -1)
#endif
			{
				throw std::runtime_error(fmt::format("commit page at %p failture", pointer));
			}
		}

		void unmap(void* pointer, size_t size)
		{
#ifdef _WIN32
			if (VirtualFree(pointer, 0, MEM_RELEASE) != 0)
#else
			if (munmap(pointer, size) == 0)
#endif
			{
				throw std::runtime_error(fmt::format("unmap at %p failture", pointer));
			}
		}
	}
}
