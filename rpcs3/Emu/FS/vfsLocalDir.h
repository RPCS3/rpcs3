#pragma once
#include "vfsDirBase.h"

class vfsLocalDir : public vfsDirBase
{
public:
	vfsLocalDir(const wxString& path = wxEmptyString);
	virtual ~vfsLocalDir();

	virtual bool Open(const wxString& path) override;

	virtual bool Create(const wxString& path) override;
	virtual bool IsExists(const wxString& path) const override;
	virtual bool Rename(const wxString& from, const wxString& to) override;
	virtual bool Remove(const wxString& path) override;
};