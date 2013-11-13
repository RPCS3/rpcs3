#include "stdafx.h"
#include "vfsFile.h"

vfsFile::vfsFile()
	: vfsFileBase()
	, m_stream(nullptr)
{
}

vfsFile::vfsFile(const wxString path, vfsOpenMode mode)
	: vfsFileBase()
	, m_stream(nullptr)
{
	Open(path, mode);
}

vfsFile::~vfsFile()
{
	Close();
}

vfsDevice* vfsFile::GetNew()
{
	return new vfsFile();
}

bool vfsFile::Open(const wxString& path, vfsOpenMode mode)
{
	Close();

	m_stream = Emu.GetVFS().Open(path, mode);

	return m_stream && m_stream->IsOpened();
}

bool vfsFile::Create(const wxString& path)
{
	if(wxFileExists(path)) return false;

	wxFile f;
	return f.Create(path);
}

bool vfsFile::Close()
{
	if(m_stream)
	{
		delete m_stream;
		m_stream = nullptr;
		return vfsFileBase::Close();
	}

	return false;
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
