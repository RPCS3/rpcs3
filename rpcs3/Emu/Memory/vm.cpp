#include "stdafx.h"
#include "Memory.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/PPUThread.h"

namespace vm
{
	#ifdef _WIN32
	#include <Windows.h>
		void* const g_base_addr = VirtualAlloc(nullptr, 0x100000000, MEM_RESERVE, PAGE_NOACCESS);
	#else
	#include <sys/mman.h>

	/* OS X uses MAP_ANON instead of MAP_ANONYMOUS */
	#ifndef MAP_ANONYMOUS
	#define MAP_ANONYMOUS MAP_ANON
	#endif

	void* const g_base_addr = mmap(nullptr, 0x100000000, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	#endif

	bool check_addr(u32 addr)
	{
		// Checking address before using it is unsafe.
		// The only safe way to check it is to protect both actions (checking and using) with mutex that is used for mapping/allocation.
		return false;
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

		u32 sprx_alloc(u32 size)
		{
			return Memory.SPRXMem.AllocAlign(size, 1);
		}
		u32 sprx_fixed_alloc(u32 addr, u32 size)
		{
			return Memory.SPRXMem.AllocFixed(Memory.SPRXMem.GetStartAddr() + addr, size) ? Memory.SPRXMem.GetStartAddr() + addr : 0;
		}
		void sprx_dealloc(u32 addr)
		{
			Memory.SPRXMem.Free(addr);
		}

		u32 user_space_alloc(u32 size)
		{
			return Memory.PRXMem.AllocAlign(size, 1);
		}
		u32 user_space_fixed_alloc(u32 addr, u32 size)
		{
			return Memory.PRXMem.AllocFixed(addr, size) ? addr : 0;
		}
		void user_space_dealloc(u32 addr)
		{
			Memory.PRXMem.Free(addr);
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
		{ 0x00010000, 0x2FFF0000, ps3::main_alloc, ps3::main_fixed_alloc, ps3::main_dealloc },
		{ 0xD0000000, 0x10000000, ps3::stack_alloc, ps3::stack_fixed_alloc, ps3::stack_dealloc },

		//remove me
		{ 0x00010000, 0x2FFF0000, ps3::sprx_alloc, ps3::sprx_fixed_alloc, ps3::sprx_dealloc },

		{ 0x30000000, 0x10000000, ps3::user_space_alloc, ps3::user_space_fixed_alloc, ps3::user_space_dealloc },
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
			PPUThread& PPU = static_cast<PPUThread&>(CPU);

			assert(align_v);
			old_pos = (u32)PPU.GPR[1];
			PPU.GPR[1] -= align(size, 8); // room minimal possible size
			PPU.GPR[1] &= ~(align_v - 1); // fix stack alignment

			if (PPU.GPR[1] < CPU.GetStackAddr())
			{
				// stack overflow
				PPU.GPR[1] = old_pos;
				return 0;
			}
			else
			{
				return (u32)PPU.GPR[1];
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
			assert(!"stack_push(): ARMv7 not supported");
			return 0;
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
			PPUThread& PPU = static_cast<PPUThread&>(CPU);

			assert(PPU.GPR[1] == addr);
			PPU.GPR[1] = old_pos;
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
			assert(!"stack_pop(): ARMv7 not supported");
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