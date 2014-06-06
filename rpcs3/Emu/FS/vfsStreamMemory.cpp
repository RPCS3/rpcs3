#include "stdafx.h"
#include "Emu/ConLog.h"
#include "Emu/Memory/Memory.h"
#include "vfsStreamMemory.h"

vfsStreamMemory::vfsStreamMemory() : vfsStream()
{
}

vfsStreamMemory::vfsStreamMemory(u64 addr, u64 size) : vfsStream()
{
	Open(addr, size);
}

void vfsStreamMemory::Open(u64 addr, u64 size)
{
	m_addr = addr;
	m_size = size ? size : ~0ULL;

	vfsStream::Reset();
}

u64 vfsStreamMemory::GetSize()
{
	return m_size;
}

u64 vfsStreamMemory::Write(const void* src, u64 size)
{
	if(Tell() + size > GetSize())
	{
		size = GetSize() - Tell();
	}

	if(!size || !Memory.IsGoodAddr(m_addr + Tell(), size)) return 0;

	Memory.CopyFromReal(m_addr + Tell(), (void*)src, size);

	return vfsStream::Write(src, size);
}

u64 vfsStreamMemory::Read(void* dst, u64 size)
{
	if(Tell() + size > GetSize())
	{
		size = GetSize() - Tell();
	}

	if(!size || !Memory.IsGoodAddr(m_addr + Tell(), size)) return 0;

	Memory.CopyToReal(dst, m_addr + Tell(), size);

	return vfsStream::Read(dst, size);
}
