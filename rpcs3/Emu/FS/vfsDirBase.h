#pragma once

struct DirInfo
{
	wxString m_name;

};

class vfsDirBase
{
	virtual bool Open(const wxString& path)=0;
	virtual Array<DirInfo> GetEntryes()=0;
	virtual void Close()=0;

	virtual bool Create(const wxString& path)=0;
	virtual bool Exists(const wxString& path)=0;
	virtual bool Rename(const wxString& from, const wxString& to)=0;
	virtual bool Remove(const wxString& path)=0;
};