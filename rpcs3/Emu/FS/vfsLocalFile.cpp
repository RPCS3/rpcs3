#include "stdafx.h"
#include "Utilities/Log.h"
#include "vfsLocalFile.h"

static const rFile::OpenMode vfs2wx_mode(vfsOpenMode mode)
{
	switch(mode)
	{
	case vfsRead:        return rFile::read;
	case vfsWrite:       return rFile::write;
	case vfsReadWrite:   return rFile::read_write;
	case vfsWriteExcl:   return rFile::write_excl;
	case vfsWriteAppend: return rFile::write_append;
	}

	return rFile::read;
}

static const rSeekMode vfs2wx_seek(vfsSeekMode mode)
{
	switch(mode)
	{
	case vfsSeekSet: return rFromStart;
	case vfsSeekCur: return rFromCurrent;
	case vfsSeekEnd: return rFromEnd;
	}

	return rFromStart;
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
		if(!m_file.Access(path, vfs2wx_mode(mode))) return false;

		return m_file.Open(path, vfs2wx_mode(mode)) && vfsFileBase::Open(path, mode);
	// }
}

bool vfsLocalFile::Create(const std::string& path)
{
	LOG_WARNING(HLE, "vfsLocalFile::Create('%s')", path.c_str());
	for(uint p=1; p < path.length() && path[p] != '\0' ; p++)
	{
		for(; p < path.length() && path[p] != '\0'; p++)
			if(path[p] == '/' || path[p] == '\\') break; // ???

		if(p == path.length() || path[p] == '\0')
			break;

		const std::string& dir = path.substr(0, p);
		if(!rExists(dir))
		{
			LOG_NOTICE(HLE, "create dir: %s", dir.c_str());
			rMkdir(dir);
		}
	}

	//create file
	const char m = path[path.length() - 1];
	if(m != '/' && m != '\\' && !rExists(path)) // ???
	{
		rFile f;
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

bool vfsLocalFile::Exists(const std::string& path)
{
	return rExists(path);
}