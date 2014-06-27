#include "stdafx.h"
#include "Utilities/Log.h"
#include "SELF.h"
#include "ELF64.h"

SELFLoader::SELFLoader(vfsStream& f)
	: self_f(f)
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

bool SELFLoader::LoadData(u64 offset)
{
	if(!self_f.IsOpened()) return false;

	sce_hdr.Show();
	self_hdr.Show();

	ELF64Loader l(self_f);
	if( !l.LoadEhdrInfo(self_hdr.se_elfoff) ||
		!l.LoadPhdrInfo(self_hdr.se_phdroff) ||
		!l.LoadShdrInfo(self_hdr.se_shdroff) ||
		!l.LoadData(self_hdr.se_appinfooff) )
	{
		LOG_ERROR(LOADER, "Broken SELF file.");

		return false;
	}

	machine = l.GetMachine();
	entry = l.GetEntry();

	return true;

	LOG_ERROR(LOADER, "Boot SELF not supported yet!");
	return false;
}