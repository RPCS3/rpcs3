#include "stdafx.h"
#include "vfsDeviceLocalFile.h"
#include "vfsLocalFile.h"
#include "vfsLocalDir.h"

std::shared_ptr<vfsFileBase> vfsDeviceLocalFile::GetNewFileStream()
{
	return std::make_shared<vfsLocalFile>(this);
}

std::shared_ptr<vfsDirBase> vfsDeviceLocalFile::GetNewDirStream()
{
	return std::make_shared<vfsLocalDir>(this);
}
