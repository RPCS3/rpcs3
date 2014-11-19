#pragma once
#include "Memory.h"

namespace vm
{
	enum memory_location
	{
		main,
		stack,
		sprx,
		user_space,

		memory_location_count
	};

	struct location_info
	{
		u32 addr_offset;
		u32 size;

		u32(*allocator)(u32 size);
		u32(*fixed_allocator)(u32 addr, u32 size);
		void(*deallocator)(u32 addr);

		u32 alloc_offset;
	};

	extern location_info g_locations[memory_location_count];

	static void set_stack_size(u32 size) {}
	static void initialize_stack() {}

	extern void* const g_base_addr;
	bool map(u32 addr, u32 size, u32 flags);
	bool unmap(u32 addr, u32 size = 0, u32 flags = 0);
	u32 alloc(u32 size, memory_location location = user_space);
	u32 alloc(u32 addr, u32 size, memory_location location = user_space);
	void dealloc(u32 addr, memory_location location = user_space);
	
	template<typename T = void>
	T* const get_ptr(u32 addr)
	{
		return (T*)((u8*)g_base_addr + addr);
	}

	template<typename T>
	T* const get_ptr(u64 addr)
	{
		return get_ptr<T>((u32)addr);
	}
	
	template<typename T>
	T& get_ref(u32 addr)
	{
		return *get_ptr<T>(addr);
	}

	template<typename T>
	T& get_ref(u64 addr)
	{
		return get_ref<T>((u32)addr);
	}

	namespace ps3
	{
		void init();

		static u8 read8(u32 addr)
		{
			return *((u8*)g_base_addr + addr);
		}

		static void write8(u32 addr, u8 value)
		{
			*((u8*)g_base_addr + addr) = value;
		}

		static u16 read16(u32 addr)
		{
			return re16(*(u16*)((u8*)g_base_addr + addr));
		}

		static void write16(u32 addr, be_t<u16> value)
		{
			*(be_t<u16>*)((u8*)g_base_addr + addr) = value;
		}

		static u32 read32(u32 addr)
		{
			if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET)
			{
				return re32(*(u32*)((u8*)g_base_addr + addr));
			}
			else
			{
				return Memory.ReadMMIO32((u32)addr);
			}
		}

		static void write32(u32 addr, be_t<u32> value)
		{
			if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET)
			{
				*(be_t<u32>*)((u8*)g_base_addr + addr) = value;
			}
			else
			{
				Memory.WriteMMIO32((u32)addr, value);
			}
		}

		static u64 read64(u32 addr)
		{
			return re64(*(u64*)((u8*)g_base_addr + addr));
		}

		static void write64(u32 addr, be_t<u64> value)
		{
			*(be_t<u64>*)((u8*)g_base_addr + addr) = value;
		}

		static void write16(u32 addr, u16 value)
		{
			write16(addr, be_t<u16>::make(value));
		}

		static void write32(u32 addr, u32 value)
		{
			write32(addr, be_t<u32>::make(value));
		}

		static void write64(u32 addr, u64 value)
		{
			write64(addr, be_t<u64>::make(value));
		}

		static u128 read128(u32 addr)
		{
			return re128(*(u128*)((u8*)g_base_addr + addr));
		}

		static void write128(u32 addr, u128 value)
		{
			*(u128*)((u8*)g_base_addr + addr) = re128(value);
		}
	}
	
	namespace psv
	{
		void init();

		static u8 read8(u32 addr)
		{
			return *((u8*)g_base_addr + addr);
		}

		static void write8(u32 addr, u8 value)
		{
			*((u8*)g_base_addr + addr) = value;
		}

		static u16 read16(u32 addr)
		{
			return *(u16*)((u8*)g_base_addr + addr);
		}

		static void write16(u32 addr, u16 value)
		{
			*(u16*)((u8*)g_base_addr + addr) = value;
		}

		static u32 read32(u32 addr)
		{
			return *(u32*)((u8*)g_base_addr + addr);
		}

		static void write32(u32 addr, u32 value)
		{
			*(u32*)((u8*)g_base_addr + addr) = value;
		}

		static u64 read64(u32 addr)
		{
			return *(u64*)((u8*)g_base_addr + addr);
		}

		static void write64(u32 addr, u64 value)
		{
			*(u64*)((u8*)g_base_addr + addr) = value;
		}

		static u128 read128(u32 addr)
		{
			return *(u128*)((u8*)g_base_addr + addr);
		}

		static void write128(u32 addr, u128 value)
		{
			*(u128*)((u8*)g_base_addr + addr) = value;
		}
	}

	namespace psp
	{
		using namespace psv;

		void init();
	}

	void close();
}

#include "vm_ref.h"
#include "vm_ptr.h"
#include "vm_var.h"