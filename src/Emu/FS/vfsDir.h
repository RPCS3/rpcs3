#pragma once
#include "vfsDirBase.h"

class vfsDir : public vfsDirBase
{
private:
	std::shared_ptr<vfsDirBase> m_stream;

public:
	vfsDir();
	vfsDir(const std::string& path);

	virtual bool Open(const std::string& path) override;
	virtual bool IsOpened() const override;
	virtual void Close() override;
	//virtual std::string GetPath() const override;
};
