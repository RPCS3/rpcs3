#include "stdafx.h"
#include "SELF.h"

SELFLoader::SELFLoader(wxFile& f)
	: self_f(f)
	, LoaderBase()
{
}

SELFLoader::SELFLoader(const wxString& path)
	: self_f(*new wxFile(path))
	, LoaderBase()
{
}

bool SELFLoader::LoadInfo()
{
	if(!self_f.IsOpened()) return false;
	self_f.Seek(0);
	sce_hdr.Load(self_f);
	self_hdr.Load(self_f);
	if(!sce_hdr.CheckMagic()) return false;

	return true;
}

bool SELFLoader::LoadData()
{
	if(!self_f.IsOpened()) return false;

	sce_hdr.Show();
	self_hdr.Show();
	ConLog.Error("Boot SELF not supported yet!");
	return false;
}