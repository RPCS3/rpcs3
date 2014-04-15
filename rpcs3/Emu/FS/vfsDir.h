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
	virtual bool IsExists(const std::string& path) const override;
	virtual const std::vector<DirEntryInfo>& GetEntries() const override;
	virtual void Close() override;
	virtual std::string GetPath() const override;

	virtual bool Create(const std::string& path) override;
	//virtual bool Create(const DirEntryInfo& info) override;
	virtual bool Rename(const std::string& from, const std::string& to) override;
	virtual bool Remove(const std::string& path) override;
	virtual const DirEntryInfo* Read() override;
};
