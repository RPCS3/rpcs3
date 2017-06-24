#include "stdafx.h"
#include "Memory.h"
#include "Emu/System.h"
#include "Utilities/mutex.h"
#include "Utilities/Thread.h"
#include "Utilities/VirtualMemory.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/lv2/sys_memory.h"

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

#include <atomic>
#include <deque>

namespace vm
{
	static u8* memory_reserve_4GiB(std::uintptr_t addr = 0)
	{
		for (u64 addr = 0x100000000;; addr += 0x100000000)
		{
			if (auto ptr = utils::memory_reserve(0x100000000, (void*)addr))
			{
				return static_cast<u8*>(ptr);
			}
		}

		// TODO: a condition to break loop
		return static_cast<u8*>(utils::memory_reserve(0x100000000));
	}

	// Emulated virtual memory
	u8* const g_base_addr = memory_reserve_4GiB(0);

	// Auxiliary virtual memory for executable areas
	u8* const g_exec_addr = memory_reserve_4GiB((std::uintptr_t)g_base_addr);

	// Memory locations
	std::vector<std::shared_ptr<block_t>> g_locations;

	// Reservations (lock lines) in a single memory page
	using reservation_info = std::array<std::atomic<u64>, 4096 / 128>;

	// Registered waiters
	std::deque<vm::waiter*> g_waiters;

	// Memory mutex core
	shared_mutex g_mutex;

	// Memory mutex acknowledgement
	thread_local atomic_t<cpu_thread*>* g_tls_locked = nullptr;

	// Memory mutex: passive locks
	std::array<atomic_t<cpu_thread*>, 32> g_locks;

	static void _register_lock(cpu_thread* _cpu)
	{
		for (u32 i = 0;; i = (i + 1) % g_locks.size())
		{
			if (!g_locks[i] && g_locks[i].compare_and_swap_test(nullptr, _cpu))
			{
				g_tls_locked = g_locks.data() + i;
				return;
			}
		}
	}

	void passive_lock(cpu_thread& cpu)
	{
		if (g_tls_locked && *g_tls_locked == &cpu)
		{
			return;
		}

		::reader_lock lock(g_mutex);

		_register_lock(&cpu);
	}

	void passive_unlock(cpu_thread& cpu)
	{
		if (g_tls_locked)
		{
			g_tls_locked->compare_and_swap_test(&cpu, nullptr);
			::reader_lock lock(g_mutex);
			g_tls_locked = nullptr;
		}
	}

	void cleanup_unlock(cpu_thread& cpu) noexcept
	{
		if (g_tls_locked && cpu.get() == thread_ctrl::get_current())
		{
			g_tls_locked = nullptr;
		}

		for (u32 i = 0; i < g_locks.size(); i++)
		{
			if (g_locks[i] == &cpu)
			{
				g_locks[i].compare_and_swap_test(&cpu, nullptr);
				return;
			}
		}
	}

	void temporary_unlock(cpu_thread& cpu) noexcept
	{
		if (g_tls_locked && g_tls_locked->compare_and_swap_test(&cpu, nullptr))
		{
			cpu.state.test_and_set(cpu_flag::memory);
		}
	}

	void temporary_unlock() noexcept
	{
		if (auto cpu = get_current_cpu_thread())
		{
			temporary_unlock(*cpu);
		}
	}

	reader_lock::reader_lock()
		: locked(true)
	{
		auto cpu = get_current_cpu_thread();

		if (!cpu || !g_tls_locked || !g_tls_locked->compare_and_swap_test(cpu, nullptr))
		{
			cpu = nullptr;
		}
		
		g_mutex.lock_shared();

		if (cpu)
		{
			_register_lock(cpu);
			cpu->state -= cpu_flag::memory;
		}
	}

	reader_lock::reader_lock(const try_to_lock_t&)
		: locked(g_mutex.try_lock_shared())
	{
	}

	reader_lock::~reader_lock()
	{
		if (locked)
		{
			g_mutex.unlock_shared();
		}
	}

	writer_lock::writer_lock(int full)
		: locked(true)
	{
		auto cpu = get_current_cpu_thread();

		if (!cpu || !g_tls_locked || !g_tls_locked->compare_and_swap_test(cpu, nullptr))
		{
			cpu = nullptr;
		}

		g_mutex.lock();

		if (full)
		{
			for (auto& lock : g_locks)
			{
				if (cpu_thread* ptr = lock)
				{
					ptr->state.test_and_set(cpu_flag::memory);
				}
			}

			for (auto& lock : g_locks)
			{
				while (cpu_thread* ptr = lock)
				{
					if (test(ptr->state, cpu_flag::dbg_global_stop + cpu_flag::exit))
					{
						break;
					}

					busy_wait();
				}
			}
		}

		if (cpu)
		{
			_register_lock(cpu);
			cpu->state -= cpu_flag::memory;
		}
	}

	writer_lock::writer_lock(const try_to_lock_t&)
		: locked(g_mutex.try_lock())
	{
	}

	writer_lock::~writer_lock()
	{
		if (locked)
		{
			g_mutex.unlock();
		}
	}

	// Page information
	struct memory_page
	{
		// Memory flags
		atomic_t<u8> flags;

		atomic_t<u32> waiters;

		// Reservations
		atomic_t<reservation_info*> reservations;

		// Access reservation info
		std::atomic<u64>& operator [](u32 addr)
		{
			auto ptr = reservations.load();

			if (!ptr)
			{
				// Opportunistic memory allocation
				ptr = new reservation_info{};

				if (auto old_ptr = reservations.compare_and_swap(nullptr, ptr))
				{
					delete ptr;
					ptr = old_ptr;
				}
			}

			return (*ptr)[(addr & 0xfff) >> 7];
		}
	};

	// Memory pages
	std::array<memory_page, 0x100000000 / 4096> g_pages{};

	u64 reservation_acquire(u32 addr, u32 _size)
	{
		// Access reservation info: stamp and the lock bit
		return g_pages[addr >> 12][addr].load(std::memory_order_acquire);
	}

	void reservation_update(u32 addr, u32 _size)
	{
		// Update reservation info with new timestamp (unsafe, assume allocated)
		(*g_pages[addr >> 12].reservations)[(addr & 0xfff) >> 7].store(__rdtsc(), std::memory_order_release);
	}

	void waiter::init()
	{
		// Register waiter
		writer_lock lock(0);

		g_waiters.emplace_back(this);
	}

	void waiter::test() const
	{
		if (std::memcmp(data, vm::base(addr), size) == 0)
		{
			return;
		}

		memory_page& page = g_pages[addr >> 12];

		if (page.reservations == nullptr)
		{
			return;
		}

		if (stamp >= (*page.reservations)[(addr & 0xfff) >> 7].load())
		{
			return;
		}

		if (owner)
		{
			owner->notify();
		}
	}

	waiter::~waiter()
	{
		// Unregister waiter
		writer_lock lock(0);

		// Find waiter
		const auto found = std::find(g_waiters.cbegin(), g_waiters.cend(), this);

		if (found != g_waiters.cend())
		{
			g_waiters.erase(found);
		}
	}

	void notify(u32 addr, u32 size)
	{
		for (const waiter* ptr : g_waiters)
		{
			if (ptr->addr / 128 == addr / 128)
			{
				ptr->test();
			}
		}
	}

	void notify_all()
	{
		for (const waiter* ptr : g_waiters)
		{
			ptr->test();
		}
	}

	void _page_map(u32 addr, u32 size, u8 flags)
	{
		if (!size || (size | addr) % 4096 || flags & page_allocated)
		{
			fmt::throw_exception("Invalid arguments (addr=0x%x, size=0x%x)" HERE, addr, size);
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (g_pages[i].flags)
			{
				fmt::throw_exception("Memory already mapped (addr=0x%x, size=0x%x, flags=0x%x, current_addr=0x%x)" HERE, addr, size, flags, i * 4096);
			}
		}

		void* real_addr = g_base_addr + addr;
		void* exec_addr = g_exec_addr + addr;

#ifdef _WIN32
		auto protection = flags & page_writable ? PAGE_READWRITE : (flags & page_readable ? PAGE_READONLY : PAGE_NOACCESS);
		verify(__func__), ::VirtualAlloc(real_addr, size, MEM_COMMIT, protection);
#else
		auto protection = flags & page_writable ? PROT_WRITE | PROT_READ : (flags & page_readable ? PROT_READ : PROT_NONE);
		verify(__func__), !::mprotect(real_addr, size, protection), !::madvise(real_addr, size, MADV_WILLNEED);
#endif

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (g_pages[i].flags.exchange(flags | page_allocated))
			{
				fmt::throw_exception("Concurrent access (addr=0x%x, size=0x%x, flags=0x%x, current_addr=0x%x)" HERE, addr, size, flags, i * 4096);
			}
		}
	}

	bool page_protect(u32 addr, u32 size, u8 flags_test, u8 flags_set, u8 flags_clear)
	{
		writer_lock lock;

		if (!size || (size | addr) % 4096)
		{
			fmt::throw_exception("Invalid arguments (addr=0x%x, size=0x%x)" HERE, addr, size);
		}

		const u8 flags_both = flags_set & flags_clear;

		flags_test  |= page_allocated;
		flags_set   &= ~flags_both;
		flags_clear &= ~flags_both;

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if ((g_pages[i].flags & flags_test) != (flags_test | page_allocated))
			{
				return false;
			}
		}

		if (!flags_set && !flags_clear)
		{
			return true;
		}

		u8 start_value = 0xff;

		for (u32 start = addr / 4096, end = start + size / 4096, i = start; i < end + 1; i++)
		{
			u8 new_val = 0xff;

			if (i < end)
			{
				g_pages[i].flags |= flags_set;
				g_pages[i].flags &= ~flags_clear;

				new_val = g_pages[i].flags & (page_readable | page_writable);
			}

			if (new_val != start_value)
			{
				if (u32 page_size = (i - start) * 4096)
				{
#ifdef _WIN32
					DWORD old;

					auto protection = start_value & page_writable ? PAGE_READWRITE : (start_value & page_readable ? PAGE_READONLY : PAGE_NOACCESS);
					verify(__func__), ::VirtualProtect(vm::base(start * 4096), page_size, protection, &old);
#else
					auto protection = start_value & page_writable ? PROT_WRITE | PROT_READ : (start_value & page_readable ? PROT_READ : PROT_NONE);
					verify(__func__), !::mprotect(vm::base(start * 4096), page_size, protection);
#endif
				}

				start_value = new_val;
				start = i;
			}
		}

		return true;
	}

	void _page_unmap(u32 addr, u32 size)
	{
		if (!size || (size | addr) % 4096)
		{
			fmt::throw_exception("Invalid arguments (addr=0x%x, size=0x%x)" HERE, addr, size);
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if ((g_pages[i].flags & page_allocated) == 0)
			{
				fmt::throw_exception("Memory not mapped (addr=0x%x, size=0x%x, current_addr=0x%x)" HERE, addr, size, i * 4096);
			}
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (!(g_pages[i].flags.exchange(0) & page_allocated))
			{
				fmt::throw_exception("Concurrent access (addr=0x%x, size=0x%x, current_addr=0x%x)" HERE, addr, size, i * 4096);
			}
		}

		void* real_addr = g_base_addr + addr;
		void* exec_addr = g_exec_addr + addr;

#ifdef _WIN32
		verify(__func__), ::VirtualFree(real_addr, size, MEM_DECOMMIT);
#else
		verify(__func__), ::mmap(real_addr, size, PROT_NONE, MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
	}

	bool check_addr(u32 addr, u32 size, u8 flags)
	{
		for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
		{
			if (UNLIKELY((g_pages[i % g_pages.size()].flags & flags) != flags))
			{
				return false;
			}
		}
		
		return true;
	}

	u32 alloc(u32 size, memory_location_t location, u32 align, u32 sup)
	{
		const auto block = get(location);

		if (!block)
		{
			fmt::throw_exception("Invalid memory location (%u)" HERE, (uint)location);
		}

		return block->alloc(size, align, sup);
	}

	u32 falloc(u32 addr, u32 size, memory_location_t location, u32 sup)
	{
		const auto block = get(location, addr);

		if (!block)
		{
			fmt::throw_exception("Invalid memory location (%u, addr=0x%x)" HERE, (uint)location, addr);
		}

		return block->falloc(addr, size, sup);
	}

	u32 dealloc(u32 addr, memory_location_t location, u32* sup_out)
	{
		const auto block = get(location, addr);

		if (!block)
		{
			fmt::throw_exception("Invalid memory location (%u, addr=0x%x)" HERE, (uint)location, addr);
		}

		return block->dealloc(addr, sup_out);
	}

	void dealloc_verbose_nothrow(u32 addr, memory_location_t location) noexcept
	{
		const auto block = get(location, addr);

		if (!block)
		{
			LOG_ERROR(MEMORY, "vm::dealloc(): invalid memory location (%u, addr=0x%x)\n", (uint)location, addr);
			return;
		}

		if (!block->dealloc(addr))
		{
			LOG_ERROR(MEMORY, "vm::dealloc(): deallocation failed (addr=0x%x)\n", addr);
			return;
		}
	}

	bool block_t::try_alloc(u32 addr, u32 size, u8 flags, u32 sup)
	{
		// Check if memory area is already mapped
		for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
		{
			if (g_pages[i].flags)
			{
				return false;
			}
		}

		// Map "real" memory pages
		_page_map(addr, size, flags);

		// Add entry
		m_map[addr] = size;

		// Add supplementary info if necessary
		if (sup) m_sup[addr] = sup;

		return true;
	}

	block_t::block_t(u32 addr, u32 size, u64 flags)
		: addr(addr)
		, size(size)
		, flags(flags)
	{
	}

	block_t::~block_t()
	{
		writer_lock lock;

		// Deallocate all memory
		for (auto& entry : m_map)
		{
			_page_unmap(entry.first, entry.second);
		}
	}

	u32 block_t::alloc(u32 size, u32 align, u32 sup)
	{
		writer_lock lock;

		// Align to minimal page size
		size = ::align(size, 4096);

		// Check alignment (it's page allocation, so passing small values there is just silly)
		if (align < 4096 || align != (0x80000000u >> cntlz32(align, true)))
		{
			fmt::throw_exception("Invalid alignment (size=0x%x, align=0x%x)" HERE, size, align);
		}

		// Return if size is invalid
		if (!size || size > this->size)
		{
			return 0;
		}

		u8 pflags = page_readable | page_writable;

		if (align >= 0x100000)
		{
			pflags |= page_1m_size;
		}
		else if (align >= 0x10000)
		{
			pflags |= page_64k_size;
		}

		// Search for an appropriate place (unoptimized)
		for (u32 addr = ::align(this->addr, align); addr < this->addr + this->size - 1; addr += align)
		{
			if (try_alloc(addr, size, pflags, sup))
			{
				return addr;
			}
		}

		return 0;
	}

	u32 block_t::falloc(u32 addr, u32 size, u32 sup)
	{
		writer_lock lock;

		// align to minimal page size
		size = ::align(size, 4096);

		// return if addr or size is invalid
		if (!size || size > this->size || addr < this->addr || addr + size - 1 > this->addr + this->size - 1)
		{
			return 0;
		}
		u8 pflags = page_readable | page_writable;
		if ((flags & SYS_MEMORY_PAGE_SIZE_1M) == SYS_MEMORY_PAGE_SIZE_1M)
		{
			pflags |= page_1m_size;
		}
		else if ((flags & SYS_MEMORY_PAGE_SIZE_64K) == SYS_MEMORY_PAGE_SIZE_64K)
		{
			pflags |= page_64k_size;
		}

		if (!try_alloc(addr, size, pflags, sup))
		{
			return 0;
		}

		return addr;
	}

	u32 block_t::dealloc(u32 addr, u32* sup_out)
	{
		writer_lock lock;

		const auto found = m_map.find(addr);

		if (found != m_map.end())
		{
			const u32 size = found->second;

			// Remove entry
			m_map.erase(found);

			// Unmap "real" memory pages
			_page_unmap(addr, size);

			// Write supplementary info if necessary
			if (sup_out) *sup_out = m_sup[addr];

			// Remove supplementary info
			m_sup.erase(addr);

			return size;
		}

		return 0;
	}

	u32 block_t::used()
	{
		reader_lock lock;

		u32 result = 0;

		for (auto& entry : m_map)
		{
			result += entry.second;
		}

		return result;
	}

	std::shared_ptr<block_t> map(u32 addr, u32 size, u64 flags)
	{
		writer_lock lock(0);

		if (!size || (size | addr) % 4096)
		{
			fmt::throw_exception("Invalid arguments (addr=0x%x, size=0x%x)" HERE, addr, size);
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
			if (g_pages[i].flags)
			{
				fmt::throw_exception("Unexpected pages allocated (current_addr=0x%x)" HERE, i * 4096);
			}
		}

		auto block = std::make_shared<block_t>(addr, size, flags);

		g_locations.emplace_back(block);

		return block;
	}

	std::shared_ptr<block_t> unmap(u32 addr, bool must_be_empty)
	{
		writer_lock lock(0);

		for (auto it = g_locations.begin(); it != g_locations.end(); it++)
		{
			if (*it && (*it)->addr == addr)
			{
				if (must_be_empty && (!it->unique() || (*it)->used()))
				{
					return *it;
				}

				auto block = std::move(*it);
				g_locations.erase(it);
				return block;
			}
		}

		return nullptr;
	}

	std::shared_ptr<block_t> get(memory_location_t location, u32 addr)
	{
		reader_lock lock;

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
				std::make_shared<block_t>(0xE0000000, 0x20000000), // SPU reserved
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
				std::make_shared<block_t>(0xC0000000, 0x10000000), // video (arbitrarily)
				std::make_shared<block_t>(0xD0000000, 0x10000000), // stack (arbitrarily)
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

		utils::memory_decommit(g_base_addr, 0x100000000);
		utils::memory_decommit(g_exec_addr, 0x100000000);
	}
}

void fmt_class_string<vm::_ptr_base<const void>>::format(std::string& out, u64 arg)
{
	fmt_class_string<u32>::format(out, arg);
}

void fmt_class_string<vm::_ptr_base<const char>>::format(std::string& out, u64 arg)
{
	// Special case (may be allowed for some arguments)
	if (arg == 0)
	{
		out += u8"«NULL»";
		return;
	}

	// Filter certainly invalid addresses (TODO)
	if (arg < 0x10000 || arg >= 0xf0000000)
	{
		out += u8"«INVALID_ADDRESS:";
		fmt_class_string<u32>::format(out, arg);
		out += u8"»";
		return;
	}

	const auto start = out.size();

	out += u8"“";

	for (vm::_ptr_base<const volatile char> ptr = vm::cast(arg);; ptr++)
	{
		if (!vm::check_addr(ptr.addr()))
		{
			// TODO: optimize checks
			out.resize(start);
			out += u8"«INVALID_ADDRESS:";
			fmt_class_string<u32>::format(out, arg);
			out += u8"»";
			return;
		}

		if (const char ch = *ptr)
		{
			out += ch;
		}
		else
		{
			break;
		}
	}

	out += u8"”";
}
