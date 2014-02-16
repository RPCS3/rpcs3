#pragma once
#include "vfsDirBase.h"

class vfsDir : public vfsDirBase
{
private:
	std::shared_ptr<vfsDirBase> m_stream;

public:
	vfsDir();
	vfsDir(const wxString path);

	virtual bool Open(const wxString& path) override;
	virtual bool IsOpened() const override;
	virtual bool IsExists(const wxString& path) const override;
	virtual const Array<DirEntryInfo>& GetEntries() const override;
	virtual void Close() override;
	virtual wxString GetPath() const override;

	virtual bool Create(const wxString& path) override;
	//virtual bool Create(const DirEntryInfo& info) override;
	virtual bool Rename(const wxString& from, const wxString& to) override;
	virtual bool Remove(const wxString& path) override;
	virtual const DirEntryInfo* Read() override;
};