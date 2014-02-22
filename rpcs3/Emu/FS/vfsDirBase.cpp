#include "stdafx.h"
#include "vfsDirBase.h"

vfsDirBase::vfsDirBase(vfsDevice* device)
	: m_pos(0)
	, m_device(device)
{
}

vfsDirBase::~vfsDirBase()
{
}

bool vfsDirBase::Open(const wxString& path)
{
	if(IsOpened())
		Close();

	if(!IsExists(path))
		return false;

	m_pos = 0;
	m_cwd += '/' + path;
	return true;
}

bool vfsDirBase::IsOpened() const
{
	return !m_cwd.IsEmpty();
}

bool vfsDirBase::IsExists(const wxString& path) const
{
	return wxDirExists(path);
}

const Array<DirEntryInfo>& vfsDirBase::GetEntries() const
{
	return m_entries;
}

void vfsDirBase::Close()
{
	m_cwd = wxEmptyString;
	m_entries.Clear();
}

wxString vfsDirBase::GetPath() const
{
	return m_cwd;
}

const DirEntryInfo* vfsDirBase::Read()
{
	if (m_pos >= m_entries.GetCount())
		return nullptr;

	return &m_entries[m_pos++];
}