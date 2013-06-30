#pragma once
#include "vfsFileBase.h"

class vfsLocalFile : public vfsFileBase
{
private:
	wxFile m_file;

public:
	vfsLocalFile();
	vfsLocalFile(const wxString path, vfsOpenMode mode = vfsRead);
	vfsDevice* GetNew();

	virtual bool Open(const wxString& path, vfsOpenMode mode = vfsRead);
	virtual bool Create(const wxString& path);
	virtual bool Close();

	virtual u64 GetSize();

	virtual u32 Write(const void* src, u32 size);
	virtual u32 Read(void* dst, u32 size);

	virtual u64 Seek(s64 offset, vfsSeekMode mode = vfsSeekSet);
	virtual u64 Tell() const;

	virtual bool IsOpened() const;
};