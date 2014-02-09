#pragma once

enum DirEntryFlags
{
	DirEntry_TypeDir = 0x0,
	DirEntry_TypeFile = 0x1,
	DirEntry_TypeMask = 0x1,
	DirEntry_PermWritable = 0x20,
	DirEntry_PermReadable = 0x40,
	DirEntry_PermExecutable = 0x80,
};

struct DirEntryInfo
{
	wxString name;
	u32 flags;
	time_t create_time;
	time_t access_time;
	time_t modify_time;

	DirEntryInfo()
		: flags(0)
		, create_time(0)
		, access_time(0)
		, modify_time(0)
	{
	}
};

class vfsDirBase
{
protected:
	wxString m_cwd;
	Array<DirEntryInfo> m_entries;

public:
	vfsDirBase(const wxString& path);
	virtual ~vfsDirBase();

	virtual bool Open(const wxString& path);
	virtual bool IsOpened() const;
	virtual bool IsExists(const wxString& path) const;
	virtual const Array<DirEntryInfo>& GetEntries() const;
	virtual void Close();
	virtual wxString GetPath() const;

	virtual bool Create(const wxString& path)=0;
	//virtual bool Create(const DirEntryInfo& info)=0;
	virtual bool Rename(const wxString& from, const wxString& to)=0;
	virtual bool Remove(const wxString& path)=0;
};