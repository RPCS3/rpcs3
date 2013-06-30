#include "stdafx.h"
#include "vfsStreamMemory.h"

vfsStreamMemory::vfsStreamMemory() : vfsStream()
{
}

vfsStreamMemory::vfsStreamMemory(u64 addr) : vfsStream()
{
	Open(addr);
}

void vfsStreamMemory::Open(u64 addr)
{
	m_addr = addr;

	vfsStream::Reset();
}

u32 vfsStreamMemory::Write(const void* src, u32 size)
{
	if(!Memory.IsGoodAddr(m_addr + Tell(), size)) return 0;

	memcpy(&Memory[m_addr + Tell()], src, size);

	return vfsStream::Write(src, size);
}

u32 vfsStreamMemory::Read(void* dst, u32 size)
{
	if(!Memory.IsGoodAddr(m_addr + Tell(), size)) return 0;

	memcpy(dst, &Memory[m_addr + Tell()], size);

	return vfsStream::Read(dst, size);
}
