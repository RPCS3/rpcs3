#include "stdafx.h"
#include "vfsStream.h"

vfsStream::vfsStream()
{
}

vfsStream::~vfsStream()
{
	Close();
}

void vfsStream::Reset()
{
	m_pos = 0;
}

bool vfsStream::Close()
{
	Reset();

	return true;
}

u64 vfsStream::GetSize()
{
	u64 last_pos = Tell();
	Seek(0, vfsSeekEnd);
	u64 size = Tell();
	Seek(last_pos, vfsSeekSet);

	return size;
}

u32 vfsStream::Write(const void* src, u32 size)
{
	m_pos += size;

	return size;
}

u32 vfsStream::Read(void* dst, u32 size)
{
	m_pos += size;

	return size;
}

u64 vfsStream::Seek(s64 offset, vfsSeekMode mode)
{
	switch(mode)
	{
	case vfsSeekSet: m_pos = offset; break;
	case vfsSeekCur: m_pos += offset; break;
	case vfsSeekEnd: m_pos = GetSize() + offset; break;
	}

	return m_pos;
}

u64 vfsStream::Tell() const
{
	return m_pos;
}

bool vfsStream::Eof()
{
	return Tell() >= GetSize();
}

bool vfsStream::IsOpened() const
{
	return true;
}