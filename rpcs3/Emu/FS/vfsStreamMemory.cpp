#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "vfsStreamMemory.h"

vfsStreamMemory::vfsStreamMemory() : vfsStream()
{
}

vfsStreamMemory::vfsStreamMemory(u32 addr, u32 size) : vfsStream()
{
	Open(addr, size);
}

void vfsStreamMemory::Open(u32 addr, u32 size)
{
	m_addr = addr;
	m_size = size ? size : 0x100000000ull - addr; // determine max possible size

	vfsStream::Reset();
}

u64 vfsStreamMemory::GetSize()
{
	return m_size;
}

u64 vfsStreamMemory::Write(const void* src, u64 size)
{
	assert(Tell() < m_size);
	if (Tell() + size > m_size)
	{
		size = m_size - Tell();
	}

	memcpy(vm::get_ptr<void>(vm::cast(m_addr + Tell())), src, size);
	return vfsStream::Write(src, size);
}

u64 vfsStreamMemory::Read(void* dst, u64 size)
{
	assert(Tell() < GetSize());
	if (Tell() + size > GetSize())
	{
		size = GetSize() - Tell();
	}

	memcpy(dst, vm::get_ptr<void>(vm::cast(m_addr + Tell())), size);
	return vfsStream::Read(dst, size);
}
