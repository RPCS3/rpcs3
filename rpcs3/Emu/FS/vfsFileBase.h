#pragma once
#include "vfsStream.h"

enum vfsOpenMode : u32
{
	vfsRead = o_read,
	vfsReadWrite = o_read | o_write,
	vfsWriteNew = o_write | o_create | o_trunc,
	vfsWriteExcl = o_write | o_create | o_excl,
};

class vfsDevice;

struct vfsFileBase : public vfsStream
{
protected:
	std::string m_path;
	u32 m_mode;
	vfsDevice* m_device;

public:
	vfsFileBase(vfsDevice* device);
	virtual ~vfsFileBase() override;

	virtual bool Open(const std::string& path, u32 mode);
	virtual bool Close() override;
	virtual bool IsOpened() const override { return !m_path.empty(); }

	std::string GetPath() const;
	u32 GetOpenMode() const;
};
