#pragma once
#include "vfsStream.h"

enum vfsOpenMode
{
	vfsRead = 0x1,
	vfsWrite = 0x2,
	vfsExcl = 0x4,
	vfsAppend = 0x8,
	vfsReadWrite = vfsRead | vfsWrite,
	vfsWriteExcl = vfsWrite | vfsExcl,
	vfsWriteAppend = vfsWrite | vfsAppend,
};

class vfsDevice;

struct vfsFileBase : public vfsStream
{
protected:
	wxString m_path;
	vfsOpenMode m_mode;
	vfsDevice* m_device;

public:
	vfsFileBase(vfsDevice* device);
	virtual ~vfsFileBase();

	virtual bool Open(const wxString& path, vfsOpenMode mode);
	virtual bool Close() override;
	virtual bool Create(const wxString& path) { return false; }
	virtual bool Exists(const wxString& path) { return false; }
	virtual bool Rename(const wxString& from, const wxString& to) { return false; }
	virtual bool Remove(const wxString& path) { return false; }

	wxString GetPath() const;
	vfsOpenMode GetOpenMode() const;
};
