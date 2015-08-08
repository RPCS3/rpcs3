#pragma once
#include "vfsFileBase.h"

class vfsFile : public vfsFileBase
{
private:
	std::shared_ptr<vfsFileBase> m_stream;

public:
	vfsFile();
	vfsFile(const std::string& path, u32 mode = vfsRead);

	virtual bool Open(const std::string& path, u32 mode = vfsRead) override;
	virtual bool Close() override;

	virtual u64 GetSize() const override;

	virtual u64 Write(const void* src, u64 size) override;
	virtual u64 Read(void* dst, u64 size) override;

	virtual u64 Seek(s64 offset, u32 mode = from_begin) override;
	virtual u64 Tell() const override;

	virtual bool IsOpened() const override;
};
