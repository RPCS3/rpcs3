#include "stdafx.h"
#include "Memory.h"
#include "Emu/System.h"
#include "Utilities/Thread.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/PSP2/ARMv7Thread.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

/* OS X uses MAP_ANON instead of MAP_ANONYMOUS */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

#include "wait_engine.h"

namespace vm
{
	thread_local u64 g_tls_fault_count{};

	template<std::size_t Size> struct mapped_ptr_deleter
	{
		void operator ()(void* ptr)
		{
#ifdef _WIN32
			::UnmapViewOfFile(ptr);
#else
			::munmap(ptr, Size);
#endif
		}
	};

	using mapped_ptr_t = std::unique_ptr<u8[], mapped_ptr_deleter<0x100000000>>;

	std::array<mapped_ptr_t, 2> initialize()
	{
#ifdef _WIN32
		const HANDLE memory_handle = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_RESERVE, 0x1, 0x0, NULL);

		if (memory_handle == NULL)
		{
			MessageBoxA(0, fmt::format("CreateFileMapping() failed (0x%x).", GetLastError()).c_str(), "vm::initialize()", MB_ICONERROR);
			std::abort();
		}

		mapped_ptr_t base_addr(static_cast<u8*>(::MapViewOfFile(memory_handle, FILE_MAP_WRITE, 0, 0, 0x100000000)));
		mapped_ptr_t priv_addr(static_cast<u8*>(::MapViewOfFile(memory_handle, FILE_MAP_WRITE, 0, 0, 0x100000000)));

		::CloseHandle(memory_handle);
#else
		const int memory_handle = ::shm_open("/rpcs3_vm", O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

		if (memory_handle == -1)
		{
			std::printf("shm_open('/rpcs3_vm') failed (%d).\n", errno);
			std::abort();
		}

		if (::ftruncate(memory_handle, 0x100000000) == -1)
		{
			std::printf("ftruncate(memory_handle) failed (%d).\n", errno);
			::shm_unlink("/rpcs3_vm");
			std::abort();
		}

		mapped_ptr_t base_addr(static_cast<u8*>(::mmap(nullptr, 0x100000000, PROT_NONE, MAP_SHARED, memory_handle, 0)));
		mapped_ptr_t priv_addr(static_cast<u8*>(::mmap(nullptr, 0x100000000, PROT_NONE, MAP_SHARED, memory_handle, 0)));

		::shm_unlink("/rpcs3_vm");
#endif

		std::printf("vm::g_base_addr = %p\nvm::g_priv_addr = %p\n", base_addr.get(), priv_addr.get());

		return{ std::move(base_addr), std::move(priv_addr) };
	}

	const auto g_addr_set = vm::initialize();

	u8* const g_base_addr = g_addr_set[0].get();
	u8* const g_priv_addr = g_addr_set[1].get();

	std::array<atomic_t<u8>, 0x100000000ull / 4096> g_pages{}; // information about every page

	std::vector<std::shared_ptr<block_t>> g_locations; // memory locations

	access_violation::access_violation(u64 addr, const char* cause)
		: std::runtime_error(fmt::exception("Access violation %s address 0x%llx", cause, addr))
	{
		g_tls_fault_count &= ~(1ull << 63);
	}

	using reservation_mutex_t = std::mutex;

	thread_ctrl* volatile g_reservation_owner = nullptr;

	u32 g_reservation_addr = 0;
	u32 g_reservation_size = 0;

	thread_local bool g_tls_did_break_reservation = false;

	reservation_mutex_t g_reservation_mutex;

	void _reservation_set(u32 addr, bool no_access = false)
	{
#ifdef _WIN32
		DWORD old;
		if (!::VirtualProtect(vm::base(addr & ~0xfff), 4096, no_access ? PAGE_NOACCESS : PAGE_READONLY, &old))
#else
		if (::mprotect(vm::base(addr & ~0xfff), 4096, no_access ? PROT_NONE : PROT_READ))
#endif
		{
			throw EXCEPTION("System failure (addr=0x%x)", addr);
		}
	}

	bool _reservation_break(u32 addr)
	{
		if (g_reservation_addr >> 12 == addr >> 12)
		{
#ifdef _WIN32
			DWORD old;
			if (!::VirtualProtect(vm::base(addr & ~0xfff), 4096, PAGE_READWRITE, &old))
#else
			if (::mprotect(vm::base(addr & ~0xfff), 4096, PROT_READ | PROT_WRITE))
#endif
			{
				throw EXCEPTION("System failure (addr=0x%x)", addr);
			}

			g_reservation_addr = 0;
			g_reservation_size = 0;
			g_reservation_owner = nullptr;
			
			return true;
		}

		return false;
	}

	void reservation_break(u32 addr)
	{
		std::unique_lock<reservation_mutex_t> lock(g_reservation_mutex);

		const u32 raddr = g_reservation_addr;
		const u32 rsize = g_reservation_size;

		if ((g_tls_did_break_reservation = _reservation_break(addr)))
		{
			lock.unlock(), vm::notify_at(raddr, rsize);
		}
	}

	void reservation_acquire(void* data, u32 addr, u32 size)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		const u64 align = 0x80000000ull >> cntlz32(size);

		if (!size || !addr || size > 4096 || size != align || addr & (align - 1))
		{
			throw EXCEPTION("Invalid arguments (addr=0x%x, size=0x%x)", addr, size);
		}

		const u8 flags = g_pages[addr >> 12];

		if (!(flags & page_writable) || !(flags & page_allocated) || (flags & page_no_reservations))
		{
			throw EXCEPTION("Invalid page flags (addr=0x%x, size=0x%x, flags=0x%x)", addr, size, flags);
		}

		// break the reservation
		g_tls_did_break_reservation = g_reservation_owner && _reservation_break(g_reservation_addr);

		// change memory protection to read-only
		_reservation_set(addr);

		// may not be necessary
		_mm_mfence();

		// set additional information
		g_reservation_addr = addr;
		g_reservation_size = size;
		g_reservation_owner = thread_ctrl::get_current();

		// copy data
		std::memcpy(data, vm::base(addr), size);
	}

	bool reservation_update(u32 addr, const void* data, u32 size)
	{
		std::unique_lock<reservation_mutex_t> lock(g_reservation_mutex);

		const u64 align = 0x80000000ull >> cntlz32(size);

		if (!size || !addr || size > 4096 || size != align || addr & (align - 1))
		{
			throw EXCEPTION("Invalid arguments (addr=0x%x, size=0x%x)", addr, size);
		}

		if (g_reservation_owner != thread_ctrl::get_current() || g_reservation_addr != addr || g_reservation_size != size)
		{
			// atomic update failed
			return false;
		}

		// change memory protection to no access
		_reservation_set(addr, true);

		// update memory using privileged access
		std::memcpy(vm::base_priv(addr), data, size);

		// free the reservation and restore memory protection
		_reservation_break(addr);

		// notify waiter
		lock.unlock(), vm::notify_at(addr, size);

		// atomic update succeeded
		return true;
	}

	bool reservation_query(u32 addr, u32 size, bool is_writing, std::function<bool()> callback)
	{
		std::unique_lock<reservation_mutex_t> lock(g_reservation_mutex);

		if (!check_addr(addr))
		{
			return false;
		}

		// check if current reservation and address may overlap
		if (g_reservation_addr >> 12 == addr >> 12 && is_writing)
		{
			const bool result = callback(); 

			if (result && size && addr + size - 1 >= g_reservation_addr && g_reservation_addr + g_reservation_size - 1 >= addr)
			{
				const u32 raddr = g_reservation_addr;
				const u32 rsize = g_reservation_size;

				// break the reservation if overlap
				if ((g_tls_did_break_reservation = _reservation_break(addr)))
				{
					lock.unlock(), vm::notify_at(raddr, rsize);
				}
			}
			
			return result;
		}
		
		return true;
	}

	bool reservation_test(thread_ctrl* current)
	{
		const auto owner = g_reservation_owner;

		return owner && owner == current;
	}

	void reservation_free()
	{
		auto thread = thread_ctrl::get_current();

		if (reservation_test(thread))
		{
			std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

			if (g_reservation_owner && g_reservation_owner == thread)
			{
				g_tls_did_break_reservation = _reservation_break(g_reservation_addr);
			}
		}
	}

	void reservation_op(u32 addr, u32 size, std::function<void()> proc)
	{
		std::unique_lock<reservation_mutex_t> lock(g_reservation_mutex);

		const u64 align = 0x80000000ull >> cntlz32(size);

		if (!size || !addr || size > 4096 || size != align || addr & (align - 1))
		{
			throw EXCEPTION("Invalid arguments (addr=0x%x, size=0x%x)", addr, size);
		}

		g_tls_did_break_reservation = false;

		// check and possibly break previous reservation
		if (g_reservation_owner != thread_ctrl::get_current() || g_reservation_addr != addr || g_reservation_size != size)
		{
			if (g_reservation_owner)
			{
				_reservation_break(g_reservation_addr);
			}

			g_tls_did_break_reservation = true;
		}

		// change memory protection to no access
		_reservation_set(addr, true);

		// set additional information
		g_reservation_addr = addr;
		g_reservation_size = size;
		g_reservation_owner = thread_ctrl::get_current();

		// may not be necessary
		_mm_mfence();

		// do the operation
		proc();

		// remove the reservation
		_reservation_break(addr);

		// notify waiter
		lock.unlock(), vm::notify_at(addr, size);
	}

	void _page_map(u32 addr, u32 size, u8 flags)
	{
		if (!size || (size | addr) % 4096 || flags & page_allocated)
		{
			throw EXCEPTION("Invalid arguments (addr=0x%x, size=0x%x)", addr, size);
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (g_pages[i])
			{
				throw EXCEPTION("Memory already mapped (addr=0x%x, size=0x%x, flags=0x%x, current_addr=0x%x)", addr, size, flags, i * 4096);
			}
		}

		void* real_addr = vm::base(addr);
		void* priv_addr = vm::base_priv(addr);

#ifdef _WIN32
		auto protection = flags & page_writable ? PAGE_READWRITE : (flags & page_readable ? PAGE_READONLY : PAGE_NOACCESS);
		if (!::VirtualAlloc(priv_addr, size, MEM_COMMIT, PAGE_READWRITE) || !::VirtualAlloc(real_addr, size, MEM_COMMIT, protection))
#else
		auto protection = flags & page_writable ? PROT_WRITE | PROT_READ : (flags & page_readable ? PROT_READ : PROT_NONE);
		if (::mprotect(priv_addr, size, PROT_READ | PROT_WRITE) || ::mprotect(real_addr, size, protection))
#endif
		{
			throw EXCEPTION("System failure (addr=0x%x, size=0x%x, flags=0x%x)", addr, size, flags);
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (g_pages[i].exchange(flags | page_allocated))
			{
				throw EXCEPTION("Concurrent access (addr=0x%x, size=0x%x, flags=0x%x, current_addr=0x%x)", addr, size, flags, i * 4096);
			}
		}

		std::memset(priv_addr, 0, size); // ???
	}

	bool page_protect(u32 addr, u32 size, u8 flags_test, u8 flags_set, u8 flags_clear)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		if (!size || (size | addr) % 4096)
		{
			throw EXCEPTION("Invalid arguments (addr=0x%x, size=0x%x)", addr, size);
		}

		const u8 flags_inv = flags_set & flags_clear;

		flags_test |= page_allocated;

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if ((g_pages[i] & flags_test) != (flags_test | page_allocated))
			{
				return false;
			}
		}

		if (!flags_inv && !flags_set && !flags_clear)
		{
			return true;
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			_reservation_break(i * 4096);

			const u8 f1 = g_pages[i].fetch_or(flags_set & ~flags_inv) & (page_writable | page_readable);
			g_pages[i].fetch_and(~(flags_clear & ~flags_inv));
			const u8 f2 = (g_pages[i] ^= flags_inv) & (page_writable | page_readable);

			if (f1 != f2)
			{
				void* real_addr = vm::base(i * 4096);

#ifdef _WIN32
				DWORD old;

				auto protection = f2 & page_writable ? PAGE_READWRITE : (f2 & page_readable ? PAGE_READONLY : PAGE_NOACCESS);
				if (!::VirtualProtect(real_addr, 4096, protection, &old))
#else
				auto protection = f2 & page_writable ? PROT_WRITE | PROT_READ : (f2 & page_readable ? PROT_READ : PROT_NONE);
				if (::mprotect(real_addr, 4096, protection))
#endif
				{
					throw EXCEPTION("System failure (addr=0x%x, size=0x%x, flags_test=0x%x, flags_set=0x%x, flags_clear=0x%x)", addr, size, flags_test, flags_set, flags_clear);
				}
			}
		}

		return true;
	}

	void _page_unmap(u32 addr, u32 size)
	{
		if (!size || (size | addr) % 4096)
		{
			throw EXCEPTION("Invalid arguments (addr=0x%x, size=0x%x)", addr, size);
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if ((g_pages[i] & page_allocated) == 0)
			{
				throw EXCEPTION("Memory not mapped (addr=0x%x, size=0x%x, current_addr=0x%x)", addr, size, i * 4096);
			}
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			_reservation_break(i * 4096);

			if (!(g_pages[i].exchange(0) & page_allocated))
			{
				throw EXCEPTION("Concurrent access (addr=0x%x, size=0x%x, current_addr=0x%x)", addr, size, i * 4096);
			}
		}

		void* real_addr = vm::base(addr);
		void* priv_addr = vm::base_priv(addr);

#ifdef _WIN32
		DWORD old;

		if (!::VirtualProtect(real_addr, size, PAGE_NOACCESS, &old) || !::VirtualProtect(priv_addr, size, PAGE_NOACCESS, &old))
#else
		if (::mprotect(real_addr, size, PROT_NONE) || ::mprotect(priv_addr, size, PROT_NONE))
#endif
		{
			throw EXCEPTION("System failure (addr=0x%x, size=0x%x)", addr, size);
		}
	}

	bool check_addr(u32 addr, u32 size)
	{
		if (addr + (size - 1) < addr)
		{
			return false;
		}

		for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
		{
			if ((g_pages[i] & page_allocated) == 0)
			{
				return false;
			}
		}
		
		return true;
	}

	u32 alloc(u32 size, memory_location_t location, u32 align)
	{
		const auto block = get(location);

		if (!block)
		{
			throw EXCEPTION("Invalid memory location (%d)", location);
		}

		return block->alloc(size, align);
	}

	u32 falloc(u32 addr, u32 size, memory_location_t location)
	{
		const auto block = get(location, addr);

		if (!block)
		{
			throw EXCEPTION("Invalid memory location (%d, addr=0x%x)", location, addr);
		}

		return block->falloc(addr, size);
	}

	bool dealloc(u32 addr, memory_location_t location)
	{
		const auto block = get(location, addr);

		if (!block)
		{
			throw EXCEPTION("Invalid memory location (%d, addr=0x%x)", location, addr);
		}

		return block->dealloc(addr);
	}

	void dealloc_verbose_nothrow(u32 addr, memory_location_t location) noexcept
	{
		const auto block = get(location, addr);

		if (!block)
		{
			LOG_ERROR(MEMORY, "vm::dealloc(): invalid memory location (%d, addr=0x%x)\n", location, addr);
			return;
		}

		if (!block->dealloc(addr))
		{
			LOG_ERROR(MEMORY, "vm::dealloc(): deallocation failed (addr=0x%x)\n", addr);
			return;
		}
	}

	bool block_t::try_alloc(u32 addr, u32 size)
	{
		// check if memory area is already mapped
		for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
		{
			if (g_pages[i])
			{
				return false;
			}
		}

		// try to reserve "physical" memory
		if (!used.atomic_op([=](u32& used) -> bool
		{
			if (used > this->size)
			{
				throw EXCEPTION("Unexpected memory amount used (0x%x)", used);
			}

			if (used + size > this->size)
			{
				return false;
			}

			used += size;

			return true;
		}))
		{
			return false;
		}

		// map memory pages
		_page_map(addr, size, page_readable | page_writable);

		// add entry
		m_map[addr] = size;

		return true;
	}

	block_t::~block_t()
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		// deallocate all memory
		for (auto& entry : m_map)
		{
			_page_unmap(entry.first, entry.second);
		}
	}

	u32 block_t::alloc(u32 size, u32 align)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// align to minimal page size
		size = ::align(size, 4096);

		// check alignment (it's page allocation, so passing small values there is just silly)
		if (align < 4096 || align != (0x80000000u >> cntlz32(align)))
		{
			throw EXCEPTION("Invalid alignment (size=0x%x, align=0x%x)", size, align);
		}

		// return if size is invalid
		if (!size || size > this->size)
		{
			return 0;
		}

		// search for an appropriate place (unoptimized)
		for (u32 addr = ::align(this->addr, align); addr < this->addr + this->size - 1; addr += align)
		{
			if (try_alloc(addr, size))
			{
				return addr;
			}

			if (used + size > this->size)
			{
				return 0;
			}
		}

		return 0;
	}

	u32 block_t::falloc(u32 addr, u32 size)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// align to minimal page size
		size = ::align(size, 4096);

		// return if addr or size is invalid
		if (!size || size > this->size || addr < this->addr || addr + size - 1 >= this->addr + this->size - 1)
		{
			return 0;
		}

		if (!try_alloc(addr, size))
		{
			return 0;
		}

		return addr;
	}

	bool block_t::dealloc(u32 addr)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const auto found = m_map.find(addr);

		if (found != m_map.end())
		{
			const u32 size = found->second;

			// remove entry
			m_map.erase(found);

			// return "physical" memory
			used -= size;

			// unmap memory pages
			std::lock_guard<reservation_mutex_t>{ g_reservation_mutex }, _page_unmap(addr, size);

			return true;
		}

		return false;
	}

	std::shared_ptr<block_t> map(u32 addr, u32 size, u64 flags)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		if (!size || (size | addr) % 4096)
		{
			throw EXCEPTION("Invalid arguments (addr=0x%x, size=0x%x)", addr, size);
		}

		for (auto& block : g_locations)
		{
			if (block->addr >= addr && block->addr <= addr + size - 1)
			{
				return nullptr;
			}

			if (addr >= block->addr && addr <= block->addr + block->size - 1)
			{
				return nullptr;
			}
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (g_pages[i])
			{
				throw EXCEPTION("Unexpected pages allocated (current_addr=0x%x)", i * 4096);
			}
		}

		auto block = std::make_shared<block_t>(addr, size, flags);

		g_locations.emplace_back(block);

		return block;
	}

	std::shared_ptr<block_t> unmap(u32 addr)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		for (auto it = g_locations.begin(); it != g_locations.end(); it++)
		{
			if (*it && (*it)->addr == addr)
			{
				auto block = std::move(*it);
				g_locations.erase(it);
				return block;
			}
		}

		return nullptr;
	}

	std::shared_ptr<block_t> get(memory_location_t location, u32 addr)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		if (location != any)
		{
			// return selected location
			if (location < g_locations.size())
			{
				return g_locations[location];
			}

			return nullptr;
		}
		
		// search location by address
		for (auto& block : g_locations)
		{
			if (addr >= block->addr && addr <= block->addr + block->size - 1)
			{
				return block;
			}
		}

		return nullptr;
	}

	extern void start();

	namespace ps3
	{
		void init()
		{
			g_locations =
			{
				std::make_shared<block_t>(0x00010000, 0x1FFF0000), // main
				std::make_shared<block_t>(0x20000000, 0x10000000), // user
				std::make_shared<block_t>(0xC0000000, 0x10000000), // video
				std::make_shared<block_t>(0xD0000000, 0x10000000), // stack
				std::make_shared<block_t>(0xE0000000, 0x20000000), // SPU reserved
			};

			vm::start();
		}
	}

	namespace psv
	{
		void init()
		{
			g_locations = 
			{
				std::make_shared<block_t>(0x81000000, 0x10000000), // RAM
				std::make_shared<block_t>(0x91000000, 0x2F000000), // user
				std::make_shared<block_t>(0xC0000000, 0x10000000), // video (arbitrarily)
				std::make_shared<block_t>(0xD0000000, 0x10000000), // stack (arbitrarily)
			};

			vm::start();
		}
	}

	namespace psp
	{
		void init()
		{
			g_locations =
			{
				std::make_shared<block_t>(0x08000000, 0x02000000), // RAM
				std::make_shared<block_t>(0x08800000, 0x01800000), // user
				std::make_shared<block_t>(0x04000000, 0x00200000), // VRAM
				nullptr, // stack

				std::make_shared<block_t>(0x00010000, 0x00004000), // scratchpad
				std::make_shared<block_t>(0x88000000, 0x00800000), // kernel
			};

			vm::start();
		}
	}

	void close()
	{
		g_locations.clear();
	}

	u32 stack_push(u32 size, u32 align_v)
	{
		if (auto cpu = get_current_cpu_thread()) switch (cpu->type)
		{
		case cpu_type::ppu:
		{
			PPUThread& context = static_cast<PPUThread&>(*cpu);

			const u32 old_pos = vm::cast(context.GPR[1], HERE);
			context.GPR[1] -= align(size + 4, 8); // room minimal possible size
			context.GPR[1] &= ~(align_v - 1); // fix stack alignment

			if (context.GPR[1] < context.stack_addr)
			{
				throw EXCEPTION("Stack overflow (size=0x%x, align=0x%x, SP=0x%llx, stack=*0x%x)", size, align_v, old_pos, context.stack_addr);
			}
			else
			{
				const u32 addr = static_cast<u32>(context.GPR[1]);
				vm::ps3::_ref<nse_t<u32>>(addr + size) = old_pos;
				std::memset(vm::base(addr), 0, size);
				return addr;
			}
		}

		case cpu_type::spu:
		{
			SPUThread& context = static_cast<SPUThread&>(*cpu);

			const u32 old_pos = context.gpr[1]._u32[3];
			context.gpr[1]._u32[3] -= align(size + 4, 16);
			context.gpr[1]._u32[3] &= ~(align_v - 1);

			if (context.gpr[1]._u32[3] >= 0x40000) // extremely rough
			{
				throw EXCEPTION("Stack overflow (size=0x%x, align=0x%x, SP=LS:0x%05x)", size, align_v, old_pos);
			}
			else
			{
				const u32 addr = context.gpr[1]._u32[3] + context.offset;
				vm::ps3::_ref<nse_t<u32>>(addr + size) = old_pos;
				return addr;
			}
		}

		case cpu_type::arm:
		{
			ARMv7Thread& context = static_cast<ARMv7Thread&>(*cpu);

			const u32 old_pos = context.SP;
			context.SP -= align(size + 4, 4); // room minimal possible size
			context.SP &= ~(align_v - 1); // fix stack alignment

			if (context.SP < context.stack_addr)
			{
				throw EXCEPTION("Stack overflow (size=0x%x, align=0x%x, SP=0x%x, stack=*0x%x)", size, align_v, context.SP, context.stack_addr);
			}
			else
			{
				vm::psv::_ref<nse_t<u32>>(context.SP + size) = old_pos;
				return context.SP;
			}
		}

		default:
		{
			throw EXCEPTION("Invalid thread type (%u)", cpu->type);
		}
		}

		throw EXCEPTION("Invalid thread");
	}

	void stack_pop_verbose(u32 addr, u32 size) noexcept
	{
		if (auto cpu = get_current_cpu_thread()) switch (cpu->type)
		{
		case cpu_type::ppu:
		{
			PPUThread& context = static_cast<PPUThread&>(*cpu);

			if (context.GPR[1] != addr)
			{
				LOG_ERROR(MEMORY, "Stack inconsistency (addr=0x%x, SP=0x%llx, size=0x%x)", addr, context.GPR[1], size);
				return;
			}

			context.GPR[1] = vm::ps3::_ref<nse_t<u32>>(context.GPR[1] + size);
			return;
		}

		case cpu_type::spu:
		{
			SPUThread& context = static_cast<SPUThread&>(*cpu);

			if (context.gpr[1]._u32[3] + context.offset != addr)
			{
				LOG_ERROR(MEMORY, "Stack inconsistency (addr=0x%x, SP=LS:0x%05x, size=0x%x)", addr, context.gpr[1]._u32[3], size);
				return;
			}

			context.gpr[1]._u32[3] = vm::ps3::_ref<nse_t<u32>>(context.gpr[1]._u32[3] + context.offset + size);
			return;
		}

		case cpu_type::arm:
		{
			ARMv7Thread& context = static_cast<ARMv7Thread&>(*cpu);

			if (context.SP != addr)
			{
				LOG_ERROR(MEMORY, "Stack inconsistency (addr=0x%x, SP=0x%x, size=0x%x)", addr, context.SP, size);
				return;
			}

			context.SP = vm::psv::_ref<nse_t<u32>>(context.SP + size);
			return;
		}
		}
	}

	[[noreturn]] void throw_access_violation(u64 addr, const char* cause)
	{
		throw access_violation(addr, cause);
	}
}
