#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "vfsStreamMemory.h"

u64 vfsStreamMemory::Write(const void* src, u64 count)
{
	assert(m_pos < m_size);
	if (m_pos + count > m_size)
	{
		count = m_size - m_pos;
	}

	memcpy(vm::get_ptr<void>(vm::cast(m_addr + m_pos)), src, count);
	m_pos += count;
	return count;
}

u64 vfsStreamMemory::Read(void* dst, u64 count)
{
	assert(m_pos < m_size);
	if (m_pos + count > m_size)
	{
		count = m_size - m_pos;
	}

	memcpy(dst, vm::get_ptr<void>(vm::cast(m_addr + m_pos)), count);
	m_pos += count;
	return count;
}
