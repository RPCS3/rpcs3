#pragma once

#include "vfsFileBase.h"

class vfsFile : public vfsFileBase
{
private:
	std::shared_ptr<vfsFileBase> m_stream;

public:
	vfsFile();
	vfsFile(const std::string& path, u32 mode = fom::read);

	virtual bool Open(const std::string& path, u32 mode = fom::read) override;
	virtual void Close() override;

	virtual u64 GetSize() const override;

	virtual u64 Write(const void* src, u64 size) override;
	virtual u64 Read(void* dst, u64 size) override;

	virtual u64 Seek(s64 offset, fs::seek_mode whence = fs::seek_set) override;
	virtual u64 Tell() const override;

	virtual bool IsOpened() const override;
};
