#include "stdafx.h"
#include "HDD.h"

vfsDeviceHDD::vfsDeviceHDD(const std::string& hdd_path) : m_hdd_path(hdd_path)
{
}

std::shared_ptr<vfsFileBase> vfsDeviceHDD::GetNewFileStream()
{
	return std::make_shared<vfsHDD>(this, m_hdd_path);
}

std::shared_ptr<vfsDirBase> vfsDeviceHDD::GetNewDirStream()
{
	return nullptr;
}
