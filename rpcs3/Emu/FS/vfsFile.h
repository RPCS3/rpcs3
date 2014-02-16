#pragma once
#include "vfsFileBase.h"

class vfsFile : public vfsFileBase
{
private:
	std::shared_ptr<vfsFileBase> m_stream;

public:
	vfsFile();
	vfsFile(const wxString path, vfsOpenMode mode = vfsRead);

	virtual bool Open(const wxString& path, vfsOpenMode mode = vfsRead) override;
	virtual bool Create(const wxString& path) override;
	virtual bool Exists(const wxString& path) override;
	virtual bool Rename(const wxString& from, const wxString& to) override;
	virtual bool Remove(const wxString& path) override;
	virtual bool Close() override;

	virtual u64 GetSize() override;

	virtual u64 Write(const void* src, u64 size) override;
	virtual u64 Read(void* dst, u64 size) override;

	virtual u64 Seek(s64 offset, vfsSeekMode mode = vfsSeekSet) override;
	virtual u64 Tell() const override;

	virtual bool IsOpened() const override;
};