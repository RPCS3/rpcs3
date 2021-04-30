#include "stdafx.h"
#include "util/logs.hpp"
#include "util/vm.hpp"
#include "util/asm.hpp"
#ifdef _WIN32
#include "Utilities/File.h"
#include "util/dyn_lib.hpp"
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#endif

#ifdef __linux__
#include <sys/syscall.h>
#include <linux/memfd.h>

#ifdef __NR_memfd_create
#elif __x86_64__
#define __NR_memfd_create 319
#elif __aarch64__
#define __NR_memfd_create 279
#endif

static int memfd_create_(const char *name, uint flags)
{
	return syscall(__NR_memfd_create, name, flags);
}
#endif

namespace utils
{
#ifdef MAP_NORESERVE
	constexpr int c_map_noreserve = MAP_NORESERVE;
#else
	constexpr int c_map_noreserve = 0;
#endif

#ifdef MADV_FREE
	[[maybe_unused]] constexpr int c_madv_free = MADV_FREE;
#elif defined(MADV_DONTNEED)
	[[maybe_unused]] constexpr int c_madv_free = MADV_DONTNEED;
#else
	[[maybe_unused]] constexpr int c_madv_free = 0;
#endif

#ifdef MADV_HUGEPAGE
	constexpr int c_madv_hugepage = MADV_HUGEPAGE;
#else
	constexpr int c_madv_hugepage = 0;
#endif

#if defined(MADV_DONTDUMP) && defined(MADV_DODUMP)
	constexpr int c_madv_no_dump = MADV_DONTDUMP;
	constexpr int c_madv_dump = MADV_DODUMP;
#elif defined(MADV_NOCORE) && defined(MADV_CORE)
	constexpr int c_madv_no_dump = MADV_NOCORE;
	constexpr int c_madv_dump = MADV_CORE;
#else
	constexpr int c_madv_no_dump = 0;
	constexpr int c_madv_dump = 0;
#endif

#if defined(MFD_HUGETLB) && defined(MFD_HUGE_2MB)
	constexpr int c_mfd_huge_2mb = MFD_HUGETLB | MFD_HUGE_2MB;
#else
	constexpr int c_mfd_huge_2mb = 0;
#endif

#ifdef _WIN32
	DYNAMIC_IMPORT("KernelBase.dll", VirtualAlloc2, PVOID(HANDLE Process, PVOID Base, SIZE_T Size, ULONG AllocType, ULONG Prot, MEM_EXTENDED_PARAMETER*, ULONG));
	DYNAMIC_IMPORT("KernelBase.dll", MapViewOfFile3, PVOID(HANDLE Handle, HANDLE Process, PVOID Base, ULONG64 Off, SIZE_T ViewSize, ULONG AllocType, ULONG Prot, MEM_EXTENDED_PARAMETER*, ULONG));
#endif

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

	void* memory_reserve(usz size, void* use_addr)
	{
#ifdef _WIN32
		return ::VirtualAlloc(use_addr, size, MEM_RESERVE, PAGE_NOACCESS);
#else
		if (use_addr && reinterpret_cast<uptr>(use_addr) % 0x10000)
		{
			return nullptr;
		}

		const auto orig_size = size;

		if (!use_addr)
		{
			// Hack: Ensure aligned 64k allocations
			size += 0x10000;
		}

		auto ptr = ::mmap(use_addr, size, PROT_NONE, MAP_ANON | MAP_PRIVATE | c_map_noreserve, -1, 0);

		if (ptr == reinterpret_cast<void*>(UINT64_MAX))
		{
			return nullptr;
		}

		if (use_addr && ptr != use_addr)
		{
			::munmap(ptr, size);
			return nullptr;
		}

		if (!use_addr && ptr)
		{
			// Continuation of the hack above
			const auto misalign = reinterpret_cast<uptr>(ptr) % 0x10000;
			::munmap(ptr, 0x10000 - misalign);

			if (misalign)
			{
				::munmap(static_cast<u8*>(ptr) + size - misalign, misalign);
			}

			ptr = static_cast<u8*>(ptr) + (0x10000 - misalign);
		}

		if constexpr (c_madv_hugepage != 0)
		{
			if (orig_size % 0x200000 == 0)
			{
				::madvise(ptr, orig_size, c_madv_hugepage);
			}
		}

		if constexpr (c_madv_no_dump != 0)
		{
			ensure(::madvise(ptr, orig_size, c_madv_no_dump) != -1);
		}
		else
		{
			ensure(::madvise(ptr, orig_size, c_madv_free) != -1);
		}

		return ptr;
#endif
	}

	void memory_commit(void* pointer, usz size, protection prot)
	{
#ifdef _WIN32
		ensure(::VirtualAlloc(pointer, size, MEM_COMMIT, +prot));
#else
		const u64 ptr64 = reinterpret_cast<u64>(pointer);
		ensure(::mprotect(reinterpret_cast<void*>(ptr64 & -4096), size + (ptr64 & 4095), +prot) != -1);

		if constexpr (c_madv_dump != 0)
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -4096), size + (ptr64 & 4095), c_madv_dump) != -1);
		}
		else
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -4096), size + (ptr64 & 4095), MADV_WILLNEED) != -1);
		}
#endif
	}

	void memory_decommit(void* pointer, usz size)
	{
#ifdef _WIN32
		ensure(::VirtualFree(pointer, size, MEM_DECOMMIT));
#else
		const u64 ptr64 = reinterpret_cast<u64>(pointer);
		ensure(::mmap(pointer, size, PROT_NONE, MAP_FIXED | MAP_ANON | MAP_PRIVATE | c_map_noreserve, -1, 0) != reinterpret_cast<void*>(UINT64_MAX));

		if constexpr (c_madv_no_dump != 0)
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -4096), size + (ptr64 & 4095), c_madv_no_dump) != -1);
		}
		else
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -4096), size + (ptr64 & 4095), c_madv_free) != -1);
		}
#endif
	}

	void memory_reset(void* pointer, usz size, protection prot)
	{
#ifdef _WIN32
		memory_decommit(pointer, size);
		memory_commit(pointer, size, prot);
#else
		const u64 ptr64 = reinterpret_cast<u64>(pointer);
		ensure(::mmap(pointer, size, +prot, MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0) != reinterpret_cast<void*>(UINT64_MAX));

		if constexpr (c_madv_hugepage != 0)
		{
			if (size % 0x200000 == 0)
			{
				::madvise(reinterpret_cast<void*>(ptr64 & -4096), size + (ptr64 & 4095), c_madv_hugepage);
			}
		}

		if constexpr (c_madv_dump != 0)
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -4096), size + (ptr64 & 4095), c_madv_dump) != -1);
		}
		else
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -4096), size + (ptr64 & 4095), MADV_WILLNEED) != -1);
		}
#endif
	}

	void memory_release(void* pointer, usz size)
	{
#ifdef _WIN32
		ensure(::VirtualFree(pointer, 0, MEM_RELEASE));
#else
		ensure(::munmap(pointer, size) != -1);
#endif
	}

	void memory_protect(void* pointer, usz size, protection prot)
	{
#ifdef _WIN32

		DWORD old;
		if (::VirtualProtect(pointer, size, +prot, &old))
		{
			return;
		}

		for (u64 addr = reinterpret_cast<u64>(pointer), end = addr + size; addr < end;)
		{
			const u64 boundary = (addr + 0x10000) & -0x10000;
			const u64 block_size = std::min(boundary, end) - addr;

			if (!::VirtualProtect(reinterpret_cast<LPVOID>(addr), block_size, +prot, &old))
			{
				fmt::throw_exception("VirtualProtect failed (%p, 0x%x, addr=0x%x, error=%#x)", pointer, size, addr, GetLastError());
			}

			// Next region
			addr += block_size;
		}
#else
		const u64 ptr64 = reinterpret_cast<u64>(pointer);
		ensure(::mprotect(reinterpret_cast<void*>(ptr64 & -4096), size + (ptr64 & 4095), +prot) != -1);
#endif
	}

	bool memory_lock(void* pointer, usz size)
	{
#ifdef _WIN32
		return ::VirtualLock(pointer, size);
#else
		return !::mlock(pointer, size);
#endif
	}

	shm::shm(u32 size, u32 flags)
		: m_flags(flags)
		, m_size(utils::align(size, 0x10000))
	{
#ifdef _WIN32
		m_handle = ensure(::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_EXECUTE_READWRITE, 0, m_size, nullptr));
#elif __linux__
		m_file = -1;

		// Try to use 2MB pages for 2M-aligned shm
		if constexpr (c_mfd_huge_2mb != 0)
		{
			if (m_size % 0x200000 == 0 && flags & 2)
			{
				m_file = ::memfd_create_("2M", c_mfd_huge_2mb);
			}
		}

		if (m_file == -1)
		{
			m_file = ::memfd_create_("", 0);
		}

		ensure(m_file >= 0);
		ensure(::ftruncate(m_file, m_size) >= 0);
#else
		const std::string name = "/rpcs3-mem-" + std::to_string(reinterpret_cast<u64>(this));

		while ((m_file = ::shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR)) == -1)
		{
			if (errno == EMFILE)
			{
				fmt::throw_exception("Too many open files. Raise the limit and try again.");
			}

			ensure(errno == EEXIST);
		}

		ensure(::shm_unlink(name.c_str()) >= 0);
		ensure(::ftruncate(m_file, m_size) >= 0);
#endif
	}

	shm::shm(u64 size, const std::string& storage)
		: m_size(utils::align(size, 0x10000))
	{
#ifdef _WIN32
		fs::file f = ensure(fs::file(storage, fs::read + fs::rewrite));
		FILE_DISPOSITION_INFO disp{ .DeleteFileW = true };
		ensure(SetFileInformationByHandle(f.get_handle(), FileDispositionInfo, &disp, sizeof(disp)));
		ensure(DeviceIoControl(f.get_handle(), FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, nullptr, nullptr));
		ensure(f.trunc(m_size));
		m_handle = ensure(::CreateFileMappingW(f.get_handle(), nullptr, PAGE_READWRITE, 0, 0, nullptr));
#else
		m_file = ::open(storage.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
		ensure(m_file >= 0);
		ensure(::ftruncate(m_file, m_size) >= 0);
		::unlink(storage.c_str());
#endif
	}

	shm::~shm()
	{
		this->unmap_self();

#ifdef _WIN32
		::CloseHandle(m_handle);
#else
		::close(m_file);
#endif
	}

	u8* shm::map(void* ptr, protection prot, bool cow) const
	{
#ifdef _WIN32
		DWORD access = FILE_MAP_WRITE;
		switch (prot)
		{
		case protection::rw:
		case protection::ro:
		case protection::no:
			break;
		case protection::wx:
		case protection::rx:
			access |= FILE_MAP_EXECUTE;
			break;
		}

		if (cow)
		{
			access |= FILE_MAP_COPY;
		}

		if (auto ret = static_cast<u8*>(::MapViewOfFileEx(m_handle, access, 0, 0, m_size, ptr)))
		{
			if (prot != protection::rw && prot != protection::wx)
			{
				DWORD old;
				if (!::VirtualProtect(ret, m_size, +prot, &old))
				{
					::UnmapViewOfFile(ret);
					return nullptr;
				}
			}

			return ret;
		}

		return nullptr;
#else
		const u64 ptr64 = reinterpret_cast<u64>(ptr) & -0x10000;

		if (ptr64)
		{
			const auto result = ::mmap(reinterpret_cast<void*>(ptr64), m_size, +prot, (cow ? MAP_PRIVATE : MAP_SHARED) | MAP_FIXED, m_file, 0);

			return reinterpret_cast<u8*>(result);
		}
		else
		{
			const u64 res64 = reinterpret_cast<u64>(::mmap(reinterpret_cast<void*>(ptr64), m_size + 0xf000, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0));

			const u64 aligned = utils::align(res64, 0x10000);
			const auto result = ::mmap(reinterpret_cast<void*>(aligned), m_size, +prot, (cow ? MAP_PRIVATE : MAP_SHARED) | MAP_FIXED, m_file, 0);

			// Now cleanup remnants
			if (aligned > res64)
			{
				ensure(::munmap(reinterpret_cast<void*>(res64), aligned - res64) == 0);
			}

			if (aligned < res64 + 0xf000)
			{
				ensure(::munmap(reinterpret_cast<void*>(aligned + m_size), (res64 + 0xf000) - (aligned)) == 0);
			}

			return reinterpret_cast<u8*>(result);
		}
#endif
	}

	u8* shm::try_map(void* ptr, protection prot, bool cow) const
	{
		// Non-null pointer shall be specified
		const auto target = ensure(reinterpret_cast<u8*>(reinterpret_cast<u64>(ptr) & -0x10000));

#ifdef _WIN32
		return this->map(target, prot, cow);
#else
		const auto result = reinterpret_cast<u8*>(::mmap(reinterpret_cast<void*>(target), m_size, +prot, (cow ? MAP_PRIVATE : MAP_SHARED), m_file, 0));

		if (result == reinterpret_cast<void*>(UINT64_MAX))
		{
			[[unlikely]] return nullptr;
		}

		return result;
#endif
	}

	u8* shm::map_critical(void* ptr, protection prot, bool cow)
	{
		const auto target = reinterpret_cast<u8*>(reinterpret_cast<u64>(ptr) & -0x10000);

#ifdef _WIN32
		::MEMORY_BASIC_INFORMATION mem;
		if (!::VirtualQuery(target, &mem, sizeof(mem)) || mem.State != MEM_RESERVE || !::VirtualFree(mem.AllocationBase, 0, MEM_RELEASE))
		{
			return nullptr;
		}

		const auto base = (u8*)mem.AllocationBase;
		const auto size = mem.RegionSize + (target - base);

		if (base < target && !::VirtualAlloc(base, target - base, MEM_RESERVE, PAGE_NOACCESS))
		{
			return nullptr;
		}

		if (target + m_size < base + size && !::VirtualAlloc(target + m_size, base + size - target - m_size, MEM_RESERVE, PAGE_NOACCESS))
		{
			return nullptr;
		}
#endif

		return this->map(target, prot, cow);
	}

	u8* shm::map_self(protection prot)
	{
		void* ptr = m_ptr;

		if (!ptr)
		{
			const auto mapped = this->map(nullptr, prot);

			// Install mapped memory
			if (!m_ptr.compare_exchange(ptr, mapped))
			{
				// Mapped already, nothing to do.
				this->unmap(mapped);
			}
			else
			{
				ptr = mapped;
			}
		}

		return static_cast<u8*>(ptr);
	}

	void shm::unmap(void* ptr) const
	{
#ifdef _WIN32
		::UnmapViewOfFile(ptr);
#else
		::munmap(ptr, m_size);
#endif
	}

	void shm::unmap_critical(void* ptr)
	{
		const auto target = reinterpret_cast<u8*>(reinterpret_cast<u64>(ptr) & -0x10000);

#ifdef _WIN32
		this->unmap(target);

		::MEMORY_BASIC_INFORMATION mem, mem2;
		if (!::VirtualQuery(target - 1, &mem, sizeof(mem)) || !::VirtualQuery(target + m_size, &mem2, sizeof(mem2)))
		{
			return;
		}

		if (mem.State == MEM_RESERVE && !::VirtualFree(mem.AllocationBase, 0, MEM_RELEASE))
		{
			return;
		}

		if (mem2.State == MEM_RESERVE && !::VirtualFree(mem2.AllocationBase, 0, MEM_RELEASE))
		{
			return;
		}

		const auto size1 = mem.State == MEM_RESERVE ? target - (u8*)mem.AllocationBase : 0;
		const auto size2 = mem2.State == MEM_RESERVE ? mem2.RegionSize : 0;

		if (!::VirtualAlloc(mem.State == MEM_RESERVE ? mem.AllocationBase : target, m_size + size1 + size2, MEM_RESERVE, PAGE_NOACCESS))
		{
			return;
		}
#else
		// This method is faster but leaves mapped remnants of the shm (until overwritten)
		ensure(::mprotect(target, m_size, PROT_NONE) != -1);
#endif
	}

	void shm::unmap_self()
	{
		if (auto ptr = m_ptr.exchange(nullptr))
		{
			this->unmap(ptr);
		}
	}
}
