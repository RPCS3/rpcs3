#pragma once
#include "vfsFileBase.h"

class vfsFile : public vfsFileBase
{
private:
	std::shared_ptr<vfsFileBase> m_stream;

public:
	vfsFile();
	vfsFile(const std::string& path, vfsOpenMode mode = vfsRead);

	virtual bool Open(const std::string& path, vfsOpenMode mode = vfsRead) override;
	virtual bool Create(const std::string& path) override;
	virtual bool Exists(const std::string& path) override;
	virtual bool Rename(const std::string& from, const std::string& to) override;
	virtual bool Remove(const std::string& path) override;
	virtual bool Close() override;

	virtual u64 GetSize() override;

	virtual u64 Write(const void* src, u64 size) override;
	virtual u64 Read(void* dst, u64 size) override;

	virtual u64 Seek(s64 offset, vfsSeekMode mode = vfsSeekSet) override;
	virtual u64 Tell() const override;

	virtual bool IsOpened() const override;
};