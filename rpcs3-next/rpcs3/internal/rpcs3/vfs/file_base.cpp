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

bool vfsFileBase::Open(const std::string& path, u32 mode)
{
	m_path = path;
	m_mode = mode;

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

u32 vfsFileBase::GetOpenMode() const
{
	return m_mode;
}
