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

	virtual bool Create(const std::string& path) override;
	virtual bool Rename(const std::string& from, const std::string& to) override;
	virtual bool Remove(const std::string& path) override;
	virtual bool IsOpened() const override;
	virtual bool IsExists(const std::string& path) const;
};
