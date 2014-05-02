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

	rDir dir;

	if(!dir.Open(path))
		return false;

	std::string name;
	for(bool is_ok = dir.GetFirst(&name); is_ok; is_ok = dir.GetNext(&name))
	{
		std::string dir_path = path + name;

		m_entries.emplace_back();
		DirEntryInfo& info = m_entries.back();
		info.name = name;

		info.flags |= dir.Exists(dir_path) ? DirEntry_TypeDir : DirEntry_TypeFile;
		if(rIsWritable(dir_path)) info.flags |= DirEntry_PermWritable;
		if(rIsReadable(dir_path)) info.flags |= DirEntry_PermReadable;
		if(rIsExecutable(dir_path)) info.flags |= DirEntry_PermExecutable;
	}

	return true;
}

bool vfsLocalDir::Create(const std::string& path)
{
	return rFileName::Mkdir(path, 0777, rPATH_MKDIR_FULL);
}

bool vfsLocalDir::Rename(const std::string& from, const std::string& to)
{
	return false;
}

bool vfsLocalDir::Remove(const std::string& path)
{
	return rRmdir(path);
}
