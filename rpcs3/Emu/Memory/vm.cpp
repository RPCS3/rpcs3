#include "stdafx.h"
#include "Memory.h"
#include "Emu/System.h"
#include "Utilities/mutex.h"
#include "Utilities/cond.h"
#include "Utilities/Thread.h"
#include "Utilities/VirtualMemory.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/RSX/GSRender.h"
#include <atomic>
#include <thread>
#include <deque>

static_assert(sizeof(notifier) == 8, "Unexpected size of notifier");

namespace vm
{
	static u8* memory_reserve_4GiB(std::uintptr_t _addr = 0)
	{
		for (u64 addr = _addr + 0x100000000;; addr += 0x100000000)
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

	// Stats for debugging
	u8* const g_stat_addr = memory_reserve_4GiB((std::uintptr_t)g_exec_addr);

	// Reservation stats (compressed x16)
	u8* const g_reservations = memory_reserve_4GiB((std::uintptr_t)g_stat_addr);

	// Reservation sync variables
	u8* const g_reservations2 = g_reservations + 0x10000000;

	// Memory locations
	std::vector<std::shared_ptr<block_t>> g_locations;

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

	bool passive_lock(cpu_thread& cpu, bool wait)
	{
		if (UNLIKELY(g_tls_locked && *g_tls_locked == &cpu))
		{
			return true;
		}

		if (LIKELY(g_mutex.is_lockable()))
		{
			// Optimistic path (hope that mutex is not exclusively locked)
			_register_lock(&cpu);

			if (UNLIKELY(!g_mutex.is_lockable()))
			{
				passive_unlock(cpu);

				if (!wait)
				{
					return false;
				}

				::reader_lock lock(g_mutex);
				_register_lock(&cpu);
			}
		}
		else
		{
			if (!wait)
			{
				return false;
			}

			::reader_lock lock(g_mutex);
			_register_lock(&cpu);
		}

		return true;
	}

	void passive_unlock(cpu_thread& cpu)
	{
		if (auto& ptr = g_tls_locked)
		{
			*ptr = nullptr;
			ptr = nullptr;

			if (test(cpu.state, cpu_flag::memory))
			{
				cpu.state -= cpu_flag::memory;
			}
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
			cpu.cpu_unmem();
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

	writer_lock::~writer_lock()
	{
		if (locked)
		{
			g_mutex.unlock();
		}
	}

	void reservation_lock_internal(atomic_t<u64>& res)
	{
		for (u64 i = 0;; i++)
		{
			if (LIKELY(!atomic_storage<u64>::bts(res.raw(), 0)))
			{
				break;
			}

			if (i < 15)
			{
				busy_wait(500);
			}
			else
			{
				std::this_thread::yield();
			}
		}
	}

	// Page information
	struct memory_page
	{
		// Memory flags
		atomic_t<u8> flags;
	};

	// Memory pages
	std::array<memory_page, 0x100000000 / 4096> g_pages{};

	static void _page_map(u32 addr, u8 flags, utils::shm& shm)
	{
		const u32 size = shm.size();

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

		if (shm.map_critical(g_base_addr + addr) != g_base_addr + addr)
		{
			fmt::throw_exception("Memory mapping failed - blame Windows (addr=0x%x, size=0x%x, flags=0x%x)", addr, size, flags);
		}

		if (flags & page_executable)
		{
			utils::memory_commit(g_exec_addr + addr, size);
		}

		if (g_cfg.core.ppu_debug)
		{
			utils::memory_commit(g_stat_addr + addr, size);
		}

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
		vm::writer_lock lock(0);

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
					const auto protection = start_value & page_writable ? utils::protection::rw : (start_value & page_readable ? utils::protection::ro : utils::protection::no);
					utils::memory_protect(g_base_addr + start * 4096, page_size, protection);
				}

				start_value = new_val;
				start = i;
			}
		}

		return true;
	}

	static void _page_unmap(u32 addr, utils::shm& shm)
	{
		const u32 size = shm.size();

		if (!size || (size | addr) % 4096)
		{
			fmt::throw_exception("Invalid arguments (addr=0x%x, size=0x%x)" HERE, addr, size);
		}

		bool is_exec = false;

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if ((g_pages[i].flags & page_allocated) == 0)
			{
				fmt::throw_exception("Memory not mapped (addr=0x%x, size=0x%x, current_addr=0x%x)" HERE, addr, size, i * 4096);
			}

			if (g_pages[i].flags & page_executable)
			{
				is_exec = true;
			}
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (!(g_pages[i].flags.exchange(0) & page_allocated))
			{
				fmt::throw_exception("Concurrent access (addr=0x%x, size=0x%x, current_addr=0x%x)" HERE, addr, size, i * 4096);
			}
		}

		shm.unmap_critical(g_base_addr + addr);

		if (is_exec)
		{
			utils::memory_decommit(g_exec_addr + addr, size);
		}

		if (g_cfg.core.ppu_debug)
		{
			utils::memory_decommit(g_stat_addr + addr, size);
		}
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

	u32 alloc(u32 size, memory_location_t location, u32 align)
	{
		const auto block = get(location);

		if (!block)
		{
			fmt::throw_exception("Invalid memory location (%u)" HERE, (uint)location);
		}

		return block->alloc(size, align);
	}

	u32 falloc(u32 addr, u32 size, memory_location_t location)
	{
		const auto block = get(location, addr);

		if (!block)
		{
			fmt::throw_exception("Invalid memory location (%u, addr=0x%x)" HERE, (uint)location, addr);
		}

		return block->falloc(addr, size);
	}

	u32 dealloc(u32 addr, memory_location_t location)
	{
		const auto block = get(location, addr);

		if (!block)
		{
			fmt::throw_exception("Invalid memory location (%u, addr=0x%x)" HERE, (uint)location, addr);
		}

		return block->dealloc(addr);
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

	bool block_t::try_alloc(u32 addr, u8 flags, std::shared_ptr<utils::shm>&& shm)
	{
		const u32 size = shm->size();

		// Check if memory area is already mapped
		for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
		{
			if (g_pages[i].flags)
			{
				return false;
			}
		}

		// Map "real" memory pages
		_page_map(addr, flags, *shm);

		// Add entry
		m_map[addr] = std::move(shm);

		return true;
	}

	block_t::block_t(u32 addr, u32 size, u64 flags)
		: addr(addr)
		, size(size)
		, flags(flags)
	{
		// Allocate compressed reservation info area (avoid SPU MMIO area)
		if (addr != 0xe0000000)
		{
			// Beginning of the address space
			if (addr == 0x10000)
			{
				utils::memory_commit(g_reservations, 0x1000);
				utils::memory_commit(g_reservations2, 0x1000);
			}

			utils::memory_commit(g_reservations + addr / 16, size / 16);
			utils::memory_commit(g_reservations2 + addr / 16, size / 16);
		}
		else
		{
			// RawSPU LS
			for (u32 i = 0; i < 6; i++)
			{
				utils::memory_commit(g_reservations + addr / 16 + i * 0x10000, 0x4000);
				utils::memory_commit(g_reservations2 + addr / 16 + i * 0x10000, 0x4000);
			}

			// End of the address space
			utils::memory_commit(g_reservations + 0xfff0000, 0x10000);
			utils::memory_commit(g_reservations2 + 0xfff0000, 0x10000);
		}
	}

	block_t::~block_t()
	{
		{
			vm::writer_lock lock(0);

			// Deallocate all memory
			for (auto& entry : m_map)
			{
				_page_unmap(entry.first, *entry.second);
			}
		}

		// Notify rsx to invalidate range (TODO)
		if (const auto rsxthr = fxm::check_unlocked<GSRender>())
		{
			rsxthr->on_notify_memory_unmapped(addr, size);
		}
	}

	u32 block_t::alloc(const u32 orig_size, u32 align, const std::shared_ptr<utils::shm>* src)
	{
		vm::writer_lock lock(0);

		// Align to minimal page size
		const u32 size = ::align(orig_size, 0x10000);

		// Check alignment (it's page allocation, so passing small values there is just silly)
		if (align < 0x10000 || align != (0x80000000u >> cntlz32(align, true)))
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

		// Create or import shared memory object
		std::shared_ptr<utils::shm> shm = src ? std::shared_ptr<utils::shm>(*src) : std::make_shared<utils::shm>(size);

		// Search for an appropriate place (unoptimized)
		for (u32 addr = ::align(this->addr, align); addr < this->addr + this->size - 1; addr += align)
		{
			if (try_alloc(addr, pflags, std::move(shm)))
			{
				return addr;
			}
		}

		return 0;
	}

	u32 block_t::falloc(u32 addr, const u32 orig_size, const std::shared_ptr<utils::shm>* src)
	{
		vm::writer_lock lock(0);

		// align to minimal page size
		const u32 size = ::align(orig_size, 0x10000);

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

		if (!try_alloc(addr, pflags, src ? std::shared_ptr<utils::shm>(*src) : std::make_shared<utils::shm>(size)))
		{
			return 0;
		}

		return addr;
	}

	u32 block_t::dealloc(u32 addr, const std::shared_ptr<utils::shm>* src)
	{
		u32 result = 0;
		{
			vm::writer_lock lock(0);

			const auto found = m_map.find(addr);

			if (found == m_map.end())
			{
				return 0;
			}

			if (src && found->second.get() != src->get())
			{
				return 0;
			}

			result = found->second->size();

			// Unmap "real" memory pages
			_page_unmap(addr, *found->second);

			// Remove entry
			m_map.erase(found);
		}

		// Notify rsx to invalidate range (TODO)
		if (const auto rsxthr = fxm::check_unlocked<GSRender>())
		{
			rsxthr->on_notify_memory_unmapped(addr, result);
		}

		return result;
	}

	std::pair<const u32, std::shared_ptr<utils::shm>> block_t::get(u32 addr, u32 size)
	{
		if (addr < this->addr || std::max<u32>(size, addr - this->addr + size) >= this->size)
		{
			return {addr, nullptr};
		}

		vm::reader_lock lock;

		const auto upper = m_map.upper_bound(addr);

		if (upper == m_map.begin())
		{
			return {addr, nullptr};
		}

		const auto found = std::prev(upper);

		// Exact address condition (size == 0)
		if (size == 0 && found->first != addr)
		{
			return {addr, nullptr};
		}

		// Range check
		if (std::max<u32>(size, addr - found->first + size) > found->second->size())
		{
			return {addr, nullptr};
		}

		return *found;
	}

	u32 block_t::imp_used(const vm::writer_lock&)
	{
		u32 result = 0;

		for (auto& entry : m_map)
		{
			result += entry.second->size();
		}

		return result;
	}

	u32 block_t::used()
	{
		vm::writer_lock lock(0);

		return imp_used(lock);
	}

	std::shared_ptr<block_t> map(u32 addr, u32 size, u64 flags)
	{
		vm::writer_lock lock(0);

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
		vm::writer_lock lock(0);

		for (auto it = g_locations.begin(); it != g_locations.end(); it++)
		{
			if (*it && (*it)->addr == addr)
			{
				if (must_be_empty && (!it->unique() || (*it)->imp_used(lock)))
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
		vm::reader_lock lock;

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

	inline namespace ps3_
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
				std::make_shared<block_t>(0x30000000, 0x10000000), // main extend
			};
		}
	}

	void close()
	{
		g_locations.clear();

		utils::memory_decommit(g_base_addr, 0x100000000);
		utils::memory_decommit(g_exec_addr, 0x100000000);
		utils::memory_decommit(g_stat_addr, 0x100000000);
		utils::memory_decommit(g_reservations, 0x100000000);
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
