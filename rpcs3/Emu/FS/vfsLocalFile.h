#pragma once

#include "vfsFileBase.h"

class vfsLocalFile : public vfsFileBase
{
private:
	fs::file m_file;

public:
	vfsLocalFile(vfsDevice* device);

	virtual bool Open(const std::string& path, u32 mode = fom::read) override;
	virtual bool Close() override;

	virtual u64 GetSize() const override;

	virtual u64 Write(const void* src, u64 size) override;
	virtual u64 Read(void* dst, u64 size) override;

	virtual u64 Seek(s64 offset, fsm seek_mode = fsm::begin) override;
	virtual u64 Tell() const override;

	virtual bool IsOpened() const override;
	
	virtual const fs::file& GetFile() const { return m_file; }
};
