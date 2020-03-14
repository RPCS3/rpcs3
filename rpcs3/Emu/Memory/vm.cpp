#include "stdafx.h"
#include "vm_locking.h"
#include "vm_ptr.h"
#include "vm_ref.h"
#include "vm_reservation.h"
#include "vm_var.h"

#include "Utilities/mutex.h"
#include "Utilities/cond.h"
#include "Utilities/Thread.h"
#include "Utilities/VirtualMemory.h"
#include "Utilities/asm.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/RSX/GSRender.h"
#include <atomic>
#include <thread>
#include <deque>

LOG_CHANNEL(vm_log, "VM");

namespace vm
{
	static u8* memory_reserve_4GiB(void* _addr, u64 size = 0x100000000)
	{
		for (u64 addr = reinterpret_cast<u64>(_addr) + 0x100000000;; addr += 0x100000000)
		{
			if (auto ptr = utils::memory_reserve(size, reinterpret_cast<void*>(addr)))
			{
				return static_cast<u8*>(ptr);
			}
		}

		// TODO: a condition to break loop
		return static_cast<u8*>(utils::memory_reserve(size));
	}

	// Emulated virtual memory
	u8* const g_base_addr = memory_reserve_4GiB(reinterpret_cast<void*>(0x2'0000'0000));

	// Unprotected virtual memory mirror
	u8* const g_sudo_addr = memory_reserve_4GiB(g_base_addr);

	// Auxiliary virtual memory for executable areas
	u8* const g_exec_addr = memory_reserve_4GiB(g_sudo_addr, 0x200000000);

	// Stats for debugging
	u8* const g_stat_addr = memory_reserve_4GiB(g_exec_addr);

	// Reservation stats (compressed x16)
	u8* const g_reservations = memory_reserve_4GiB(g_stat_addr);

	// Memory locations
	std::vector<std::shared_ptr<block_t>> g_locations;

	// Memory mutex core
	shared_mutex g_mutex;

	// Memory mutex acknowledgement
	thread_local atomic_t<cpu_thread*>* g_tls_locked = nullptr;

	// Currently locked cache line
	atomic_t<u32> g_addr_lock = 0;

	// Memory mutex: passive locks
	std::array<atomic_t<cpu_thread*>, g_cfg.core.ppu_threads.max> g_locks{};
	std::array<atomic_t<u64>, 6> g_range_locks{};

	static void _register_lock(cpu_thread* _cpu)
	{
		for (u32 i = 0, max = g_cfg.core.ppu_threads;;)
		{
			if (!g_locks[i] && g_locks[i].compare_and_swap_test(nullptr, _cpu))
			{
				g_tls_locked = g_locks.data() + i;
				return;
			}

			if (++i == max) i = 0;
		}
	}

	static atomic_t<u64>* _register_range_lock(const u64 lock_info)
	{
		while (true)
		{
			for (auto& lock : g_range_locks)
			{
				if (!lock && lock.compare_and_swap_test(0, lock_info))
				{
					return &lock;
				}
			}
		}
	}

	void passive_lock(cpu_thread& cpu)
	{
		if (g_tls_locked && *g_tls_locked == &cpu) [[unlikely]]
		{
			return;
		}

		if (g_mutex.is_lockable()) [[likely]]
		{
			// Optimistic path (hope that mutex is not exclusively locked)
			_register_lock(&cpu);

			if (g_mutex.is_lockable()) [[likely]]
			{
				return;
			}

			passive_unlock(cpu);
		}

		::reader_lock lock(g_mutex);
		_register_lock(&cpu);
	}

	atomic_t<u64>* passive_lock(const u32 addr, const u32 end)
	{
		static const auto test_addr = [](const u32 target, const u32 addr, const u32 end)
		{
			return addr > target || end <= target;
		};

		atomic_t<u64>* _ret;

		if (test_addr(g_addr_lock.load(), addr, end)) [[likely]]
		{
			// Optimistic path (hope that address range is not locked)
			_ret = _register_range_lock(u64{end} << 32 | addr);

			if (test_addr(g_addr_lock.load(), addr, end)) [[likely]]
			{
				return _ret;
			}

			*_ret = 0;
		}

		{
			::reader_lock lock(g_mutex);
			_ret = _register_range_lock(u64{end} << 32 | addr);
		}

		return _ret;
	}

	void passive_unlock(cpu_thread& cpu)
	{
		if (auto& ptr = g_tls_locked)
		{
			*ptr = nullptr;
			ptr = nullptr;

			if (cpu.state & cpu_flag::memory)
			{
				cpu.state -= cpu_flag::memory;
			}
		}
	}

	void cleanup_unlock(cpu_thread& cpu) noexcept
	{
		for (u32 i = 0, max = g_cfg.core.ppu_threads; i < max; i++)
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
		cpu.state += cpu_flag::wait;

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
		if (m_upgraded)
		{
			g_mutex.unlock();
		}
		else
		{
			g_mutex.unlock_shared();
		}
	}

	void reader_lock::upgrade()
	{
		if (m_upgraded)
		{
			return;
		}

		g_mutex.lock_upgrade();
		m_upgraded = true;
	}

	writer_lock::writer_lock(u32 addr)
	{
		auto cpu = get_current_cpu_thread();

		if (!cpu || !g_tls_locked || !g_tls_locked->compare_and_swap_test(cpu, nullptr))
		{
			cpu = nullptr;
		}

		g_mutex.lock();

		if (addr)
		{
			for (auto lock = g_locks.cbegin(), end = lock + g_cfg.core.ppu_threads; lock != end; lock++)
			{
				if (cpu_thread* ptr = *lock)
				{
					ptr->state.test_and_set(cpu_flag::memory);
				}
			}

			g_addr_lock = addr;

			for (auto& lock : g_range_locks)
			{
				while (true)
				{
					const u64 value = lock;

					// Test beginning address
					if (static_cast<u32>(value) > addr)
					{
						break;
					}

					// Test end address
					if (static_cast<u32>(value >> 32) <= addr)
					{
						break;
					}

					_mm_pause();
				}
			}

			for (auto lock = g_locks.cbegin(), end = lock + g_cfg.core.ppu_threads; lock != end; lock++)
			{
				while (*lock)
				{
					_mm_pause();
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
		g_addr_lock.release(0);
		g_mutex.unlock();
	}

	void reservation_lock_internal(atomic_t<u64>& res)
	{
		for (u64 i = 0;; i++)
		{
			if (!res.bts(0)) [[likely]]
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

	static void _page_map(u32 addr, u8 flags, u32 size, utils::shm* shm)
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

		// Notify rsx that range has become valid
		// Note: This must be done *before* memory gets mapped while holding the vm lock, otherwise
		//       the RSX might try to invalidate memory that got unmapped and remapped
		if (const auto rsxthr = g_fxo->get<rsx::thread>())
		{
			rsxthr->on_notify_memory_mapped(addr, size);
		}

		if (!shm)
		{
			utils::memory_protect(g_base_addr + addr, size, utils::protection::rw);
			std::memset(g_base_addr + addr, 0, size);
		}
		else if (shm->map_critical(g_base_addr + addr) != g_base_addr + addr || shm->map_critical(g_sudo_addr + addr) != g_sudo_addr + addr)
		{
			fmt::throw_exception("Memory mapping failed - blame Windows (addr=0x%x, size=0x%x, flags=0x%x)", addr, size, flags);
		}

		if (flags & page_executable)
		{
			// TODO
			utils::memory_commit(g_exec_addr + addr * 2, size * 2);
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
				new_val = g_pages[i].flags;
				new_val |= flags_set;
				new_val &= ~flags_clear;

				g_pages[i].flags.release(new_val);
				new_val &= (page_readable | page_writable);
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

	static u32 _page_unmap(u32 addr, u32 max_size, utils::shm* shm)
	{
		if (!max_size || (max_size | addr) % 4096)
		{
			fmt::throw_exception("Invalid arguments (addr=0x%x, max_size=0x%x)" HERE, addr, max_size);
		}

		// Determine deallocation size
		u32 size = 0;
		bool is_exec = false;

		for (u32 i = addr / 4096; i < addr / 4096 + max_size / 4096; i++)
		{
			if ((g_pages[i].flags & page_allocated) == 0)
			{
				break;
			}

			if (size == 0)
			{
				is_exec = !!(g_pages[i].flags & page_executable);
			}
			else
			{
				// Must be consistent
				verify(HERE), is_exec == !!(g_pages[i].flags & page_executable);
			}

			size += 4096;
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (!(g_pages[i].flags.exchange(0) & page_allocated))
			{
				fmt::throw_exception("Concurrent access (addr=0x%x, size=0x%x, current_addr=0x%x)" HERE, addr, size, i * 4096);
			}
		}

		// Notify rsx to invalidate range
		// Note: This must be done *before* memory gets unmapped while holding the vm lock, otherwise
		//       the RSX might try to call VirtualProtect on memory that is already unmapped
		if (const auto rsxthr = g_fxo->get<rsx::thread>())
		{
			rsxthr->on_notify_memory_unmapped(addr, size);
		}

		// Actually unmap memory
		if (!shm)
		{
			utils::memory_protect(g_base_addr + addr, size, utils::protection::no);
		}
		else
		{
			shm->unmap_critical(g_base_addr + addr);
			shm->unmap_critical(g_sudo_addr + addr);
		}

		if (is_exec)
		{
			utils::memory_decommit(g_exec_addr + addr * 2, size * 2);
		}

		if (g_cfg.core.ppu_debug)
		{
			utils::memory_decommit(g_stat_addr + addr, size);
		}

		return size;
	}

	bool check_addr(u32 addr, u32 size, u8 flags)
	{
		// Overflow checking
		if (addr + size < addr && (addr + size) != 0)
		{
			return false;
		}

		// Always check this flag
		flags |= page_allocated;

		for (u32 i = addr / 4096, max = (addr + size - 1) / 4096; i <= max; i++)
		{
			if ((g_pages[i].flags & flags) != flags) [[unlikely]]
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
			fmt::throw_exception("Invalid memory location (%u)" HERE, +location);
		}

		return block->alloc(size, align);
	}

	u32 falloc(u32 addr, u32 size, memory_location_t location)
	{
		const auto block = get(location, addr);

		if (!block)
		{
			fmt::throw_exception("Invalid memory location (%u, addr=0x%x)" HERE, +location, addr);
		}

		return block->falloc(addr, size);
	}

	u32 dealloc(u32 addr, memory_location_t location)
	{
		const auto block = get(location, addr);

		if (!block)
		{
			fmt::throw_exception("Invalid memory location (%u, addr=0x%x)" HERE, +location, addr);
		}

		return block->dealloc(addr);
	}

	void dealloc_verbose_nothrow(u32 addr, memory_location_t location) noexcept
	{
		const auto block = get(location, addr);

		if (!block)
		{
			vm_log.error("vm::dealloc(): invalid memory location (%u, addr=0x%x)\n", +location, addr);
			return;
		}

		if (!block->dealloc(addr))
		{
			vm_log.error("vm::dealloc(): deallocation failed (addr=0x%x)\n", addr);
			return;
		}
	}

	bool block_t::try_alloc(u32 addr, u8 flags, u32 size, std::shared_ptr<utils::shm>&& shm)
	{
		// Check if memory area is already mapped
		for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
		{
			if (g_pages[i].flags)
			{
				return false;
			}
		}

		const u32 page_addr = addr + (this->flags & 0x10 ? 0x1000 : 0);
		const u32 page_size = size - (this->flags & 0x10 ? 0x2000 : 0);

		if (this->flags & 0x10)
		{
			// Mark overflow/underflow guard pages as allocated
			verify(HERE), !g_pages[addr / 4096].flags.exchange(page_allocated);
			verify(HERE), !g_pages[addr / 4096 + size / 4096 - 1].flags.exchange(page_allocated);
		}

		// Map "real" memory pages
		_page_map(page_addr, flags, page_size, shm.get());

		// Add entry
		m_map[addr] = std::make_pair(size, std::move(shm));

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
			}

			utils::memory_commit(g_reservations + addr / 16, size / 16);
		}
		else
		{
			// RawSPU LS
			for (u32 i = 0; i < 6; i++)
			{
				utils::memory_commit(g_reservations + addr / 16 + i * 0x10000, 0x4000);
			}

			// End of the address space
			utils::memory_commit(g_reservations + 0xfff0000, 0x10000);
		}

		if (flags & 0x100)
		{
			// Special path for 4k-aligned pages
			m_common = std::make_shared<utils::shm>(size);
			verify(HERE), m_common->map_critical(vm::base(addr), utils::protection::no) == vm::base(addr);
			verify(HERE), m_common->map_critical(vm::get_super_ptr(addr), utils::protection::rw) == vm::get_super_ptr(addr);
		}
	}

	block_t::~block_t()
	{
		{
			vm::writer_lock lock(0);

			// Deallocate all memory
			for (auto it = m_map.begin(), end = m_map.end(); !m_common && it != end;)
			{
				const auto next = std::next(it);
				const auto size = it->second.first;
				_page_unmap(it->first, size, it->second.second.get());
				it = next;
			}

			// Special path for 4k-aligned pages
			if (m_common)
			{
				m_common->unmap_critical(vm::base(addr));
				m_common->unmap_critical(vm::get_super_ptr(addr));
			}
		}
	}

	u32 block_t::alloc(const u32 orig_size, u32 align, const std::shared_ptr<utils::shm>* src, u64 flags)
	{
		if (!src)
		{
			// Use the block's flags
			flags = this->flags;
		}

		vm::writer_lock lock(0);

		// Determine minimal alignment
		const u32 min_page_size = flags & 0x100 ? 0x1000 : 0x10000;

		// Align to minimal page size
		const u32 size = ::align(orig_size, min_page_size) + (flags & 0x10 ? 0x2000 : 0);

		// Check alignment (it's page allocation, so passing small values there is just silly)
		if (align < min_page_size || align != (0x80000000u >> utils::cntlz32(align, true)))
		{
			fmt::throw_exception("Invalid alignment (size=0x%x, align=0x%x)" HERE, size, align);
		}

		// Return if size is invalid
		if (!orig_size || !size || orig_size > size || size > this->size)
		{
			return 0;
		}

		u8 pflags = page_readable | page_writable;

		if ((flags & SYS_MEMORY_PAGE_SIZE_64K) == SYS_MEMORY_PAGE_SIZE_64K)
		{
			pflags |= page_64k_size;
		}
		else if (!(flags & (SYS_MEMORY_PAGE_SIZE_MASK & ~SYS_MEMORY_PAGE_SIZE_1M)))
		{
			pflags |= page_1m_size;
		}

		// Create or import shared memory object
		std::shared_ptr<utils::shm> shm;

		if (m_common)
			verify(HERE), !src;
		else if (src)
			shm = *src;
		else
			shm = std::make_shared<utils::shm>(size);

		// Search for an appropriate place (unoptimized)
		for (u32 addr = ::align(this->addr, align); u64{addr} + size <= u64{this->addr} + this->size; addr += align)
		{
			if (try_alloc(addr, pflags, size, std::move(shm)))
			{
				return addr + (flags & 0x10 ? 0x1000 : 0);
			}
		}

		return 0;
	}

	u32 block_t::falloc(u32 addr, const u32 orig_size, const std::shared_ptr<utils::shm>* src, u64 flags)
	{
		if (!src)
		{
			// Use the block's flags
			flags = this->flags;
		}

		vm::writer_lock lock(0);

		// Determine minimal alignment
		const u32 min_page_size = flags & 0x100 ? 0x1000 : 0x10000;

		// Align to minimal page size
		const u32 size = ::align(orig_size, min_page_size);

		// return if addr or size is invalid
		if (!size || addr < this->addr || orig_size > size || addr + u64{size} > this->addr + u64{this->size} || flags & 0x10)
		{
			return 0;
		}

		u8 pflags = page_readable | page_writable;

		if ((flags & SYS_MEMORY_PAGE_SIZE_64K) == SYS_MEMORY_PAGE_SIZE_64K)
		{
			pflags |= page_64k_size;
		}
		else if (!(flags & (SYS_MEMORY_PAGE_SIZE_MASK & ~SYS_MEMORY_PAGE_SIZE_1M)))
		{
			pflags |= page_1m_size;
		}

		// Create or import shared memory object
		std::shared_ptr<utils::shm> shm;

		if (m_common)
			verify(HERE), !src;
		else if (src)
			shm = *src;
		else
			shm = std::make_shared<utils::shm>(size);

		if (!try_alloc(addr, pflags, size, std::move(shm)))
		{
			return 0;
		}

		return addr;
	}

	u32 block_t::dealloc(u32 addr, const std::shared_ptr<utils::shm>* src)
	{
		{
			vm::writer_lock lock(0);

			const auto found = m_map.find(addr - (flags & 0x10 ? 0x1000 : 0));

			if (found == m_map.end())
			{
				return 0;
			}

			if (src && found->second.second.get() != src->get())
			{
				return 0;
			}

			// Get allocation size
			const auto size = found->second.first - (flags & 0x10 ? 0x2000 : 0);

			if (flags & 0x10)
			{
				// Clear guard pages
				verify(HERE), g_pages[addr / 4096 - 1].flags.exchange(0) == page_allocated;
				verify(HERE), g_pages[addr / 4096 + size / 4096].flags.exchange(0) == page_allocated;
			}

			// Unmap "real" memory pages
			verify(HERE), size == _page_unmap(addr, size, found->second.second.get());

			// Remove entry
			m_map.erase(found);

			return size;
		}
	}

	std::pair<u32, std::shared_ptr<utils::shm>> block_t::get(u32 addr, u32 size)
	{
		if (addr < this->addr || addr + u64{size} > this->addr + u64{this->size})
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

		// Special path
		if (m_common)
		{
			return {this->addr, m_common};
		}

		// Range check
		if (addr + u64{size} > found->first + u64{found->second.second->size()})
		{
			return {addr, nullptr};
		}

		return {found->first, found->second.second};
	}

	u32 block_t::imp_used(const vm::writer_lock&)
	{
		u32 result = 0;

		for (auto& entry : m_map)
		{
			result += entry.second.first - (flags & 0x10 ? 0x2000 : 0);
		}

		return result;
	}

	u32 block_t::used()
	{
		vm::writer_lock lock(0);

		return imp_used(lock);
	}

	static bool _test_map(u32 addr, u32 size)
	{
		for (auto& block : g_locations)
		{
			if (block && block->addr >= addr && block->addr <= addr + size - 1)
			{
				return false;
			}

			if (block && addr >= block->addr && addr <= block->addr + block->size - 1)
			{
				return false;
			}
		}

		return true;
	}

	static std::shared_ptr<block_t> _find_map(u32 size, u32 align, u64 flags)
	{
		for (u32 addr = ::align<u32>(0x20000000, align); addr - 1 < 0xC0000000 - 1; addr += align)
		{
			if (_test_map(addr, size))
			{
				return std::make_shared<block_t>(addr, size, flags);
			}
		}

		return nullptr;
	}

	static std::shared_ptr<block_t> _map(u32 addr, u32 size, u64 flags)
	{
		if (!size || (size | addr) % 4096)
		{
			fmt::throw_exception("Invalid arguments (addr=0x%x, size=0x%x)" HERE, addr, size);
		}

		if (!_test_map(addr, size))
		{
			return nullptr;
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

	static std::shared_ptr<block_t> _get_map(memory_location_t location, u32 addr)
	{
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
			if (block && addr >= block->addr && addr <= block->addr + block->size - 1)
			{
				return block;
			}
		}

		return nullptr;
	}

	std::shared_ptr<block_t> map(u32 addr, u32 size, u64 flags)
	{
		vm::writer_lock lock(0);

		return _map(addr, size, flags);
	}

	std::shared_ptr<block_t> find_map(u32 orig_size, u32 align, u64 flags)
	{
		vm::writer_lock lock(0);

		// Align to minimal page size
		const u32 size = ::align(orig_size, 0x10000);

		// Check alignment
		if (align < 0x10000 || align != (0x80000000u >> utils::cntlz32(align, true)))
		{
			fmt::throw_exception("Invalid alignment (size=0x%x, align=0x%x)" HERE, size, align);
		}

		// Return if size is invalid
		if (!size || size > 0x40000000)
		{
			return nullptr;
		}

		auto block = _find_map(size, align, flags);

		if (block) g_locations.emplace_back(block);

		return block;
	}

	std::shared_ptr<block_t> unmap(u32 addr, bool must_be_empty)
	{
		vm::writer_lock lock(0);

		for (auto it = g_locations.begin() + memory_location_max; it != g_locations.end(); it++)
		{
			if (*it && (*it)->addr == addr)
			{
				if (must_be_empty && (*it)->flags & 0x3)
				{
					continue;
				}

				if (!must_be_empty && ((*it)->flags & 0x3) != 2)
				{
					continue;
				}

				if (must_be_empty && (it->use_count() != 1 || (*it)->imp_used(lock)))
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

		return _get_map(location, addr);
	}

	std::shared_ptr<block_t> reserve_map(memory_location_t location, u32 addr, u32 area_size, u64 flags)
	{
		vm::reader_lock lock;

		auto area = _get_map(location, addr);

		if (area)
		{
			return area;
		}

		lock.upgrade();

		// Allocation on arbitrary address
		if (location != any && location < g_locations.size())
		{
			// return selected location
			auto& loc = g_locations[location];

			if (!loc)
			{
				// Deferred allocation
				loc = _find_map(area_size, 0x10000000, flags);
			}

			return loc;
		}

		// Fixed address allocation
		area = _get_map(location, addr);

		if (area)
		{
			return area;
		}

		return _map(addr, area_size, flags);
	}

	bool try_access(u32 addr, void* ptr, u32 size, bool is_write)
	{
		vm::reader_lock lock;

		if (size == 0)
		{
			return true;
		}

		if (vm::check_addr(addr, size, is_write ? page_writable : page_readable))
		{
			void* src = vm::g_sudo_addr + addr;
			void* dst = ptr;

			if (is_write)
				std::swap(src, dst);

			if (size <= 16 && utils::popcnt32(size) == 1 && (addr & (size - 1)) == 0)
			{
				if (is_write)
				{
					switch (size)
					{
					case 1: atomic_storage<u8>::release(*static_cast<u8*>(dst), *static_cast<u8*>(src)); break;
					case 2: atomic_storage<u16>::release(*static_cast<u16*>(dst), *static_cast<u16*>(src)); break;
					case 4: atomic_storage<u32>::release(*static_cast<u32*>(dst), *static_cast<u32*>(src)); break;
					case 8: atomic_storage<u64>::release(*static_cast<u64*>(dst), *static_cast<u64*>(src)); break;
					case 16: _mm_store_si128(static_cast<__m128i*>(dst), _mm_loadu_si128(static_cast<__m128i*>(src))); break;
					}

					return true;
				}
			}

			std::memcpy(dst, src, size);
			return true;
		}

		return false;
	}

	inline namespace ps3_
	{
		void init()
		{
			vm_log.notice("Guest memory bases address ranges:\n"
			"vm::g_base_addr = %p - %p\n"
			"vm::g_sudo_addr = %p - %p\n"
			"vm::g_exec_addr = %p - %p\n"
			"vm::g_stat_addr = %p - %p\n"
			"vm::g_reservations = %p - %p\n",
			g_base_addr, g_base_addr + UINT32_MAX,
			g_sudo_addr, g_sudo_addr + UINT32_MAX,
			g_exec_addr, g_exec_addr + 0x200000000 - 1,
			g_stat_addr, g_stat_addr + UINT32_MAX,
			g_reservations, g_reservations + UINT32_MAX);

			g_locations =
			{
				std::make_shared<block_t>(0x00010000, 0x1FFF0000, 0x200), // main
				std::make_shared<block_t>(0x20000000, 0x10000000, 0x201), // user 64k pages
				nullptr, // user 1m pages
				nullptr, // rsx context
				std::make_shared<block_t>(0xC0000000, 0x10000000), // video
				std::make_shared<block_t>(0xD0000000, 0x10000000, 0x111), // stack
				std::make_shared<block_t>(0xE0000000, 0x20000000), // SPU reserved
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

void fmt_class_string<vm::_ptr_base<const void, u32>>::format(std::string& out, u64 arg)
{
	fmt_class_string<u32>::format(out, arg);
}

void fmt_class_string<vm::_ptr_base<const char, u32>>::format(std::string& out, u64 arg)
{
	// Special case (may be allowed for some arguments)
	if (arg == 0)
	{
		out += reinterpret_cast<const char*>(u8"«NULL»");
		return;
	}

	// Filter certainly invalid addresses (TODO)
	if (arg < 0x10000 || arg >= 0xf0000000)
	{
		out += reinterpret_cast<const char*>(u8"«INVALID_ADDRESS:");
		fmt_class_string<u32>::format(out, arg);
		out += reinterpret_cast<const char*>(u8"»");
		return;
	}

	const auto start = out.size();

	out += reinterpret_cast<const char*>(u8"“");

	for (vm::_ptr_base<const volatile char, u32> ptr = vm::cast(arg);; ptr++)
	{
		if (!vm::check_addr(ptr.addr()))
		{
			// TODO: optimize checks
			out.resize(start);
			out += reinterpret_cast<const char*>(u8"«INVALID_ADDRESS:");
			fmt_class_string<u32>::format(out, arg);
			out += reinterpret_cast<const char*>(u8"»");
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

	out += reinterpret_cast<const char*>(u8"”");
}
