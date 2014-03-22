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

vfsLocalFile::vfsLocalFile(vfsDevice* device) : vfsFileBase(device)
{
}

bool vfsLocalFile::Open(const wxString& path, vfsOpenMode mode)
{
	Close();

	if(m_device)
	{
		if(!m_file.Access(vfsDevice::GetWinPath(m_device->GetLocalPath(), path), vfs2wx_mode(mode))) return false;

		return m_file.Open(vfsDevice::GetWinPath(m_device->GetLocalPath(), path), vfs2wx_mode(mode)) &&
			vfsFileBase::Open(vfsDevice::GetPs3Path(m_device->GetPs3Path(), path), mode);
	}
	else
	{
		if(!m_file.Access(path, vfs2wx_mode(mode))) return false;

		return m_file.Open(path, vfs2wx_mode(mode)) && vfsFileBase::Open(path, mode);
	}
}

bool vfsLocalFile::Create(const wxString& path)
{
	ConLog.Warning("vfsLocalFile::Create('%s')", path.wx_str());
	for(uint p=1; p < path.Len() && path[p] != '\0' ; p++)
	{
		for(; p < path.Len() && path[p] != '\0'; p++)
			if(path[p] == '/' || path[p] == '\\') break; // ???

		if(p == path.Len() || path[p] == '\0')
			break;

		const wxString& dir = path(0, p);
		if(!wxDirExists(dir))
		{
			ConLog.Write("create dir: %s", dir.wx_str());
			wxMkdir(dir);
		}
	}

	//create file
	wxString m = path(path.Len() - 1, 1);
	if(m != '/' && m != '\\' && !wxFileExists(path)) // ???
	{
		wxFile f;
		return f.Create(path);
	}

	return true;
}

bool vfsLocalFile::Close()
{
	return m_file.Close() && vfsFileBase::Close();
}

u64 vfsLocalFile::GetSize()
{
	return m_file.Length();
}

u64 vfsLocalFile::Write(const void* src, u64 size)
{
	return m_file.Write(src, size);
}

u64 vfsLocalFile::Read(void* dst, u64 size)
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
