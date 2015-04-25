#include "stdafx.h"
#include "vfsDevice.h"
#include "vfsLocalDir.h"

vfsLocalDir::vfsLocalDir(vfsDevice* device) : vfsDirBase(device)
{
}

vfsLocalDir::~vfsLocalDir()
{
}

bool vfsLocalDir::Open(const std::string& path)
{
	if (!vfsDirBase::Open(path) || !m_dir.open(path))
	{
		return false;
	}

	std::string name;
	fs::stat_t file_info;

	for (bool is_ok = m_dir.get_first(name, file_info); is_ok; is_ok = m_dir.get_next(name, file_info))
	{
		m_entries.emplace_back();

		DirEntryInfo& info = m_entries.back();

		info.name = name;
		info.flags |= file_info.is_directory ? DirEntry_TypeDir | DirEntry_PermExecutable : DirEntry_TypeFile;
		info.flags |= file_info.is_writable ? DirEntry_PermWritable | DirEntry_PermReadable : DirEntry_PermReadable;
		info.size = file_info.size;
		info.access_time = file_info.atime;
		info.modify_time = file_info.mtime;
		info.create_time = file_info.ctime;
	}

	return true;
}

bool vfsLocalDir::Create(const std::string& path)
{
	return fs::create_dir(path);
}

bool vfsLocalDir::IsExists(const std::string& path) const
{
	return fs::is_dir(path);
}

bool vfsLocalDir::Rename(const std::string& from, const std::string& to)
{
	return fs::rename(from, to);
}

bool vfsLocalDir::Remove(const std::string& path)
{
	return fs::remove_dir(path);
}

bool vfsLocalDir::IsOpened() const
{
	return m_dir && vfsDirBase::IsOpened();
}
