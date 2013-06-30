#include "stdafx.h"
#include "vfsFileBase.h"

vfsFileBase::vfsFileBase() : vfsDevice()
{
}

vfsFileBase::~vfsFileBase()
{
	Close();
}

bool Access(const wxString& path, vfsOpenMode mode)
{
	return false;
}

bool vfsFileBase::Open(const wxString& path, vfsOpenMode mode)
{
	m_path = path;
	m_mode = mode;

	vfsStream::Reset();

	return true;
}

bool vfsFileBase::Close()
{
	m_path = wxEmptyString;

	return vfsStream::Close();
}

wxString vfsFileBase::GetPath() const
{
	return m_path;
}

vfsOpenMode vfsFileBase::GetOpenMode() const
{
	return m_mode;
}