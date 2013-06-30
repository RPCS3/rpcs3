#include "stdafx.h"
#include "vfsLocalFile.h"

static const wxFile::OpenMode vfs2wx_mode(vfsOpenMode mode)
{
	switch(mode)
	{
	case vfsRead:			return wxFile::read;
	case vfsWrite:			return wxFile::write;
	case vfsReadWrite:		return wxFile::read_write;
	case vfsWriteExcl:		return wxFile::write_excl;
	case vfsWriteAppend:	return wxFile::write_append;
	}

	return wxFile::read;
}

static const wxSeekMode vfs2wx_seek(vfsSeekMode mode)
{
	switch(mode)
	{
	case vfsSeekSet: return wxFromStart;
	case vfsSeekCur: return wxFromCurrent;
	case vfsSeekEnd: return wxFromEnd;
	}

	return wxFromStart;
}

vfsLocalFile::vfsLocalFile() : vfsFileBase()
{
}

vfsLocalFile::vfsLocalFile(const wxString path, vfsOpenMode mode) : vfsFileBase()
{
	Open(path, mode);
}

vfsDevice* vfsLocalFile::GetNew()
{
	return new vfsLocalFile();
}

bool vfsLocalFile::Open(const wxString& path, vfsOpenMode mode)
{
	Close();

	if(mode == vfsRead && !m_file.Access(vfsDevice::GetWinPath(GetLocalPath(), path), vfs2wx_mode(mode))) return false;

	return m_file.Open(vfsDevice::GetWinPath(GetLocalPath(), path), vfs2wx_mode(mode)) &&
		vfsFileBase::Open(vfsDevice::GetPs3Path(GetPs3Path(), path), mode);
}

bool vfsLocalFile::Create(const wxString& path)
{
	if(wxFileExists(path)) return false;

	wxFile f;
	return f.Create(path);
}

bool vfsLocalFile::Close()
{
	return m_file.Close() && vfsFileBase::Close();
}

u64 vfsLocalFile::GetSize()
{
	return m_file.Length();
}

u32 vfsLocalFile::Write(const void* src, u32 size)
{
	return m_file.Write(src, size);
}

u32 vfsLocalFile::Read(void* dst, u32 size)
{
	return m_file.Read(dst, size);
}

u64 vfsLocalFile::Seek(s64 offset, vfsSeekMode mode)
{
	return m_file.Seek(offset, vfs2wx_seek(mode));
}

u64 vfsLocalFile::Tell() const
{
	return m_file.Tell();
}

bool vfsLocalFile::IsOpened() const
{
	return m_file.IsOpened() && vfsFileBase::IsOpened();
}