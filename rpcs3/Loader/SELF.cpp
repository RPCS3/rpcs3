#include "stdafx.h"
#include "Utilities/Log.h"
#include "SELF.h"
#include "ELF64.h"

void SceHeader::Load(vfsStream& f)
{
	se_magic = Read32(f);
	se_hver  = Read32(f);
	se_flags = Read16(f);
	se_type  = Read16(f);
	se_meta  = Read32(f);
	se_hsize = Read64(f);
	se_esize = Read64(f);
}

void SceHeader::Show()
{
	LOG_NOTICE(LOADER, "Magic: %08x", se_magic);
	LOG_NOTICE(LOADER, "Class: %s", "SELF");
	LOG_NOTICE(LOADER, "hver: 0x%08x", se_hver);
	LOG_NOTICE(LOADER, "flags: 0x%04x", se_flags);
	LOG_NOTICE(LOADER, "type: 0x%04x", se_type);
	LOG_NOTICE(LOADER, "meta: 0x%08x", se_meta);
	LOG_NOTICE(LOADER, "hsize: 0x%llx", se_hsize);
	LOG_NOTICE(LOADER, "esize: 0x%llx", se_esize);
}

void SelfHeader::Load(vfsStream& f)
{
	se_htype       = Read64(f);
	se_appinfooff  = Read64(f);
	se_elfoff      = Read64(f);
	se_phdroff     = Read64(f);
	se_shdroff     = Read64(f);
	se_secinfoff   = Read64(f);
	se_sceveroff   = Read64(f);
	se_controloff  = Read64(f);
	se_controlsize = Read64(f);
	pad            = Read64(f);
}

void SelfHeader::Show()
{
	LOG_NOTICE(LOADER, "header type: 0x%llx", se_htype);
	LOG_NOTICE(LOADER, "app info offset: 0x%llx", se_appinfooff);
	LOG_NOTICE(LOADER, "elf offset: 0x%llx", se_elfoff);
	LOG_NOTICE(LOADER, "program header offset: 0x%llx", se_phdroff);
	LOG_NOTICE(LOADER, "section header offset: 0x%llx", se_shdroff);
	LOG_NOTICE(LOADER, "section info offset: 0x%llx", se_secinfoff);
	LOG_NOTICE(LOADER, "sce version offset: 0x%llx", se_sceveroff);
	LOG_NOTICE(LOADER, "control info offset: 0x%llx", se_controloff);
	LOG_NOTICE(LOADER, "control info size: 0x%llx", se_controlsize);
}

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