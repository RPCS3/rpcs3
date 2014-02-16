#pragma once
#include "vfsDevice.h"

class vfsDeviceLocalFile : public vfsDevice
{
public:
	virtual vfsFileBase* GetNewFileStream() override;
	virtual vfsDirBase* GetNewDirStream() override;
};