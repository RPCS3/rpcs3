#include "stdafx.h"
#include "vfsFileBase.h"

vfsFileBase::vfsFileBase(vfsDevice* device)
	: vfsStream()
	, m_device(device)
{
}

vfsFileBase::~vfsFileBase()
{
	Close();
}

bool Access(const std::string& path, vfsOpenMode mode)
{
	return false;
}

bool vfsFileBase::Open(const std::string& path, vfsOpenMode mode)
{
	m_path = path;
	m_mode = mode;

	vfsStream::Reset();

	return true;
}

bool vfsFileBase::Close()
{
	m_path = "";

	return vfsStream::Close();
}

std::string vfsFileBase::GetPath() const
{
	return m_path;
}

vfsOpenMode vfsFileBase::GetOpenMode() const
{
	return m_mode;
}
