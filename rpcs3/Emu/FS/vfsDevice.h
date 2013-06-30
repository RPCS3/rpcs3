#pragma once
#include "vfsStream.h"

enum vfsOpenMode
{
	vfsRead,
	vfsWrite,
	vfsReadWrite,
	vfsWriteExcl,
	vfsWriteAppend,
};

class vfsDevice : public vfsStream
{
	wxString m_ps3_path;
	wxString m_local_path;

public:
	vfsDevice(const wxString& ps3_path, const wxString& local_path);
	vfsDevice() {}

	virtual vfsDevice* GetNew()=0;
	virtual bool Open(const wxString& path, vfsOpenMode mode = vfsRead)=0;
	virtual bool Create(const wxString& path)=0;
	wxString GetLocalPath() const;
	wxString GetPs3Path() const;

	void SetPath(const wxString& ps3_path, const wxString& local_path);

	u32 CmpPs3Path(const wxString& ps3_path);

	static wxString ErasePath(const wxString& local_path, u32 start_dir_count, u32 end_dir_count);
	static wxString GetRoot(const wxString& local_path);
	static wxString GetRootPs3(const wxString& local_path);
	static wxString GetWinPath(const wxString& p, bool is_dir = true);
	static wxString GetWinPath(const wxString& l, const wxString& r);
	static wxString GetPs3Path(const wxString& p, bool is_dir = true);
	static wxString GetPs3Path(const wxString& l, const wxString& r);
};