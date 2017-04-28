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

namespace utils
{
	// Convert memory protection (internal)
	static auto operator +(protection prot)
	{
#ifdef _WIN32
		DWORD _prot = PAGE_NOACCESS;
		switch (prot)
		{
		case protection::rw: _prot = PAGE_READWRITE; break;
		case protection::ro: _prot = PAGE_READONLY; break;
		case protection::no: break;
		case protection::wx: _prot = PAGE_EXECUTE_READWRITE; break;
		case protection::rx: _prot = PAGE_EXECUTE_READ; break;
		}
#else
		int _prot = PROT_NONE;
		switch (prot)
		{
		case protection::rw: _prot = PROT_READ | PROT_WRITE; break;
		case protection::ro: _prot = PROT_READ; break;
		case protection::no: break;
		case protection::wx: _prot = PROT_READ | PROT_WRITE | PROT_EXEC; break;
		case protection::rx: _prot = PROT_READ | PROT_EXEC; break;
		}
#endif

		return _prot;
	}

	void* memory_reserve(std::size_t size, void* use_addr)
	{
#ifdef _WIN32
		return ::VirtualAlloc(use_addr, size, MEM_RESERVE, PAGE_NOACCESS);
#else
		auto ptr = ::mmap(use_addr, size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);

		if (use_addr && ptr != use_addr)
		{
			::munmap(ptr, size);
			return nullptr;
		}

		return ptr;
#endif
	}

	void memory_commit(void* pointer, std::size_t size, protection prot)
	{
#ifdef _WIN32
		verify(HERE), ::VirtualAlloc(pointer, size, MEM_COMMIT, +prot);
#else
		verify(HERE), ::mprotect((void*)((u64)pointer & -4096), ::align(size, 4096), +prot) != -1;
#endif
	}

	void memory_decommit(void* pointer, std::size_t size)
	{
#ifdef _WIN32
		verify(HERE), ::VirtualFree(pointer, 0, MEM_DECOMMIT);
#else
		verify(HERE), ::mmap(pointer, size, PROT_NONE, MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
	}

	void memory_protect(void* pointer, std::size_t size, protection prot)
	{
#ifdef _WIN32
		DWORD old;
		verify(HERE), ::VirtualProtect(pointer, size, +prot, &old);
#else
		verify(HERE), ::mprotect((void*)((u64)pointer & -4096), ::align(size, 4096), +prot) != -1;
#endif
	}
}
