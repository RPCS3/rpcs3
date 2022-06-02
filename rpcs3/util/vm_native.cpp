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

#if defined(__FreeBSD__)
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#endif

#ifdef __linux__
#include <sys/syscall.h>
#include <linux/memfd.h>

#ifdef __NR_memfd_create
#elif __x86_64__
#define __NR_memfd_create 319
#elif ARCH_ARM64
#define __NR_memfd_create 279
#endif

static int memfd_create_(const char *name, uint flags)
{
	return syscall(__NR_memfd_create, name, flags);
}
#elif defined(__FreeBSD__)
# if __FreeBSD__ < 13
// XXX Drop after FreeBSD 12.* reaches EOL on 2024-06-30
#define MFD_CLOEXEC O_CLOEXEC
#define memfd_create_(name, flags) shm_open(SHM_ANON, O_RDWR | flags, 0600)
# else
#define memfd_create_ memfd_create
# endif
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

	long get_page_size()
	{
		static const long r = []() -> long
		{
#ifdef _WIN32
			SYSTEM_INFO info;
			::GetSystemInfo(&info);
			return info.dwPageSize;
#else
			return ::sysconf(_SC_PAGESIZE);
#endif
		}();

		ensure(r > 0 && r <= 0x10000);

		return r;
	}

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

		if (ptr == reinterpret_cast<void*>(uptr{umax}))
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
		ensure(::mprotect(reinterpret_cast<void*>(ptr64 & -c_page_size), size + (ptr64 & (c_page_size - 1)), +prot) != -1);

		if constexpr (c_madv_dump != 0)
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -c_page_size), size + (ptr64 & (c_page_size - 1)), c_madv_dump) != -1);
		}
		else
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -c_page_size), size + (ptr64 & (c_page_size - 1)), MADV_WILLNEED) != -1);
		}
#endif
	}

	void memory_decommit(void* pointer, usz size)
	{
#ifdef _WIN32
		ensure(::VirtualFree(pointer, size, MEM_DECOMMIT));
#else
		const u64 ptr64 = reinterpret_cast<u64>(pointer);
		ensure(::mmap(pointer, size, PROT_NONE, MAP_FIXED | MAP_ANON | MAP_PRIVATE | c_map_noreserve, -1, 0) != reinterpret_cast<void*>(uptr{umax}));

		if constexpr (c_madv_no_dump != 0)
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -c_page_size), size + (ptr64 & (c_page_size - 1)), c_madv_no_dump) != -1);
		}
		else
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -c_page_size), size + (ptr64 & (c_page_size - 1)), c_madv_free) != -1);
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
		ensure(::mmap(pointer, size, +prot, MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0) != reinterpret_cast<void*>(uptr{umax}));

		if constexpr (c_madv_hugepage != 0)
		{
			if (size % 0x200000 == 0)
			{
				::madvise(reinterpret_cast<void*>(ptr64 & -c_page_size), size + (ptr64 & (c_page_size - 1)), c_madv_hugepage);
			}
		}

		if constexpr (c_madv_dump != 0)
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -c_page_size), size + (ptr64 & (c_page_size - 1)), c_madv_dump) != -1);
		}
		else
		{
			ensure(::madvise(reinterpret_cast<void*>(ptr64 & -c_page_size), size + (ptr64 & (c_page_size - 1)), MADV_WILLNEED) != -1);
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
		ensure(::mprotect(reinterpret_cast<void*>(ptr64 & -c_page_size), size + (ptr64 & (c_page_size - 1)), +prot) != -1);
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

	void* memory_map_fd(native_handle fd, usz size, protection prot)
	{
#ifdef _WIN32
		// TODO
		return nullptr;
#else
		const auto result = ::mmap(nullptr, size, +prot, MAP_SHARED, fd, 0);

		if (result == reinterpret_cast<void*>(uptr{umax}))
		{
			[[unlikely]] return nullptr;
		}

		return result;
#endif
	}

	shm::shm(u32 size, u32 flags)
		: m_flags(flags)
		, m_size(utils::align(size, 0x10000))
	{
#ifdef _WIN32
		m_handle = ensure(::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_EXECUTE_READWRITE, 0, m_size, nullptr));
#elif defined(__linux__) || defined(__FreeBSD__)
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
		fs::file f;

		// Get system version
		[[maybe_unused]] static const DWORD version_major = *reinterpret_cast<const DWORD*>(__readgsqword(0x60) + 0x118);

		auto set_sparse = [](HANDLE h, usz m_size) -> bool
		{
			FILE_SET_SPARSE_BUFFER arg{.SetSparse = true};
			FILE_BASIC_INFO info0{};
			ensure(GetFileInformationByHandleEx(h, FileBasicInfo, &info0, sizeof(info0)));

			if ((info0.FileAttributes & FILE_ATTRIBUTE_ARCHIVE) || (~info0.FileAttributes & FILE_ATTRIBUTE_TEMPORARY))
			{
				info0.FileAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;
				info0.FileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
				ensure(SetFileInformationByHandle(h, FileBasicInfo, &info0, sizeof(info0)));
			}

			if ((info0.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) == 0 && version_major <= 7)
			{
				MessageBoxW(0, L"RPCS3 needs to be restarted to create sparse file rpcs3_vm.", L"RPCS3", MB_ICONEXCLAMATION);
			}

			if (DWORD bytesReturned{}; (info0.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) || DeviceIoControl(h, FSCTL_SET_SPARSE, &arg, sizeof(arg), nullptr, 0, &bytesReturned, nullptr))
			{
				if ((info0.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) == 0 && version_major <= 7)
				{
					std::abort();
				}

				FILE_STANDARD_INFO info;
				FILE_END_OF_FILE_INFO _eof{};
				ensure(GetFileInformationByHandleEx(h, FileStandardInfo, &info, sizeof(info)));
				ensure(GetFileSizeEx(h, &_eof.EndOfFile));

				if (info.AllocationSize.QuadPart && _eof.EndOfFile.QuadPart == m_size)
				{
					// Truncate file since it may be dirty (fool-proof)
					DWORD ret = 0;
					FILE_ALLOCATED_RANGE_BUFFER dummy{};
					dummy.Length.QuadPart = m_size;

					if (!DeviceIoControl(h, FSCTL_QUERY_ALLOCATED_RANGES, &dummy, sizeof(dummy), nullptr, 0, &ret, 0) || ret)
					{
						_eof.EndOfFile.QuadPart = 0;
					}
				}

				if (_eof.EndOfFile.QuadPart != m_size)
				{
					// Reset file size to 0 if it doesn't match
					_eof.EndOfFile.QuadPart = 0;
					ensure(SetFileInformationByHandle(h, FileEndOfFileInfo, &_eof, sizeof(_eof)));
				}

				return true;
			}

			return false;
		};

		const std::string storage2 = fs::get_temp_dir() + "rpcs3_vm_sparse.tmp";
		const std::string storage3 = fs::get_cache_dir() + "rpcs3_vm_sparse.tmp";

		if (!storage.empty())
		{
			// Explicitly specified storage
			ensure(f.open(storage, fs::read + fs::write + fs::create));
		}
		else if (!f.open(storage2, fs::read + fs::write + fs::create) || !set_sparse(f.get_handle(), m_size))
		{
			// Fallback storage
			ensure(f.open(storage3, fs::read + fs::write + fs::create));
		}
		else
		{
			goto check;
		}

		if (!set_sparse(f.get_handle(), m_size))
		{
			MessageBoxW(0, L"Failed to initialize sparse file.\nCan't find a filesystem with sparse file support (NTFS).", L"RPCS3", MB_ICONERROR);
		}

	check:
		if (f.size() != m_size)
		{
			// Resize the file gradually (bug workaround)
			for (usz i = 0; i < m_size / (1024 * 1024 * 256); i++)
			{
				ensure(f.trunc((i + 1) * (1024 * 1024 * 256)));
			}

			ensure(f.trunc(m_size));
		}

		m_handle = ensure(::CreateFileMappingW(f.get_handle(), nullptr, PAGE_READWRITE, 0, 0, nullptr));
#else

#ifdef __linux__
		if (const char c = fs::file("/proc/sys/vm/overcommit_memory").read<char>(); c == '0' || c == '1')
		{
			// Simply use memfd for overcommit memory
			m_file = ::memfd_create_("", 0);
			ensure(m_file >= 0);
			ensure(::ftruncate(m_file, m_size) >= 0);
			return;
		}
		else
		{
			fprintf(stderr, "Reading /proc/sys/vm/overcommit_memory: %c", c);
		}
#else
		int vm_overcommit = 0;
		auto vm_sz = sizeof(int);

#if defined(__NetBSD__) || defined(__APPLE__)
		// Always ON
		vm_overcommit = 0;
#elif defined(__FreeBSD__)
		int mib[2]{CTL_VM, VM_OVERCOMMIT};
		if (::sysctl(mib, 2, &vm_overcommit, &vm_sz, NULL, 0) != 0)
			vm_overcommit = -1;
#else
		vm_overcommit = -1;
#endif

		if ((vm_overcommit & 3) == 0)
		{
#if defined(__FreeBSD__)
			m_file = ::memfd_create_("", 0);
			ensure(m_file >= 0);
#else
			const std::string name = "/rpcs3-mem2-" + std::to_string(reinterpret_cast<u64>(this));

			while ((m_file = ::shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR)) == -1)
			{
				if (errno == EMFILE)
				{
					fmt::throw_exception("Too many open files. Raise the limit and try again.");
				}

				ensure(errno == EEXIST);
			}

			ensure(::shm_unlink(name.c_str()) >= 0);
#endif
			ensure(::ftruncate(m_file, m_size) >= 0);
			return;
		}
#endif

		if (!storage.empty())
		{
			m_file = ::open(storage.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
		}
		else
		{
			m_file = ::open((fs::get_cache_dir() + "rpcs3_vm_sparse.tmp").c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
		}

		ensure(m_file >= 0);
		struct ::stat stats;
		ensure(::fstat(m_file, &stats) >= 0);

		if (!(stats.st_size ^ m_size) && !stats.st_blocks)
		{
			// Already initialized
			return;
		}

		// Truncate file since it may be dirty (fool-proof)
		ensure(::ftruncate(m_file, 0) >= 0);
		ensure(::ftruncate(m_file, 0x100000) >= 0);
		stats.st_size = 0x100000;

#ifdef SEEK_DATA
		errno = EINVAL;
		if (stats.st_blocks * 512 >= 0x100000 && ::lseek(m_file, 0, SEEK_DATA) ^ stats.st_size && errno != ENXIO)
		{
			fmt::throw_exception("Failed to initialize sparse file in '%s'\n"
				"It seems this filesystem doesn't support sparse files (%d).\n",
				storage.empty() ? fs::get_cache_dir().c_str() : storage.c_str(), +errno);
		}
#endif

		if (stats.st_size ^ m_size)
		{
			// Fix file size
			ensure(::ftruncate(m_file, m_size) >= 0);
		}
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

		if (result == reinterpret_cast<void*>(uptr{umax}))
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
