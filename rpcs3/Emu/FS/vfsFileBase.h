#pragma once
#include "vfsDevice.h"

struct vfsFileBase : public vfsDevice
{
protected:
	wxString m_path;
	vfsOpenMode m_mode;

public:
	vfsFileBase();
	virtual ~vfsFileBase();

	virtual bool Open(const wxString& path, vfsOpenMode mode) override;
	virtual bool Close() override;
	/*
	virtual bool Create(const wxString& path)=0;
	virtual bool Exists(const wxString& path)=0;
	virtual bool Rename(const wxString& from, const wxString& to)=0;
	virtual bool Remove(const wxString& path)=0;
	*/
	wxString GetPath() const;
	vfsOpenMode GetOpenMode() const;
};
