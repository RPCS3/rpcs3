#include "stdafx.h"
#include "HDD.h"

vfsDeviceHDD::vfsDeviceHDD(const std::string& hdd_path) : m_hdd_path(hdd_path)
{
}

vfsFileBase* vfsDeviceHDD::GetNewFileStream()
{
	return new vfsHDD(this, m_hdd_path);
}

vfsDirBase* vfsDeviceHDD::GetNewDirStream()
{
	return nullptr;
}
