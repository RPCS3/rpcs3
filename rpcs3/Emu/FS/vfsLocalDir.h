#pragma once
#include "vfsDirBase.h"
#include "Utilities/rFile.h"

class vfsLocalDir : public vfsDirBase
{
private:
	u32 m_pos;
	rDir dir;

public:
	vfsLocalDir(vfsDevice* device);
	virtual ~vfsLocalDir();

	virtual bool Open(const std::string& path) override;

	virtual bool Create(const std::string& path) override;
	virtual bool Rename(const std::string& from, const std::string& to) override;
	virtual bool Remove(const std::string& path) override;
	virtual bool IsOpened() const override;
};