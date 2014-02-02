#include "stdafx.h"
#include "vfsDirBase.h"

vfsDirBase::vfsDirBase(const wxString& path)
{
	Open(path);
}

vfsDirBase::~vfsDirBase()
{
}

bool vfsDirBase::Open(const wxString& path)
{
	if(!IsOpened())
		Close();

	if(!IsExists(path))
		return false;

	m_cwd += '/' + path;
	return true;
}

bool vfsDirBase::IsOpened() const
{
	return !m_cwd.IsEmpty();
}

const Array<DirEntryInfo>& vfsDirBase::GetEntryes() const
{
	return m_entryes;
}

void vfsDirBase::Close()
{
	m_cwd = wxEmptyString;
	m_entryes.Clear();
}

wxString vfsDirBase::GetPath() const
{
	return m_cwd;
}