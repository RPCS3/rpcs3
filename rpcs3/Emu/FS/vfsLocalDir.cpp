#include "stdafx.h"
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

	wxDir dir;

	if(!dir.Open(path))
		return false;

	wxString name;
	for(bool is_ok = dir.GetFirst(&name); is_ok; is_ok = dir.GetNext(&name))
	{
		wxString dir_path = fmt::FromUTF8(path) + name;

		m_entries.emplace_back();
		DirEntryInfo& info = m_entries.back();
		info.name = fmt::ToUTF8(name);

		info.flags |= dir.Exists(dir_path) ? DirEntry_TypeDir : DirEntry_TypeFile;
		if(wxIsWritable(dir_path)) info.flags |= DirEntry_PermWritable;
		if(wxIsReadable(dir_path)) info.flags |= DirEntry_PermReadable;
		if(wxIsExecutable(dir_path)) info.flags |= DirEntry_PermExecutable;
	}

	return true;
}

bool vfsLocalDir::Create(const std::string& path)
{
	return wxFileName::Mkdir(fmt::FromUTF8(path), 0777, wxPATH_MKDIR_FULL);
}

bool vfsLocalDir::Rename(const std::string& from, const std::string& to)
{
	return false;
}

bool vfsLocalDir::Remove(const std::string& path)
{
	return wxRmdir(fmt::FromUTF8(path));
}
