#include "stdafx.h"
#include "vm_locking.h"
#include "vm_ptr.h"
#include "vm_ref.h"
#include "vm_reservation.h"
#include "vm_var.h"

#include "Utilities/mutex.h"
#include "Utilities/Thread.h"
#include "Utilities/address_range.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Cell/SPURecompiler.h"
#include "Emu/perf_meter.hpp"
#include <thread>
#include <deque>
#include <shared_mutex>

#include "util/vm.hpp"
#include "util/asm.hpp"

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

	// Reservation stats
	alignas(4096) u8 g_reservations[65536 / 128 * 64]{0};

	// Pointers to shared memory mirror or zeros for "normal" memory
	alignas(4096) atomic_t<u64> g_shmem[65536]{0};

	// Memory locations
	alignas(64) std::vector<std::shared_ptr<block_t>> g_locations;

	// Memory mutex core
	shared_mutex g_mutex;

	// Memory mutex acknowledgement
	thread_local atomic_t<cpu_thread*>* g_tls_locked = nullptr;

	// "Unique locked" range lock, as opposed to "shared" range locks from set
	atomic_t<u64> g_range_lock = 0;

	// Memory mutex: passive locks
	std::array<atomic_t<cpu_thread*>, g_cfg.core.ppu_threads.max> g_locks{};

	// Range lock slot allocation bits
	atomic_t<u64> g_range_lock_bits{};

	// Memory range lock slots (sparse atomics)
	atomic_t<u64, 64> g_range_lock_set[64]{};

	// Page information
	struct memory_page
	{
		// Memory flags
		atomic_t<u8> flags;
	};

	// Memory pages
	std::array<memory_page, 0x100000000 / 4096> g_pages{};

	std::pair<bool, u64> try_reservation_update(u32 addr)
	{
		// Update reservation info with new timestamp
		auto& res = reservation_acquire(addr, 1);
		const u64 rtime = res;

		return {!(rtime & vm::rsrv_unique_lock) && res.compare_and_swap_test(rtime, rtime + 128), rtime};
	}

	void reservation_update(u32 addr)
	{
		u64 old = UINT64_MAX;
		const auto cpu = get_current_cpu_thread();

		while (true)
		{
			const auto [ok, rtime] = try_reservation_update(addr);

			if (ok || (old & -128) < (rtime & -128))
			{
				return;
			}

			old = rtime;

			if (cpu && cpu->test_stopped())
			{
				return;
			}
		}
	}

	static void _register_lock(cpu_thread* _cpu)
	{
		for (u32 i = 0, max = g_cfg.core.ppu_threads;;)
		{
			if (!g_locks[i] && g_locks[i].compare_and_swap_test(nullptr, _cpu))
			{
				g_tls_locked = g_locks.data() + i;
				break;
			}

			if (++i == max) i = 0;
		}
	}

	atomic_t<u64, 64>* alloc_range_lock()
	{
		const auto [bits, ok] = g_range_lock_bits.fetch_op([](u64& bits)
		{
			if (~bits) [[likely]]
			{
				bits |= bits + 1;
				return true;
			}

			return false;
		});

		if (!ok) [[unlikely]]
		{
			fmt::throw_exception("Out of range lock bits");
		}

		return &g_range_lock_set[std::countr_one(bits)];
	}

	void range_lock_internal(atomic_t<u64, 64>* range_lock, u32 begin, u32 size)
	{
		perf_meter<"RHW_LOCK"_u64> perf0;

		auto _cpu = get_current_cpu_thread();

		if (_cpu)
		{
			_cpu->state += cpu_flag::wait + cpu_flag::temp;
		}

		for (u64 i = 0;; i++)
		{
			range_lock->store(begin | (u64{size} << 32));

			const u64 lock_val = g_range_lock.load();
			const u64 is_share = g_shmem[begin >> 16].load();

			u64 lock_addr = static_cast<u32>(lock_val); // -> u64
			u32 lock_size = static_cast<u32>(lock_val << range_bits >> (range_bits + 32));

			u64 addr = begin;

			if ((lock_val & range_full_mask) == range_locked) [[likely]]
			{
				lock_size = 128;

				if (is_share)
				{
					addr = static_cast<u16>(addr) | is_share;
					lock_addr = lock_val;
				}
			}

			if (addr + size <= lock_addr || addr >= lock_addr + lock_size) [[likely]]
			{
				const u64 new_lock_val = g_range_lock.load();

				if (!new_lock_val || new_lock_val == lock_val) [[likely]]
				{
					break;
				}
			}

			// Wait a bit before accessing g_mutex
			range_lock->store(0);
			busy_wait(200);

			std::shared_lock lock(g_mutex, std::try_to_lock);

			if (!lock && i < 15)
			{
				busy_wait(200);
				continue;
			}
			else if (!lock)
			{
				lock.lock();
			}

			u32 test = 0;

			for (u32 i = begin / 4096, max = (begin + size - 1) / 4096; i <= max; i++)
			{
				if (!(g_pages[i].flags & (vm::page_readable)))
				{
					test = i * 4096;
					break;
				}
			}

			if (test)
			{
				lock.unlock();

				// Try tiggering a page fault (write)
				// TODO: Read memory if needed
				vm::_ref<atomic_t<u8>>(test) += 0;
				continue;
			}

			range_lock->release(begin | (u64{size} << 32));
			break;
		}

		if (_cpu)
		{
			_cpu->check_state();
		}
	}

	void free_range_lock(atomic_t<u64, 64>* range_lock) noexcept
	{
		if (range_lock < g_range_lock_set || range_lock >= std::end(g_range_lock_set))
		{
			fmt::throw_exception("Invalid range lock" HERE);
		}

		range_lock->release(0);

		// Use ptr difference to determine location
		const auto diff = range_lock - g_range_lock_set;
		g_range_lock_bits &= ~(1ull << diff);
	}

	template <typename F>
	FORCE_INLINE static u64 for_all_range_locks(u64 input, F func)
	{
		u64 result = input;

		for (u64 bits = input; bits; bits &= bits - 1)
		{
			const u32 id = std::countr_zero(bits);

			const u64 lock_val = g_range_lock_set[id].load();

			if (const u32 size = static_cast<u32>(lock_val >> 32)) [[unlikely]]
			{
				const u32 addr = static_cast<u32>(lock_val);

				if (func(addr, size)) [[unlikely]]
				{
					continue;
				}
			}

			result &= ~(1ull << id);
		}

		return result;
	}

	static void _lock_main_range_lock(u64 flags, u32 addr, u32 size)
	{
		// Shouldn't really happen
		if (size == 0)
		{
			vm_log.warning("Tried to lock empty range (flags=0x%x, addr=0x%x)" HERE, flags >> 32, addr);
			g_range_lock.release(0);
			return;
		}

		// Limit to 256 MiB at once; make sure if it operates on big amount of data, it's page-aligned
		if (size > 256 * 1024 * 1024 || (size > 65536 && size % 4096))
		{
			fmt::throw_exception("Failed to lock range (flags=0x%x, addr=0x%x, size=0x%x)" HERE, flags >> 32, addr, size);
		}

		// Block or signal new range locks
		g_range_lock = addr | u64{size} << 32 | flags;

		utils::prefetch_read(g_range_lock_set + 0);
		utils::prefetch_read(g_range_lock_set + 2);
		utils::prefetch_read(g_range_lock_set + 4);

		const auto range = utils::address_range::start_length(addr, size);

		u64 to_clear = g_range_lock_bits.load();

		while (to_clear)
		{
			to_clear = for_all_range_locks(to_clear, [&](u32 addr2, u32 size2)
			{
				ASSUME(size2);

				if (range.overlaps(utils::address_range::start_length(addr2, size2))) [[unlikely]]
				{
					return 1;
				}

				return 0;
			});

			if (!to_clear) [[likely]]
			{
				break;
			}

			_mm_pause();
		}
	}

	void passive_lock(cpu_thread& cpu)
	{
		bool ok = true;

		if (!g_tls_locked || *g_tls_locked != &cpu) [[unlikely]]
		{
			_register_lock(&cpu);

			if (cpu.state & cpu_flag::memory) [[likely]]
			{
				cpu.state -= cpu_flag::memory;
			}

			if (g_mutex.is_lockable())
			{
				return;
			}

			ok = false;
		}

		if (!ok || cpu.state & cpu_flag::memory)
		{
			while (true)
			{
				g_mutex.lock_unlock();
				cpu.state -= cpu_flag::memory;

				if (g_mutex.is_lockable()) [[likely]]
				{
					return;
				}
			}
		}
	}

	void passive_unlock(cpu_thread& cpu)
	{
		if (auto& ptr = g_tls_locked)
		{
			ptr->release(nullptr);
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
		if (!(cpu.state & cpu_flag::wait)) cpu.state += cpu_flag::wait;

		if (g_tls_locked && g_tls_locked->compare_and_swap_test(&cpu, nullptr))
		{
			cpu.state += cpu_flag::memory;
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

		if (cpu)
		{
			if (!g_tls_locked || *g_tls_locked != cpu || cpu->state & cpu_flag::wait)
			{
				cpu = nullptr;
			}
			else
			{
				cpu->state += cpu_flag::wait;
			}
		}

		g_mutex.lock_shared();

		if (cpu)
		{
			cpu->state -= cpu_flag::memory + cpu_flag::wait;
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

	writer_lock::writer_lock(u32 addr /*mutable*/)
	{
		auto cpu = get_current_cpu_thread();

		if (cpu)
		{
			if (!g_tls_locked || *g_tls_locked != cpu || cpu->state & cpu_flag::wait)
			{
				cpu = nullptr;
			}
			else
			{
				cpu->state += cpu_flag::wait;
			}
		}

		g_mutex.lock();

		if (addr >= 0x10000)
		{
			perf_meter<"SUSPEND"_u64> perf0;

			for (auto lock = g_locks.cbegin(), end = lock + g_cfg.core.ppu_threads; lock != end; lock++)
			{
				if (auto ptr = +*lock; ptr && !(ptr->state & cpu_flag::memory))
				{
					ptr->state.test_and_set(cpu_flag::memory);
				}
			}

			u64 addr1 = addr;

			if (u64 is_shared = g_shmem[addr >> 16]) [[unlikely]]
			{
				// Reservation address in shareable memory range
				addr1 = static_cast<u16>(addr) | is_shared;
			}

			g_range_lock = addr | range_locked;

			utils::prefetch_read(g_range_lock_set + 0);
			utils::prefetch_read(g_range_lock_set + 2);
			utils::prefetch_read(g_range_lock_set + 4);

			u64 to_clear = g_range_lock_bits.load();

			u64 point = addr1 / 128;

			while (true)
			{
				to_clear = for_all_range_locks(to_clear, [&](u64 addr2, u32 size2)
				{
					// TODO (currently not possible): handle 2 64K pages (inverse range), or more pages
					if (u64 is_shared = g_shmem[addr2 >> 16]) [[unlikely]]
					{
						addr2 = static_cast<u16>(addr2) | is_shared;
					}

					if (point - (addr2 / 128) <= (addr2 + size2 - 1) / 128 - (addr2 / 128)) [[unlikely]]
					{
						return 1;
					}

					return 0;
				});

				if (!to_clear) [[likely]]
				{
					break;
				}

				_mm_pause();
			}

			for (auto lock = g_locks.cbegin(), end = lock + g_cfg.core.ppu_threads; lock != end; lock++)
			{
				if (auto ptr = +*lock)
				{
					while (!(ptr->state & cpu_flag::wait))
						_mm_pause();
				}
			}
		}

		if (cpu)
		{
			cpu->state -= cpu_flag::memory + cpu_flag::wait;
		}
	}

	writer_lock::~writer_lock()
	{
		g_range_lock.release(0);
		g_mutex.unlock();
	}

	u64 reservation_lock_internal(u32 addr, atomic_t<u64>& res)
	{
		for (u64 i = 0;; i++)
		{
			if (u64 rtime = res; !(rtime & 127) && reservation_try_lock(res, rtime)) [[likely]]
			{
				return rtime;
			}

			if (auto cpu = get_current_cpu_thread(); cpu && cpu->state)
			{
				cpu->check_state();
			}
			else if (i < 15)
			{
				busy_wait(500);
			}
			else
			{
				// TODO: Accurate locking in this case
				if (!(g_pages[addr / 4096].flags & page_writable))
				{
					return -1;
				}

				std::this_thread::yield();
			}
		}
	}

	void reservation_shared_lock_internal(atomic_t<u64>& res)
	{
		for (u64 i = 0;; i++)
		{
			auto [_oldd, _ok] = res.fetch_op([&](u64& r)
			{
				if (r & rsrv_unique_lock)
				{
					return false;
				}

				r += 1;
				return true;
			});

			if (_ok) [[likely]]
			{
				return;
			}

			if (auto cpu = get_current_cpu_thread(); cpu && cpu->state)
			{
				cpu->check_state();
			}
			else if (i < 15)
			{
				busy_wait(500);
			}
			else
			{
				std::this_thread::yield();
			}
		}
	}

	void reservation_op_internal(u32 addr, std::function<bool()> func)
	{
		auto& res = vm::reservation_acquire(addr, 1);
		auto* ptr = vm::get_super_ptr(addr & -128);

		cpu_thread::suspend_all(get_current_cpu_thread(), {ptr, ptr + 64, &res}, [&]
		{
			if (func())
			{
				// Success, release the lock and progress
				res += 127;
			}
			else
			{
				// Only release the lock on failure
				res -= 1;
			}
		});
	}

	void reservation_escape_internal()
	{
		const auto _cpu = get_current_cpu_thread();

		if (_cpu && _cpu->id_type() == 1)
		{
			// TODO: PPU g_escape
		}

		if (_cpu && _cpu->id_type() == 2)
		{
			spu_runtime::g_escape(static_cast<spu_thread*>(_cpu));
		}

		thread_ctrl::emergency_exit("vm::reservation_escape");
	}

	static void _page_map(u32 addr, u8 flags, u32 size, utils::shm* shm, std::pair<const u32, std::pair<u32, std::shared_ptr<utils::shm>>>* (*search_shm)(vm::block_t* block, utils::shm* shm))
	{
		perf_meter<"PAGE_MAP"_u64> perf0;

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

		// Lock range being mapped
		_lock_main_range_lock(range_allocation, addr, size);

		if (shm && shm->flags() != 0 && shm->info++)
		{
			// Check ref counter (using unused member info for it)
			if (shm->info == 2)
			{
				// Allocate shm object for itself
				u64 shm_self = reinterpret_cast<u64>(shm->map_self()) ^ range_locked;

				// Pre-set range-locked flag (real pointers are 47 bits)
				// 1. To simplify range_lock logic
				// 2. To make sure it never overlaps with 32-bit addresses
				// Also check that it's aligned (lowest 16 bits)
				verify(HERE), (shm_self & 0xffff'8000'0000'ffff) == range_locked;

				// Find another mirror and map it as shareable too
				for (auto& ploc : g_locations)
				{
					if (auto loc = ploc.get())
					{
						if (auto pp = search_shm(loc, shm))
						{
							auto& [size2, ptr] = pp->second;

							for (u32 i = pp->first / 65536; i < pp->first / 65536 + size2 / 65536; i++)
							{
								g_shmem[i].release(shm_self);

								// Advance to the next position
								shm_self += 0x10000;
							}
						}
					}
				}

				// Unsharing only happens on deallocation currently, so make sure all further refs are shared
				shm->info = UINT32_MAX;
			}

			// Obtain existing pointer
			u64 shm_self = reinterpret_cast<u64>(shm->get()) ^ range_locked;

			// Check (see above)
			verify(HERE), (shm_self & 0xffff'8000'0000'ffff) == range_locked;

			// Map range as shareable
			for (u32 i = addr / 65536; i < addr / 65536 + size / 65536; i++)
			{
				g_shmem[i].release(std::exchange(shm_self, shm_self + 0x10000));
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

		// Unlock
		g_range_lock.release(0);
	}

	bool page_protect(u32 addr, u32 size, u8 flags_test, u8 flags_set, u8 flags_clear)
	{
		perf_meter<"PAGE_PRO"_u64> perf0;

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

		// Choose some impossible value (not valid without page_allocated)
		u8 start_value = page_executable;

		for (u32 start = addr / 4096, end = start + size / 4096, i = start; i < end + 1; i++)
		{
			u8 new_val = page_executable;

			if (i < end)
			{
				new_val = g_pages[i].flags;
				new_val |= flags_set;
				new_val &= ~flags_clear;
			}

			if (new_val != start_value)
			{
				const u8 old_val = g_pages[start].flags;

				if (u32 page_size = (i - start) * 4096; page_size && old_val != start_value)
				{
					u64 safe_bits = 0;

					if (old_val & start_value & page_readable)
						safe_bits |= range_readable;
					if (old_val & start_value & page_writable && safe_bits & range_readable)
						safe_bits |= range_writable;
					if (old_val & start_value & page_executable && safe_bits & range_readable)
						safe_bits |= range_executable;

					// Protect range locks from observing changes in memory protection
					_lock_main_range_lock(safe_bits, start * 4096, page_size);

					for (u32 j = start; j < i; j++)
					{
						g_pages[j].flags.release(start_value);
					}

					if ((old_val ^ start_value) & (page_readable | page_writable))
					{
						const auto protection = start_value & page_writable ? utils::protection::rw : (start_value & page_readable ? utils::protection::ro : utils::protection::no);
						utils::memory_protect(g_base_addr + start * 4096, page_size, protection);
					}
				}
				else
				{
					g_range_lock.release(0);
				}

				start_value = new_val;
				start = i;
			}
		}

		g_range_lock.release(0);

		return true;
	}

	static u32 _page_unmap(u32 addr, u32 max_size, utils::shm* shm)
	{
		perf_meter<"PAGE_UNm"_u64> perf0;

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

		// Protect range locks from actual memory protection changes
		_lock_main_range_lock(range_allocation, addr, size);

		if (shm && shm->flags() != 0 && g_shmem[addr >> 16])
		{
			shm->info--;

			for (u32 i = addr / 65536; i < addr / 65536 + size / 65536; i++)
			{
				g_shmem[i].release(0);
			}
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (!(g_pages[i].flags & page_allocated))
			{
				fmt::throw_exception("Concurrent access (addr=0x%x, size=0x%x, current_addr=0x%x)" HERE, addr, size, i * 4096);
			}

			g_pages[i].flags.release(0);
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
			std::memset(g_sudo_addr + addr, 0, size);
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

		// Unlock
		g_range_lock.release(0);

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

		// Map "real" memory pages; provide a function to search for mirrors with private member access
		_page_map(page_addr, flags, page_size, shm.get(), [](vm::block_t* _this, utils::shm* shm)
		{
			decltype(m_map)::value_type* result = nullptr;

			// Check eligibility
			if (!_this || !(SYS_MEMORY_PAGE_SIZE_MASK & _this->flags) || _this->addr < 0x20000000 || _this->addr >= 0xC0000000)
			{
				return result;
			}

			for (auto& pp : _this->m_map)
			{
				if (pp.second.second.get() == shm)
				{
					// Found match
					return &pp;
				}
			}

			return result;
		});

		// Add entry
		m_map[addr] = std::make_pair(size, std::move(shm));

		return true;
	}

	block_t::block_t(u32 addr, u32 size, u64 flags)
		: addr(addr)
		, size(size)
		, flags(flags)
	{
		if (flags & 0x100)
		{
			// Special path for 4k-aligned pages
			m_common = std::make_shared<utils::shm>(size);
			verify(HERE), m_common->map_critical(vm::base(addr), utils::protection::no) == vm::base(addr);
			verify(HERE), m_common->map_critical(vm::get_super_ptr(addr)) == vm::get_super_ptr(addr);
		}
	}

	block_t::~block_t()
	{
		{
			vm::writer_lock lock(0);

			// Deallocate all memory
			for (auto it = m_map.begin(), end = m_map.end(); it != end;)
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
		if (align < min_page_size || align != (0x80000000u >> std::countl_zero(align)))
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
		const auto range = utils::address_range::start_length(addr, size);

		if (!range.valid())
		{
			return false;
		}

		for (auto& block : g_locations)
		{
			if (!block)
			{
				continue;
			}

			if (range.overlaps(utils::address_range::start_length(block->addr, block->size)))
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
		if (align < 0x10000 || align != (0x80000000u >> std::countl_zero(align)))
		{
			fmt::throw_exception("Invalid alignment (size=0x%x, align=0x%x)" HERE, size, align);
		}

		// Return if size is invalid
		if (!size)
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

			if (size <= 16 && (size & (size - 1)) == 0 && (addr & (size - 1)) == 0)
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
			g_reservations, g_reservations + sizeof(g_reservations) - 1);

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

			std::memset(g_reservations, 0, sizeof(g_reservations));
			std::memset(g_shmem, 0, sizeof(g_shmem));
			std::memset(g_range_lock_set, 0, sizeof(g_range_lock_set));
			g_range_lock_bits = 0;
		}
	}

	void close()
	{
		g_locations.clear();

		utils::memory_decommit(g_base_addr, 0x100000000);
		utils::memory_decommit(g_exec_addr, 0x100000000);
		utils::memory_decommit(g_stat_addr, 0x100000000);
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
