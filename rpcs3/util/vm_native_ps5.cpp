// PS5: utils' virtual memory on the console (util/vm.hpp), in place of
// vm_native.cpp's POSIX backend.
//
// The console's measurements (PS5_PayloadSDK, platform/docs/PROBE.md): pages
// are 16 KiB; plain anonymous mappings are charged to the title's small
// flexible budget; ranges the size of RPCS3's guest layout (8, 12, 32 and
// 4 GiB) are granted from 0x10_0000_0000 up, exactly at the hint or not at all.
// The first RPCS3 title on the console stopped before main: memory_commit's
// mprotect on a range plain mmap had reserved failed (EINVAL).
//
// So everything goes through the console's direct memory, by the PS5 libre
// layer (libre/vm.h, the title's ps5/libre): reservations are its virtual
// ranges, committed memory its direct-memory units, and shared memory (the
// guest's RAM and its mirrors) its direct-memory blocks, mapped into the
// ranges, charged to the direct pool only.

#include "stdafx.h"
#include "util/vm.hpp"
#include "util/asm.hpp"

#include <libre/vm.h>

#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include <algorithm>
#include <mutex>
#include <vector>

LOG_CHANNEL(vm_log, "VM");

namespace utils
{
	namespace
	{
		// Measured: the console's page
		constexpr long ps5_page_size = LIBRE_VM_PAGE;

		// The console's CPU protection bits, which sceKernelMprotect takes as they are
		int ps5_protection(protection prot)
		{
			switch (prot)
			{
			case protection::rw: return LIBRE_VM_READ | LIBRE_VM_WRITE;
			case protection::ro: return LIBRE_VM_READ;
			case protection::no: return 0;
			case protection::wx: return LIBRE_VM_READ | LIBRE_VM_WRITE | LIBRE_VM_EXEC;
			case protection::rx: return LIBRE_VM_READ | LIBRE_VM_EXEC;
			}
			return 0;
		}

		int kernel_protection(protection prot)
		{
			return ps5_protection(prot);
		}

		int posix_protection(protection prot)
		{
			const int p = ps5_protection(prot);
			return (p & LIBRE_VM_READ ? PROT_READ : 0) | (p & LIBRE_VM_WRITE ? PROT_WRITE : 0) | (p & LIBRE_VM_EXEC ? PROT_EXEC : 0);
		}

		// The ranges memory_reserve gave out, so memory_protect knows reserved space
		// from mapped memory
		struct reserved_range
		{
			u64 base;
			u64 size;
		};

		std::mutex g_ranges_lock;
		std::vector<reserved_range> g_ranges;

		// The shared-memory views shm::map placed, which memory_protect must
		// never commit over, whatever the kernel's query says of them
		std::vector<reserved_range> g_views;

		bool is_view(u64 addr)
		{
			std::lock_guard lock(g_ranges_lock);
			return std::any_of(g_views.begin(), g_views.end(), [&](const reserved_range& r)
			{
				return addr >= r.base && addr - r.base < r.size;
			});
		}

		void add_view(void* base, u64 size)
		{
			std::lock_guard lock(g_ranges_lock);
			g_views.push_back({reinterpret_cast<u64>(base), size});
		}

		void remove_view(void* base)
		{
			std::lock_guard lock(g_ranges_lock);
			std::erase_if(g_views, [&](const reserved_range& r) { return r.base == reinterpret_cast<u64>(base); });
		}

		bool is_reserved(u64 addr)
		{
			std::lock_guard lock(g_ranges_lock);
			return std::any_of(g_ranges.begin(), g_ranges.end(), [&](const reserved_range& r)
			{
				return addr >= r.base && addr - r.base < r.size;
			});
		}

		// The range rounded out to whole pages
		std::pair<void*, usz> page_span(void* pointer, usz size)
		{
			const u64 begin = reinterpret_cast<u64>(pointer) & -ps5_page_size;
			const u64 end = utils::align<u64>(reinterpret_cast<u64>(pointer) + size, ps5_page_size);
			return {reinterpret_cast<void*>(begin), end - begin};
		}
	}

	long get_page_size()
	{
		return ps5_page_size;
	}

	void* memory_reserve(usz size, void* use_addr, [[maybe_unused]] bool is_memory_mapping, [[maybe_unused]] bool can_be_jit)
	{
		void* base = nullptr;

		if (use_addr)
		{
			// Exactly at the address, or refused: callers search upward themselves
			if (reinterpret_cast<uptr>(use_addr) % 0x10000 || libre_vrange_reserve_at(use_addr, size) != 0)
			{
				return nullptr;
			}
			base = use_addr;
		}
		else
		{
			// Anywhere means the first free place at 0x40_0000_0000 or above:
			// the kernel's own choice was 0x8_0000_0000 to 0xA_0000_0000, where
			// it then refused to commit memory (the JIT's code reservations,
			// memory_commit EINVAL, on my console), as it had refused the
			// guest's executable range there. The guest's ranges are at
			// 0x10_0000_0000 and the title heap at 0x20_0000_0000
			// (the libre layer's heap). Searched from the bottom each time, so a
			// released range (the JIT's, as each compiler is destroyed) is used
			// again: handed out in order, never reused, they ran out after some
			// 700 of the JIT's 768 MiB
			static std::mutex s_lock;
			std::lock_guard lock(s_lock);
			u64 at = 0x40'0000'0000;
			for (u32 tries = 0; tries < 4096 && !base; tries++)
			{
				if (libre_vrange_reserve_at(reinterpret_cast<void*>(at), size) == 0)
				{
					base = reinterpret_cast<void*>(at);
					break;
				}

				// Past what is taken here: the next range of our own, or the
				// next 256 MiB where something else is
				u64 next = at + 0x1000'0000;
				{
					std::lock_guard ranges_lock(g_ranges_lock);
					for (const reserved_range& r : g_ranges)
					{
						if (at >= r.base && at - r.base < r.size)
						{
							next = utils::align<u64>(r.base + r.size, 0x10000);
						}
					}
				}
				at = std::max(next, at + 0x10000);
			}

			if (!base)
			{
				return nullptr;
			}
		}

		std::lock_guard lock(g_ranges_lock);
		g_ranges.push_back({reinterpret_cast<u64>(base), size});
		return base;
	}

	void memory_commit(void* pointer, usz size, protection prot)
	{
		if (!size)
		{
			return;
		}

		// Execute asked for at map time is refused; the libre layer maps new
		// units read-write and gives them execute after, under its lock
		// (libre/vm.h). The whole protection
		// goes to it in one call: committing memory already backed only changes
		// its protection, and committing RPCS3's JIT code read-write first, then
		// executable, took execute away for a moment from code other threads
		// were running. The asmjit runtime commits its code 2 MiB at a time, and
		// two threads crossing into a new 2 MiB both commit it: GTA IV's SPU
		// threads faulted executing their dispatch code just past 24 MiB, three
		// at once, on my console
		const int result = libre_vrange_commit(pointer, size, ps5_protection(prot));
		if (result != 0)
		{
			fmt::throw_exception("memory_commit(%p, 0x%x, %d) failed: 0x%x", pointer, size, static_cast<int>(prot), static_cast<u32>(result));
		}
	}

	void memory_decommit(void* pointer, usz size, bool /*can_be_jit*/)
	{
		if (!size)
		{
			return;
		}

		const int result = libre_vrange_decommit(pointer, size);
		if (result != 0)
		{
			fmt::throw_exception("memory_decommit(%p, 0x%x) failed: 0x%x", pointer, size, static_cast<u32>(result));
		}
	}

	void memory_reset(void* pointer, usz size, protection prot, bool can_be_jit)
	{
		// Decommitted units come back zeroed at their next commit
		memory_decommit(pointer, size, can_be_jit);
		memory_commit(pointer, size, prot);
	}

	void memory_release(void* pointer, usz size)
	{
		if (!size)
		{
			return;
		}

		ensure(libre_vrange_release(pointer, size) == 0);

		std::lock_guard lock(g_ranges_lock);
		std::erase_if(g_ranges, [&](const reserved_range& r) { return r.base == reinterpret_cast<u64>(pointer); });
	}

	void memory_protect(void* pointer, usz size, protection prot)
	{
		if (!size)
		{
			return;
		}

		// Mapping by mapping: memory already mapped (shared-memory views, committed
		// units) changes protection in place with the kernel's own call (libc's
		// mprotect refused the guest's memory views: EINVAL); reserved space that
		// nothing backs yet is committed with that protection.
		//
		// Mapped memory is never committed over: the guest's memory is one
		// shared object seen at g_base_addr and at its mirror g_sudo_addr, and
		// fresh memory under one view parts it from the other (the PS3 home
		// menu then read code bytes where a function descriptor had been
		// written, on my console). A protection the kernel refuses there is
		// widened instead, no access to read-only, then left as it was, and
		// reported
		const auto [page, bytes] = page_span(pointer, size);
		const u64 end = reinterpret_cast<u64>(page) + bytes;

		for (u64 at = reinterpret_cast<u64>(page); at < end;)
		{
			void* start = nullptr;
			void* stop = nullptr;
			u32 current = 0;

			const s32 queried = sceKernelQueryMemoryProtection(reinterpret_cast<void*>(at), &start, &stop, &current);
			const bool mapped = queried == 0 && reinterpret_cast<u64>(start) <= at && reinterpret_cast<u64>(stop) > at;
			if (mapped || is_view(at))
			{
				// A view the query did not describe goes a page at a time
				const u64 next = mapped ? std::min<u64>(end, reinterpret_cast<u64>(stop)) : at + ps5_page_size;
				const s32 changed = sceKernelMprotect(reinterpret_cast<void*>(at), next - at, kernel_protection(prot));
				if (changed != 0)
				{
					s32 widened = changed;
					if (prot == protection::no)
					{
						widened = sceKernelMprotect(reinterpret_cast<void*>(at), next - at, kernel_protection(protection::ro));
					}

					static atomic_t<u32> s_reports = 0;
					if (s_reports++ < 20)
					{
						vm_log.error("memory_protect(%p, 0x%x, %d): the kernel refused 0x%x on mapped memory at 0x%x-0x%x (prot 0x%x): 0x%x%s",
							pointer, size, static_cast<int>(prot), kernel_protection(prot), at, next, current, static_cast<u32>(changed),
							prot == protection::no ? (widened == 0 ? "; made read-only instead" : "; read-only refused too, left as it was") : "; left as it was");
					}
				}
				at = next;
				continue;
			}

			if (!is_reserved(at))
			{
				fmt::throw_exception("memory_protect(%p, 0x%x, %d): 0x%x is neither mapped nor reserved (query 0x%x: %p-%p prot 0x%x)",
					pointer, size, static_cast<int>(prot), at, static_cast<u32>(queried), start, stop, current);
			}

			// Reserved and unbacked, a page at a time (the query says nothing of
			// where the next mapping starts)
			const u64 next = at + ps5_page_size;
			if (const int result = libre_vrange_commit(reinterpret_cast<void*>(at), next - at, ps5_protection(prot)); result != 0)
			{
				fmt::throw_exception("memory_protect(%p, 0x%x, %d) at 0x%x: query 0x%x, then commit 0x%x",
					pointer, size, static_cast<int>(prot), at, static_cast<u32>(queried), static_cast<u32>(result));
			}
			at = next;
		}
	}

	bool memory_lock(void* pointer, usz size)
	{
		return !size || !::mlock(pointer, size);
	}

	void* memory_map_fd(native_handle fd, usz size, protection prot)
	{
		const auto result = ::mmap(nullptr, size, posix_protection(prot), MAP_SHARED, fd, 0);
		return result == MAP_FAILED ? nullptr : result;
	}

	shm::shm(u64 size, u32 flags)
		: m_flags(flags)
		, m_size(utils::align(size, 0x10000))
	{
		libre_shm object{};
		const int result = libre_shm_create(m_size, &object);
		if (result != 0)
		{
			fmt::throw_exception("libre_shm_create(0x%x) failed: 0x%x", m_size, static_cast<u32>(result));
		}
		m_direct_start = object.direct_start;
		m_direct_bytes = object.size;
	}

	shm::shm(u64 size, const std::string& storage)
		: shm(size, 0)
	{
		// No sparse file on the console: the storage is direct memory as well
		m_storage = storage;
	}

	shm::~shm()
	{
		this->unmap_self();

		libre_shm object{m_direct_start, static_cast<size_t>(m_direct_bytes)};
		libre_shm_destroy(&object);
	}

	u8* shm::map(void* ptr, protection prot, bool cow) const
	{
		if (cow)
		{
			// One direct-memory object has no private copies of its pages
			vm_log.error("shm::map: copy-on-write mappings are not supported on the PS5");
			return nullptr;
		}

		const libre_shm object{m_direct_start, static_cast<size_t>(m_direct_bytes)};
		void* const target = reinterpret_cast<void*>(reinterpret_cast<u64>(ptr) & -0x10000);
		void* view = nullptr;

		// At an address: there, over whatever reservation is there; else anywhere
		const unsigned flags = target ? LIBRE_SHM_FIXED : 0;
		if (libre_shm_map(&object, 0, m_size, target, ps5_protection(prot), flags, &view) != 0)
		{
			return nullptr;
		}

		add_view(view, m_size);

		return static_cast<u8*>(view);
	}

	u8* shm::try_map(void* ptr, protection prot, bool cow) const
	{
		return this->map(ensure(ptr), prot, cow);
	}

	std::pair<u8*, std::string> shm::map_critical(void* ptr, protection prot, bool cow)
	{
		if (const auto mapped = this->map(ptr, prot, cow))
		{
			return {mapped, {}};
		}

		return {nullptr, "libre_shm_map failed"};
	}

	u8* shm::map_self(protection prot)
	{
		void* ptr = m_ptr;

		for (void* mapped = nullptr; !ptr;)
		{
			if (!mapped)
			{
				mapped = this->map(nullptr, prot);

				if (!mapped)
				{
					if ((ptr = m_ptr))
					{
						break;
					}

					return nullptr;
				}
			}

			// Install mapped memory
			if (m_ptr.compare_exchange(ptr, mapped))
			{
				ptr = mapped;
			}
			else if (ptr)
			{
				// Mapped already, nothing to do.
				ensure(ptr != mapped);
				this->unmap(mapped);
			}
		}

		return static_cast<u8*>(ptr);
	}

	void shm::unmap(void* ptr) const
	{
		remove_view(ptr);
		libre_shm_unmap(ptr, m_size, 0);
	}

	void shm::unmap_critical(void* ptr)
	{
		// The view goes; its place stays reserved, a hole in the guest's layout
		void* const target = reinterpret_cast<void*>(reinterpret_cast<u64>(ptr) & -0x10000);
		remove_view(target);
		libre_shm_unmap(target, m_size, LIBRE_SHM_KEEP_RESERVED);
	}

	void shm::unmap_self()
	{
		if (auto ptr = m_ptr.exchange(nullptr))
		{
			this->unmap(ptr);
		}
	}
}
