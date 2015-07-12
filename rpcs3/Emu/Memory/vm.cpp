#include "stdafx.h"
#include "Utilities/Log.h"
#include "Memory.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"

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

namespace vm
{
	void* initialize()
	{
#ifdef _WIN32
		HANDLE memory_handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_RESERVE, 0x1, 0x0, NULL);

		void* base_addr = MapViewOfFile(memory_handle, FILE_MAP_WRITE, 0, 0, 0x100000000);
		g_priv_addr = MapViewOfFile(memory_handle, FILE_MAP_WRITE, 0, 0, 0x100000000);

		CloseHandle(memory_handle);

		return base_addr;
#else
		int memory_handle = shm_open("/rpcs3_vm", O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

		if (memory_handle == -1)
		{
			std::printf("shm_open('/rpcs3_vm') failed\n");
			return (void*)-1;
		}

		if (ftruncate(memory_handle, 0x100000000) == -1)
		{
			std::printf("ftruncate(memory_handle) failed\n");
			shm_unlink("/rpcs3_vm");
			return (void*)-1;
		}

		void* base_addr = mmap(nullptr, 0x100000000, PROT_NONE, MAP_SHARED, memory_handle, 0);
		g_priv_addr = mmap(nullptr, 0x100000000, PROT_NONE, MAP_SHARED, memory_handle, 0);

		shm_unlink("/rpcs3_vm");

		std::printf("/rpcs3_vm: g_base_addr = %p, g_priv_addr = %p\n", base_addr, g_priv_addr);

		return base_addr;
#endif
	}

	void finalize()
	{
#ifdef _WIN32
		UnmapViewOfFile(g_base_addr);
		UnmapViewOfFile(g_priv_addr);
#else
		munmap(g_base_addr, 0x100000000);
		munmap(g_priv_addr, 0x100000000);
#endif
	}

	void* const g_base_addr = (atexit(finalize), initialize());
	void* g_priv_addr;

	std::array<atomic_t<u8>, 0x100000000ull / 4096> g_pages = {}; // information about every page

	class reservation_mutex_t
	{
		atomic_t<const thread_ctrl_t*> m_owner{};
		std::condition_variable m_cv;
		std::mutex m_mutex;

	public:
		reservation_mutex_t() = default;

		bool do_notify;

		never_inline void lock()
		{
			auto owner = get_current_thread_ctrl();

			std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);

			while (auto old = m_owner.compare_and_swap(nullptr, owner))
			{
				if (old == owner)
				{
					throw EXCEPTION("Deadlock");
				}

				if (!lock)
				{
					lock.lock();
					continue;
				}

				m_cv.wait_for(lock, std::chrono::milliseconds(1));
			}

			do_notify = true;
		}

		never_inline void unlock()
		{
			auto owner = get_current_thread_ctrl();

			if (!m_owner.compare_and_swap_test(owner, nullptr))
			{
				throw EXCEPTION("Lost lock");
			}

			if (do_notify)
			{
				m_cv.notify_one();
			}
		}
	};

	const thread_ctrl_t* volatile g_reservation_owner = nullptr;

	u32 g_reservation_addr = 0;
	u32 g_reservation_size = 0;

	reservation_mutex_t g_reservation_mutex;

	void _reservation_set(u32 addr, bool no_access = false)
	{
#ifdef _WIN32
		DWORD old;
		if (!VirtualProtect(vm::get_ptr(addr & ~0xfff), 4096, no_access ? PAGE_NOACCESS : PAGE_READONLY, &old))
#else
		if (mprotect(vm::get_ptr(addr & ~0xfff), 4096, no_access ? PROT_NONE : PROT_READ))
#endif
		{
			throw EXCEPTION("System failure (addr=0x%x)", addr);
		}
	}

	void _reservation_break(u32 addr)
	{
		if (g_reservation_addr >> 12 == addr >> 12)
		{
#ifdef _WIN32
			DWORD old;
			if (!VirtualProtect(vm::get_ptr(addr & ~0xfff), 4096, PAGE_READWRITE, &old))
#else
			if (mprotect(vm::get_ptr(addr & ~0xfff), 4096, PROT_READ | PROT_WRITE))
#endif
			{
				throw EXCEPTION("System failure (addr=0x%x)", addr);
			}

			g_reservation_addr = 0;
			g_reservation_size = 0;
			g_reservation_owner = nullptr;
		}
	}

	void reservation_break(u32 addr)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		_reservation_break(addr);
	}

	void reservation_acquire(void* data, u32 addr, u32 size)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 128);
		assert((addr + size - 1 & ~0xfff) == (addr & ~0xfff));

		const u8 flags = g_pages[addr >> 12].load();

		if (!(flags & page_writable) || !(flags & page_allocated) || (flags & page_no_reservations))
		{
			throw EXCEPTION("Invalid page flags (addr=0x%x, size=0x%x, flags=0x%x)", addr, size, flags);
		}

		// silent unlocking to prevent priority boost for threads going to break reservation
		//g_reservation_mutex.do_notify = false;

		// break previous reservation
		if (g_reservation_owner)
		{
			_reservation_break(g_reservation_addr);
		}

		// change memory protection to read-only
		_reservation_set(addr);

		// may not be necessary
		_mm_mfence();

		// set additional information
		g_reservation_addr = addr;
		g_reservation_size = size;
		g_reservation_owner = get_current_thread_ctrl();

		// copy data
		memcpy(data, vm::get_ptr(addr), size);
	}

	bool reservation_update(u32 addr, const void* data, u32 size)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 128);
		assert((addr + size - 1 & ~0xfff) == (addr & ~0xfff));

		if (g_reservation_owner != get_current_thread_ctrl() || g_reservation_addr != addr || g_reservation_size != size)
		{
			// atomic update failed
			return false;
		}

		// change memory protection to no access
		_reservation_set(addr, true);

		// update memory using privileged access
		memcpy(vm::priv_ptr(addr), data, size);

		// free the reservation and restore memory protection
		_reservation_break(addr);

		// atomic update succeeded
		return true;
	}

	bool reservation_query(u32 addr, u32 size, bool is_writing, std::function<bool()> callback)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		if (!check_addr(addr))
		{
			return false;
		}

		// check if current reservation and address may overlap
		if (g_reservation_addr >> 12 == addr >> 12 && is_writing)
		{
			if (size && addr + size - 1 >= g_reservation_addr && g_reservation_addr + g_reservation_size - 1 >= addr)
			{
				// break the reservation if overlap
				_reservation_break(addr);
			}
			else
			{
				return callback(); //? true : _reservation_break(addr), true;
			}
		}
		
		return true;
	}

	bool reservation_test()
	{
		const auto owner = g_reservation_owner;

		return owner && owner == get_current_thread_ctrl();
	}

	void reservation_free()
	{
		if (reservation_test())
		{
			std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

			_reservation_break(g_reservation_addr);
		}
	}

	void reservation_op(u32 addr, u32 size, std::function<void()> proc)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 128);
		assert((addr + size - 1 & ~0xfff) == (addr & ~0xfff));

		// break previous reservation
		if (g_reservation_owner != get_current_thread_ctrl() || g_reservation_addr != addr || g_reservation_size != size)
		{
			if (g_reservation_owner)
			{
				_reservation_break(g_reservation_addr);
			}
		}

		// change memory protection to no access
		_reservation_set(addr, true);

		// set additional information
		g_reservation_addr = addr;
		g_reservation_size = size;
		g_reservation_owner = get_current_thread_ctrl();

		// may not be necessary
		_mm_mfence();

		// do the operation
		proc();

		// remove the reservation
		_reservation_break(addr);
	}

	void _page_map(u32 addr, u32 size, u8 flags)
	{
		assert(size && (size | addr) % 4096 == 0 && flags < page_allocated);

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (g_pages[i].load())
			{
				throw EXCEPTION("Memory already mapped (addr=0x%x, size=0x%x, flags=0x%x, current_addr=0x%x)", addr, size, flags, i * 4096);
			}
		}

		void* real_addr = vm::get_ptr(addr);
		void* priv_addr = vm::priv_ptr(addr);

#ifdef _WIN32
		auto protection = flags & page_writable ? PAGE_READWRITE : (flags & page_readable ? PAGE_READONLY : PAGE_NOACCESS);
		if (!VirtualAlloc(priv_addr, size, MEM_COMMIT, PAGE_READWRITE) || !VirtualAlloc(real_addr, size, MEM_COMMIT, protection))
#else
		auto protection = flags & page_writable ? PROT_WRITE | PROT_READ : (flags & page_readable ? PROT_READ : PROT_NONE);
		if (mprotect(priv_addr, size, PROT_READ | PROT_WRITE) || mprotect(real_addr, size, protection))
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

		memset(priv_addr, 0, size); // ???
	}

	bool page_protect(u32 addr, u32 size, u8 flags_test, u8 flags_set, u8 flags_clear)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		u8 flags_inv = flags_set & flags_clear;

		assert(size && (size | addr) % 4096 == 0);

		flags_test |= page_allocated;

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if ((g_pages[i].load() & flags_test) != (flags_test | page_allocated))
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

			const u8 f1 = g_pages[i]._or(flags_set & ~flags_inv) & (page_writable | page_readable);
			g_pages[i]._and_not(flags_clear & ~flags_inv);
			const u8 f2 = (g_pages[i] ^= flags_inv) & (page_writable | page_readable);

			if (f1 != f2)
			{
				void* real_addr = vm::get_ptr(i * 4096);

#ifdef _WIN32
				DWORD old;

				auto protection = f2 & page_writable ? PAGE_READWRITE : (f2 & page_readable ? PAGE_READONLY : PAGE_NOACCESS);
				if (!VirtualProtect(real_addr, 4096, protection, &old))
#else
				auto protection = f2 & page_writable ? PROT_WRITE | PROT_READ : (f2 & page_readable ? PROT_READ : PROT_NONE);
				if (mprotect(real_addr, 4096, protection))
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
		assert(size && (size | addr) % 4096 == 0);

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (!(g_pages[i].load() & page_allocated))
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

		void* real_addr = vm::get_ptr(addr);
		void* priv_addr = vm::priv_ptr(addr);

#ifdef _WIN32
		DWORD old;

		if (!VirtualProtect(real_addr, size, PAGE_NOACCESS, &old) || !VirtualProtect(priv_addr, size, PAGE_NOACCESS, &old))
#else
		if (mprotect(real_addr, size, PROT_NONE) || mprotect(priv_addr, size, PROT_NONE))
#endif
		{
			throw EXCEPTION("System failure (addr=0x%x, size=0x%x)", addr, size);
		}
	}

	bool check_addr(u32 addr, u32 size)
	{
		assert(size);

		if (addr + (size - 1) < addr)
		{
			return false;
		}

		for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
		{
			if ((g_pages[i].load() & page_allocated) != page_allocated)
			{
				return false;
			}
		}
		
		return true;
	}

	std::vector<std::shared_ptr<block_t>> g_locations;

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

	bool block_t::try_alloc(u32 addr, u32 size)
	{
		// check if memory area is already mapped
		for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
		{
			if (g_pages[i].load())
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
			// unmap memory pages
			vm::_page_unmap(entry.first, entry.second);
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

			if (used.load() + size > this->size)
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
			std::lock_guard<reservation_mutex_t>{ g_reservation_mutex }, vm::_page_unmap(addr, size);

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
			if (g_pages[i].load())
			{
				throw EXCEPTION("Unexpected memory usage");
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

				std::make_shared<block_t>(0xE0000000, 0x20000000), // RawSPU
			};
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
				nullptr, // video
				nullptr, // stack
			};
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
		}
	}

	void close()
	{
		g_locations.clear();
	}

	u32 stack_push(CPUThread& CPU, u32 size, u32 align_v, u32& old_pos)
	{
		switch (CPU.GetType())
		{
		case CPU_THREAD_PPU:
		{
			PPUThread& context = static_cast<PPUThread&>(CPU);

			old_pos = VM_CAST(context.GPR[1]);
			context.GPR[1] -= align(size, 8); // room minimal possible size
			context.GPR[1] &= ~(align_v - 1); // fix stack alignment

			if (context.GPR[1] < context.stack_addr)
			{
				throw EXCEPTION("Stack overflow (size=0x%x, align=0x%x, SP=0x%llx, stack=*0x%x)", size, align_v, old_pos, context.stack_addr);
			}
			else
			{
				return static_cast<u32>(context.GPR[1]);
			}
		}

		case CPU_THREAD_SPU:
		case CPU_THREAD_RAW_SPU:
		{
			SPUThread& context = static_cast<SPUThread&>(CPU);

			old_pos = context.GPR[1]._u32[3];
			context.GPR[1]._u32[3] -= align(size, 16);
			context.GPR[1]._u32[3] &= ~(align_v - 1);

			if (context.GPR[1]._u32[3] >= 0x40000) // extremely rough
			{
				throw EXCEPTION("Stack overflow (size=0x%x, align=0x%x, SP=LS:0x%05x)", size, align_v, old_pos);
			}
			else
			{
				return context.GPR[1]._u32[3] + context.offset;
			}
		}

		case CPU_THREAD_ARMv7:
		{
			ARMv7Context& context = static_cast<ARMv7Thread&>(CPU);

			old_pos = context.SP;
			context.SP -= align(size, 4); // room minimal possible size
			context.SP &= ~(align_v - 1); // fix stack alignment

			if (context.SP < context.stack_addr)
			{
				throw EXCEPTION("Stack overflow (size=0x%x, align=0x%x, SP=0x%x, stack=*0x%x)", size, align_v, context.SP, context.stack_addr);
			}
			else
			{
				return context.SP;
			}
		}

		default:
		{
			throw EXCEPTION("Invalid thread type (%d)", CPU.GetId());
		}
		}
	}

	void stack_pop(CPUThread& CPU, u32 addr, u32 old_pos)
	{
		switch (CPU.GetType())
		{
		case CPU_THREAD_PPU:
		{
			PPUThread& context = static_cast<PPUThread&>(CPU);

			if (context.GPR[1] != addr)
			{
				throw EXCEPTION("Stack inconsistency (addr=0x%x, SP=0x%llx, old_pos=0x%x)", addr, context.GPR[1], old_pos);
			}

			context.GPR[1] = old_pos;
			return;
		}

		case CPU_THREAD_SPU:
		case CPU_THREAD_RAW_SPU:
		{
			SPUThread& context = static_cast<SPUThread&>(CPU);

			if (context.GPR[1]._u32[3] + context.offset != addr)
			{
				throw EXCEPTION("Stack inconsistency (addr=0x%x, SP=LS:0x%05x, old_pos=LS:0x%05x)", addr, context.GPR[1]._u32[3], old_pos);
			}

			context.GPR[1]._u32[3] = old_pos;
			return;
		}

		case CPU_THREAD_ARMv7:
		{
			ARMv7Context& context = static_cast<ARMv7Thread&>(CPU);

			if (context.SP != addr)
			{
				throw EXCEPTION("Stack inconsistency (addr=0x%x, SP=0x%x, old_pos=0x%x)", addr, context.SP, old_pos);
			}

			context.SP = old_pos;
			return;
		}

		default:
		{
			throw EXCEPTION("Invalid thread type (%d)", CPU.GetType());
		}
		}
	}
}
