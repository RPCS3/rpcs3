#pragma once
#include "vfsFileBase.h"

class vfsLocalFile : public vfsFileBase
{
private:
	rfile_t m_file;

public:
	vfsLocalFile(vfsDevice* device);

	virtual bool Open(const std::string& path, u32 mode = vfsRead) override;
	virtual bool Close() override;
	virtual bool Exists(const std::string& path) override;
	virtual bool Rename(const std::string& from, const std::string& to) override;
	virtual bool Remove(const std::string& path) override;

	virtual u64 GetSize() const override;

	virtual u64 Write(const void* src, u64 size) override;
	virtual u64 Read(void* dst, u64 size) override;

	virtual u64 Seek(s64 offset, u32 mode = from_begin) override;
	virtual u64 Tell() const override;

	virtual bool IsOpened() const override;
	
	virtual const rfile_t& GetFile() const
	{
		return m_file;
	}
};
