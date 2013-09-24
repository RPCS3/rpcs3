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

u64 vfsStreamMemory::Write(const void* src, u64 size)
{
	if(!Memory.IsGoodAddr(m_addr + Tell(), size)) return 0;

	memcpy(&Memory[m_addr + Tell()], src, size);

	return vfsStream::Write(src, size);
}

u64 vfsStreamMemory::Read(void* dst, u64 size)
{
	if(!Memory.IsGoodAddr(m_addr + Tell(), size)) return 0;

	memcpy(dst, &Memory[m_addr + Tell()], size);

	return vfsStream::Read(dst, size);
}
