#include "stdafx.h"
#include "vfsFile.h"

vfsFile::vfsFile()
	: vfsFileBase(nullptr)
	, m_stream(nullptr)
{
}

vfsFile::vfsFile(const std::string& path, vfsOpenMode mode)
	: vfsFileBase(nullptr)
	, m_stream(nullptr)
{
	Open(path, mode);
}

bool vfsFile::Open(const std::string& path, vfsOpenMode mode)
{
	Close();

	m_stream.reset(Emu.GetVFS().OpenFile(path, mode));

	return m_stream && m_stream->IsOpened();
}

bool vfsFile::Create(const std::string& path)
{
	return m_stream->Create(path);
}

bool vfsFile::Exists(const std::string& path)
{
	return m_stream->Exists(path);
}

bool vfsFile::Rename(const std::string& from, const std::string& to)
{
	return m_stream->Rename(from, to);
}

bool vfsFile::Remove(const std::string& path)
{
	return m_stream->Remove(path);
}

bool vfsFile::Close()
{
	m_stream.reset();
	return vfsFileBase::Close();
}

u64 vfsFile::GetSize()
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

u64 vfsFile::Seek(s64 offset, vfsSeekMode mode)
{
	return m_stream->Seek(offset, mode);
}

u64 vfsFile::Tell() const
{
	return m_stream->Tell();
}

bool vfsFile::IsOpened() const
{
	return m_stream && m_stream->IsOpened() && vfsFileBase::IsOpened();
}
