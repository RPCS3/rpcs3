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
	if(!vfsDirBase::Open(path))
		return false;

	if(!dir.Open(path))
		return false;

	std::string name;
	for(bool is_ok = dir.GetFirst(&name); is_ok; is_ok = dir.GetNext(&name))
	{
		std::string dir_path = path + "/" + name;

		m_entries.emplace_back();
		// TODO: Use same info structure as fileinfo?
		DirEntryInfo& info = m_entries.back();
		info.name = name;

		FileInfo fileinfo;
		getFileInfo(dir_path.c_str(), &fileinfo);

		// Not sure of purpose for below. I hope these don't need to be correct
		info.flags |= rIsDir(dir_path) ? DirEntry_TypeDir : DirEntry_TypeFile;
		if(fileinfo.isWritable) info.flags |= DirEntry_PermWritable;
		info.flags |= DirEntry_PermReadable; // Always?
		info.flags |= DirEntry_PermExecutable; // Always?
	}

	return true;
}

bool vfsLocalDir::Create(const std::string& path)
{
	return rMkpath(path);
}

bool vfsLocalDir::IsExists(const std::string& path) const
{
	return rIsDir(path);
}

bool vfsLocalDir::Rename(const std::string& from, const std::string& to)
{
	return false;
}

bool vfsLocalDir::Remove(const std::string& path)
{
	return rRmdir(path);
}

bool vfsLocalDir::IsOpened() const
{
	return dir.IsOpened() && vfsDirBase::IsOpened();
}
