#include "stdafx.h"
#include "Utilities/Log.h"
#include "Memory.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"

#include "Emu/SysCalls/lv2/sys_time.h"

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
			printf("shm_open('/rpcs3_vm') failed\n");
			return (void*)-1;
		}

		if (ftruncate(memory_handle, 0x100000000) == -1)
		{
			printf("ftruncate(memory_handle) failed\n");
			shm_unlink("/rpcs3_vm");
			return (void*)-1;
		}

		void* base_addr = mmap(nullptr, 0x100000000, PROT_NONE, MAP_SHARED, memory_handle, 0);
		g_priv_addr = mmap(nullptr, 0x100000000, PROT_NONE, MAP_SHARED, memory_handle, 0);

		shm_unlink("/rpcs3_vm");

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

	void* g_base_addr = (atexit(finalize), initialize());
	void* g_priv_addr;

	std::array<atomic<u8>, 0x100000000ull / 4096> g_page_info = {}; // information about every page

	class reservation_mutex_t
	{
		atomic<const thread_ctrl_t*> m_owner{};
		std::condition_variable m_cv;
		std::mutex m_cv_mutex;

	public:
		reservation_mutex_t()
		{
		}

		bool do_notify;

		never_inline void lock()
		{
			auto owner = get_current_thread_ctrl();

			while (auto old = m_owner.compare_and_swap(nullptr, owner))
			{
				std::unique_lock<std::mutex> cv_lock(m_cv_mutex);

				m_cv.wait_for(cv_lock, std::chrono::milliseconds(1));

				if (old == owner)
				{
					throw EXCEPTION("Deadlock");
				}

				old = nullptr;
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

	std::function<void()> g_reservation_cb = nullptr;
	const thread_ctrl_t* g_reservation_owner = nullptr;

	u32 g_reservation_addr = 0;
	u32 g_reservation_size = 0;

	reservation_mutex_t g_reservation_mutex;

	void _reservation_set(u32 addr, bool no_access = false)
	{
		//const auto stamp0 = get_time();

#ifdef _WIN32
		DWORD old;
		if (!VirtualProtect(vm::get_ptr(addr & ~0xfff), 4096, no_access ? PAGE_NOACCESS : PAGE_READONLY, &old))
#else
		if (mprotect(vm::get_ptr(addr & ~0xfff), 4096, no_access ? PROT_NONE : PROT_READ))
#endif
		{
			throw EXCEPTION("System failure (addr=0x%x)", addr);
		}

		//LOG_NOTICE(MEMORY, "VirtualProtect: %f us", (get_time() - stamp0) / 80.f);
	}

	bool _reservation_break(u32 addr)
	{
		if (g_reservation_addr >> 12 == addr >> 12)
		{
			//const auto stamp0 = get_time();

#ifdef _WIN32
			DWORD old;
			if (!VirtualProtect(vm::get_ptr(addr & ~0xfff), 4096, PAGE_READWRITE, &old))
#else
			if (mprotect(vm::get_ptr(addr & ~0xfff), 4096, PROT_READ | PROT_WRITE))
#endif
			{
				throw EXCEPTION("System failure (addr=0x%x)", addr);
			}

			//LOG_NOTICE(MEMORY, "VirtualAlloc: %f us", (get_time() - stamp0) / 80.f);

			if (g_reservation_cb)
			{
				g_reservation_cb();
				g_reservation_cb = nullptr;
			}

			g_reservation_owner = nullptr;
			g_reservation_addr = 0;
			g_reservation_size = 0;

			return true;
		}

		return false;
	}

	bool reservation_break(u32 addr)
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		return _reservation_break(addr);
	}

	bool reservation_acquire(void* data, u32 addr, u32 size, const std::function<void()>& callback)
	{
		//const auto stamp0 = get_time();

		bool broken = false;

		assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 128);
		assert((addr + size - 1 & ~0xfff) == (addr & ~0xfff));

		{
			std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

			u8 flags = g_page_info[addr >> 12].load();
			if (!(flags & page_writable) || !(flags & page_allocated) || (flags & page_no_reservations))
			{
				throw EXCEPTION("Invalid page flags (addr=0x%x, size=0x%x, flags=0x%x)", addr, size, flags);
			}

			// silent unlocking to prevent priority boost for threads going to break reservation
			//g_reservation_mutex.do_notify = false;

			// break previous reservation
			if (g_reservation_owner)
			{
				broken = _reservation_break(g_reservation_addr);
			}

			// change memory protection to read-only
			_reservation_set(addr);

			// may not be necessary
			_mm_mfence();

			// set additional information
			g_reservation_addr = addr;
			g_reservation_size = size;
			g_reservation_owner = get_current_thread_ctrl();
			g_reservation_cb = callback;

			// copy data
			memcpy(data, vm::get_ptr(addr), size);
		}

		return broken;
	}

	bool reservation_acquire_no_cb(void* data, u32 addr, u32 size)
	{
		return reservation_acquire(data, addr, size);
	}

	bool reservation_update(u32 addr, const void* data, u32 size)
	{
		assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 128);
		assert((addr + size - 1 & ~0xfff) == (addr & ~0xfff));

		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		if (g_reservation_owner != get_current_thread_ctrl() || g_reservation_addr != addr || g_reservation_size != size)
		{
			// atomic update failed
			return false;
		}

		// change memory protection to no access
		_reservation_set(addr, true);

		// update memory using privileged access
		memcpy(vm::priv_ptr(addr), data, size);

		// remove callback to not call it on successful update
		g_reservation_cb = nullptr;

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

	void reservation_free()
	{
		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		if (g_reservation_owner == get_current_thread_ctrl())
		{
			_reservation_break(g_reservation_addr);
		}
	}

	void reservation_op(u32 addr, u32 size, std::function<void()> proc)
	{
		assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 128);
		assert((addr + size - 1 & ~0xfff) == (addr & ~0xfff));

		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

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
		g_reservation_cb = nullptr;

		// may not be necessary
		_mm_mfence();

		// do the operation
		proc();

		// remove the reservation
		_reservation_break(addr);
	}

	void page_map(u32 addr, u32 size, u8 flags)
	{
		assert(size && (size | addr) % 4096 == 0 && flags < page_allocated);

		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (g_page_info[i].load())
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
			if (g_page_info[i].exchange(flags | page_allocated))
			{
				throw EXCEPTION("Concurrent access (addr=0x%x, size=0x%x, flags=0x%x, current_addr=0x%x)", addr, size, flags, i * 4096);
			}
		}

		memset(priv_addr, 0, size); // ???
	}

	bool page_protect(u32 addr, u32 size, u8 flags_test, u8 flags_set, u8 flags_clear)
	{
		u8 flags_inv = flags_set & flags_clear;

		assert(size && (size | addr) % 4096 == 0);

		flags_test |= page_allocated;

		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if ((g_page_info[i].load() & flags_test) != (flags_test | page_allocated))
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

			const u8 f1 = g_page_info[i]._or(flags_set & ~flags_inv) & (page_writable | page_readable);
			g_page_info[i]._and_not(flags_clear & ~flags_inv);
			const u8 f2 = (g_page_info[i] ^= flags_inv) & (page_writable | page_readable);

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

	void page_unmap(u32 addr, u32 size)
	{
		assert(size && (size | addr) % 4096 == 0);

		std::lock_guard<reservation_mutex_t> lock(g_reservation_mutex);

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			if (!(g_page_info[i].load() & page_allocated))
			{
				throw EXCEPTION("Memory not mapped (addr=0x%x, size=0x%x, current_addr=0x%x)", addr, size, i * 4096);
			}
		}

		for (u32 i = addr / 4096; i < addr / 4096 + size / 4096; i++)
		{
			_reservation_break(i * 4096);

			if (!(g_page_info[i].exchange(0) & page_allocated))
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

	// Not checked if address is writable/readable. Checking address before using it is unsafe.
	// The only safe way to check it is to protect both actions (checking and using) with mutex that is used for mapping/allocation.
	bool check_addr(u32 addr, u32 size)
	{
		assert(size);

		if (addr + (size - 1) < addr)
		{
			return false;
		}

		for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
		{
			if ((g_page_info[i].load() & page_allocated) != page_allocated)
			{
				return false;
			}
		}
		
		return true;
	}

	//TODO
	bool map(u32 addr, u32 size, u32 flags)
	{
		return Memory.Map(addr, size);
	}

	bool unmap(u32 addr, u32 size, u32 flags)
	{
		return Memory.Unmap(addr);
	}

	u32 alloc(u32 addr, u32 size, memory_location location)
	{
		return g_locations[location].fixed_allocator(addr, size);
	}

	u32 alloc(u32 size, memory_location location)
	{
		return g_locations[location].allocator(size);
	}

	void dealloc(u32 addr, memory_location location)
	{
		return g_locations[location].deallocator(addr);
	}

	namespace ps3
	{
		u32 main_alloc(u32 size)
		{
			return Memory.MainMem.AllocAlign(size, 1);
		}
		u32 main_fixed_alloc(u32 addr, u32 size)
		{
			return Memory.MainMem.AllocFixed(addr, size) ? addr : 0;
		}
		void main_dealloc(u32 addr)
		{
			Memory.MainMem.Free(addr);
		}

		u32 user_space_alloc(u32 size)
		{
			return Memory.Userspace.AllocAlign(size, 1);
		}
		u32 user_space_fixed_alloc(u32 addr, u32 size)
		{
			return Memory.Userspace.AllocFixed(addr, size) ? addr : 0;
		}
		void user_space_dealloc(u32 addr)
		{
			Memory.Userspace.Free(addr);
		}

		u32 g_stack_offset = 0;

		u32 stack_alloc(u32 size)
		{
			return Memory.StackMem.AllocAlign(size, 0x10);
		}
		u32 stack_fixed_alloc(u32 addr, u32 size)
		{
			return Memory.StackMem.AllocFixed(addr, size) ? addr : 0;
		}
		void stack_dealloc(u32 addr)
		{
			Memory.StackMem.Free(addr);
		}

		void init()
		{
			Memory.Init(Memory_PS3);
		}
	}

	namespace psv
	{
		void init()
		{
			Memory.Init(Memory_PSV);
		}
	}

	namespace psp
	{
		void init()
		{
			Memory.Init(Memory_PSP);
		}
	}

	location_info g_locations[memory_location_count] =
	{
		{ 0x00010000, 0x1FFF0000, ps3::main_alloc, ps3::main_fixed_alloc, ps3::main_dealloc },
		{ 0x20000000, 0x10000000, ps3::user_space_alloc, ps3::user_space_fixed_alloc, ps3::user_space_dealloc },
		{ 0xD0000000, 0x10000000, ps3::stack_alloc, ps3::stack_fixed_alloc, ps3::stack_dealloc },
	};

	void close()
	{
		Memory.Close();
	}

	u32 stack_push(CPUThread& CPU, u32 size, u32 align_v, u32& old_pos)
	{
		switch (CPU.GetType())
		{
		case CPU_THREAD_PPU:
		{
			PPUThread& context = static_cast<PPUThread&>(CPU);

			old_pos = vm::cast(context.GPR[1], "SP");
			context.GPR[1] -= align(size, 8); // room minimal possible size
			context.GPR[1] &= ~(align_v - 1); // fix stack alignment

			if (context.GPR[1] < context.stack_addr)
			{
				LOG_ERROR(PPU, "vm::stack_push(0x%x,%d): stack overflow (SP=0x%llx, stack=*0x%x)", size, align_v, context.GPR[1], context.stack_addr);
				context.GPR[1] = old_pos;
				return 0;
			}
			else
			{
				return static_cast<u32>(context.GPR[1]);
			}
		}

		case CPU_THREAD_SPU:
		case CPU_THREAD_RAW_SPU:
		{
			assert(!"stack_push(): SPU not supported");
			return 0;
		}

		case CPU_THREAD_ARMv7:
		{
			ARMv7Context& context = static_cast<ARMv7Thread&>(CPU);

			old_pos = context.SP;
			context.SP -= align(size, 4); // room minimal possible size
			context.SP &= ~(align_v - 1); // fix stack alignment

			if (context.SP < context.stack_addr)
			{
				LOG_ERROR(ARMv7, "vm::stack_push(0x%x,%d): stack overflow (SP=0x%x, stack=*0x%x)", size, align_v, context.SP, context.stack_addr);
				context.SP = old_pos;
				return 0;
			}
			else
			{
				return context.SP;
			}
		}

		default:
		{
			assert(!"stack_push(): invalid thread type");
			return 0;
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

			if (context.GPR[1] != addr && !Emu.IsStopped())
			{
				LOG_ERROR(PPU, "vm::stack_pop(*0x%x,*0x%x): stack inconsistency (SP=0x%llx)", addr, old_pos, context.GPR[1]);
			}

			context.GPR[1] = old_pos;
			return;
		}

		case CPU_THREAD_SPU:
		case CPU_THREAD_RAW_SPU:
		{
			assert(!"stack_pop(): SPU not supported");
			return;
		}

		case CPU_THREAD_ARMv7:
		{
			ARMv7Context& context = static_cast<ARMv7Thread&>(CPU);

			if (context.SP != addr && !Emu.IsStopped())
			{
				LOG_ERROR(ARMv7, "vm::stack_pop(*0x%x,*0x%x): stack inconsistency (SP=0x%x)", addr, old_pos, context.SP);
			}

			context.SP = old_pos;
			return;
		}

		default:
		{
			assert(!"stack_pop(): invalid thread type");
			return;
		}
		}
	}
}
