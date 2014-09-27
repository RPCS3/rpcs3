#pragma once

namespace vm
{
	bool map(u32 addr, u32 size, u32 flags);
	bool unmap(u32 addr, u32 size = 0, u32 flags = 0);
	u32 alloc(u32 size);
	void unalloc(u32 addr);
	
	template<typename T>
	T* const get_ptr(u32 addr)
	{
		return (T*)((u8*)m_base_addr + addr);
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
		static u8 read8(u32 addr)
		{
			return *((u8*)m_base_addr + addr);
		}

		static u8 read8(u64 addr)
		{
			return read8((u32)addr);
		}

		static void write8(u32 addr, u8 value)
		{
			*((u8*)m_base_addr + addr) = value;
		}

		static void write8(u64 addr, u8 value)
		{
			write8((u32)addr, value);
		}

		static u16 read16(u32 addr)
		{
			return re16(*(u16*)((u8*)m_base_addr + addr));
		}

		static u16 read16(u64 addr)
		{
			return read16((u32)addr);
		}

		static void write16(u32 addr, u16 value)
		{
			*(u16*)((u8*)m_base_addr + addr) = re16(value);
		}

		static void write16(u64 addr, u16 value)
		{
			write16((u32)addr, value);
		}

		static u32 read32(u32 addr)
		{
			if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET)
			{
				return re32(*(u32*)((u8*)m_base_addr + addr));
			}
			else
			{
				return Memory.ReadMMIO32((u32)addr);
			}
		}

		static u32 read32(u64 addr)
		{
			return read32((u32)addr);
		}

		static void write32(u32 addr, u32 value)
		{
			if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET)
			{
				*(u32*)((u8*)m_base_addr + addr) = re32(value);
			}
			else
			{
				Memory.WriteMMIO32((u32)addr, value);
			}
		}

		static void write32(u64 addr, u32 value)
		{
			write32((u32)addr, value);
		}

		static u64 read64(u32 addr)
		{
			return re64(*(u64*)((u8*)m_base_addr + addr));
		}

		static u64 read64(u64 addr)
		{
			return read64((u32)addr);
		}

		static void write64(u32 addr, u64 value)
		{
			*(u64*)((u8*)m_base_addr + addr) = re64(value);
		}

		static void write64(u64 addr, u64 value)
		{
			write64((u32)addr, value);
		}

		static u128 read128(u32 addr)
		{
			return re128(*(u128*)((u8*)m_base_addr + addr));
		}

		static u128 read128(u64 addr)
		{
			return read128((u32)addr);
		}

		static void write128(u32 addr, u128 value)
		{
			*(u128*)((u8*)m_base_addr + addr) = re128(value);
		}

		static void write128(u64 addr, u128 value)
		{
			write128((u32)addr, value);
		}
	}
	
	namespace psv
	{
		static u8 read8(u32 addr)
		{
			return *((u8*)m_base_addr + addr);
		}

		static void write8(u32 addr, u8 value)
		{
			*((u8*)m_base_addr + addr) = value;
		}

		static u16 read16(u32 addr)
		{
			return *(u16*)((u8*)m_base_addr + addr);
		}

		static void write16(u32 addr, u16 value)
		{
			*(u16*)((u8*)m_base_addr + addr) = value;
		}

		static u32 read32(u32 addr)
		{
			return *(u32*)((u8*)m_base_addr + addr);
		}

		static void write32(u32 addr, u32 value)
		{
			*(u32*)((u8*)m_base_addr + addr) = value;
		}

		static u64 read64(u32 addr)
		{
			return *(u64*)((u8*)m_base_addr + addr);
		}

		static void write64(u32 addr, u64 value)
		{
			*(u64*)((u8*)m_base_addr + addr) = value;
		}

		static u128 read128(u32 addr)
		{
			return *(u128*)((u8*)m_base_addr + addr);
		}

		static void write128(u32 addr, u128 value)
		{
			*(u128*)((u8*)m_base_addr + addr) = value;
		}
	}
}

#include "vm_ref.h"
#include "vm_ptr.h"
#include "vm_var.h"
#include "atomic_type.h"
