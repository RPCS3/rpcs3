#include "stdafx.h"
#include "vfsLocalFile.h"

static const wxFile::OpenMode vfs2wx_mode(vfsOpenMode mode)
{
	switch(mode)
	{
	case vfsRead:        return wxFile::read;
	case vfsWrite:       return wxFile::write;
	case vfsReadWrite:   return wxFile::read_write;
	case vfsWriteExcl:   return wxFile::write_excl;
	case vfsWriteAppend: return wxFile::write_append;
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

bool vfsLocalFile::Open(const std::string& path, vfsOpenMode mode)
{
	Close();

	// if(m_device)
	// {
	// 	if(!m_file.Access(fmt::FromUTF8(vfsDevice::GetWinPath(m_device->GetLocalPath(), path)), vfs2wx_mode(mode))) return false;

	// 	return m_file.Open(fmt::FromUTF8(vfsDevice::GetWinPath(m_device->GetLocalPath(), path)), vfs2wx_mode(mode)) &&
	// 		vfsFileBase::Open(fmt::FromUTF8(vfsDevice::GetPs3Path(m_device->GetPs3Path(), path)), mode);
	// }
	// else
	// {
		if(!m_file.Access(fmt::FromUTF8(path), vfs2wx_mode(mode))) return false;

		return m_file.Open(fmt::FromUTF8(path), vfs2wx_mode(mode)) && vfsFileBase::Open(path, mode);
	// }
}

bool vfsLocalFile::Create(const std::string& path)
{
	ConLog.Warning("vfsLocalFile::Create('%s')", path.c_str());
	for(uint p=1; p < path.length() && path[p] != '\0' ; p++)
	{
		for(; p < path.length() && path[p] != '\0'; p++)
			if(path[p] == '/' || path[p] == '\\') break; // ???

		if(p == path.length() || path[p] == '\0')
			break;

		const std::string& dir = path.substr(0, p);
		if(!wxDirExists(fmt::FromUTF8(dir)))
		{
			ConLog.Write("create dir: %s", dir.c_str());
			wxMkdir(fmt::FromUTF8(dir));
		}
	}

	//create file
	const char m = path[path.length() - 1];
	if(m != '/' && m != '\\' && !wxFileExists(fmt::FromUTF8(path))) // ???
	{
		wxFile f;
		return f.Create(fmt::FromUTF8(path));
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
