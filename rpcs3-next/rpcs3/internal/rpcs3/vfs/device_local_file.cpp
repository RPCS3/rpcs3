#include "stdafx.h"
#include "vfsDeviceLocalFile.h"
#include "vfsLocalFile.h"
#include "vfsLocalDir.h"

vfsFileBase* vfsDeviceLocalFile::GetNewFileStream()
{
	return new vfsLocalFile(this);
}

vfsDirBase* vfsDeviceLocalFile::GetNewDirStream()
{
	return new vfsLocalDir(this);
}
