#pragma once
#include "vfsDevice.h"

class vfsDeviceLocalFile : public vfsDevice
{
public:
	virtual std::shared_ptr<vfsFileBase> GetNewFileStream() override;
	virtual std::shared_ptr<vfsDirBase> GetNewDirStream() override;
};