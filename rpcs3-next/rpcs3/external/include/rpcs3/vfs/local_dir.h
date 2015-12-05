#pragma once
#include "vfsDirBase.h"
#include "Utilities/File.h"

class vfsLocalDir : public vfsDirBase
{
private:
	u32 m_pos;
	fs::dir m_dir;

public:
	vfsLocalDir(vfsDevice* device);
	virtual ~vfsLocalDir();

	virtual bool Open(const std::string& path) override;
	virtual bool IsOpened() const override;
};
