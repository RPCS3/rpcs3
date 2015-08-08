#include "stdafx.h"
#include "Emu/System.h"

#include "VFS.h"
#include "vfsFile.h"

vfsFile::vfsFile()
	: vfsFileBase(nullptr)
	, m_stream(nullptr)
{
}

vfsFile::vfsFile(const std::string& path, u32 mode)
	: vfsFileBase(nullptr)
	, m_stream(nullptr)
{
	Open(path, mode);
}

bool vfsFile::Open(const std::string& path, u32 mode)
{
	Close();

	m_stream.reset(Emu.GetVFS().OpenFile(path, mode));

	return m_stream && m_stream->IsOpened();
}

bool vfsFile::Close()
{
	m_stream.reset();

	return true;
}

u64 vfsFile::GetSize() const
{
	return m_stream->GetSize();
}

u64 vfsFile::Write(const void* src, u64 size)
{
	return m_stream->Write(src, size);
}

u64 vfsFile::Read(void* dst, u64 size)
{
	return m_stream->Read(dst, size);
}

u64 vfsFile::Seek(s64 offset, u32 mode)
{
	return m_stream->Seek(offset, mode);
}

u64 vfsFile::Tell() const
{
	return m_stream->Tell();
}

bool vfsFile::IsOpened() const
{
	return m_stream && m_stream->IsOpened();
}
