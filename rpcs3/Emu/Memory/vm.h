#pragma once

namespace vm
{
	bool check_addr(u32 addr);
	bool map(u32 addr, u32 size, u32 flags);
	bool unmap(u32 addr, u32 size = 0, u32 flags = 0);
	u32 alloc(u32 size);
	void unalloc(u32 addr);
	
	template<typename T>
	T* get_ptr(u32 addr)
	{
		return (T*)&Memory[addr];
	}
	
	template<typename T>
	T& get_ref(u32 addr)
	{
		return (T&)Memory[addr];
	}

	//TODO
	u32 read32(u32 addr);
	bool read32(u32 addr, u32& value);
	bool write32(u32 addr, u32 value);
}

#include "vm_ptr.h"
#include "vm_ref.h"
#include "vm_var.h"