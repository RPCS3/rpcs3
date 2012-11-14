#include "stdafx.h"
#include "Loader.h"
#include "ELF.h"
#include "SELF.h"
#include "PSF.h"

u8 Read8(wxFile& f)
{
	u8 ret;
	f.Read(&ret, sizeof(u8));
	return ret;
}

u16 Read16(wxFile& f)
{
	const u8 c0 = Read8(f);
	const u8 c1 = Read8(f);

	return ((u16)c0 << 8) | (u16)c1;
}

u32 Read32(wxFile& f)
{
	const u16 c0 = Read16(f);
	const u16 c1 = Read16(f);

	return ((u32)c0 << 16) | (u32)c1;
}

u64 Read64(wxFile& f)
{
	const u32 c0 = Read32(f);
	const u32 c1 = Read32(f);

	return ((u64)c0 << 32) | (u64)c1;
}

const wxString Ehdr_DataToString(const u8 data)
{
	if(data > 1) return wxString::Format("%d's complement, big endian", data);
	if(data < 1) return "Data is not found";

	return wxString::Format("%d's complement, small endian", data);
}

const wxString Ehdr_TypeToString(const u16 type)
{
	switch(type)
	{
	case 0: return "NULL";
	case 2: return "EXEC (Executable file)";
	};

	return wxString::Format("Unknown (%d)", type);
}

const wxString Ehdr_OS_ABIToString(const u8 os_abi)
{
	switch(os_abi)
	{
	case 0x0 : return "UNIX System V";
	case 0x66: return "Cell OS LV-2";
	};

	return wxString::Format("Unknown (%x)", os_abi);
}

const wxString Ehdr_MachineToString(const u16 machine)
{
	switch(machine)
	{
	case MACHINE_PPC64: return "PowerPC64";
	case MACHINE_SPU:	return "SPU";
	};

	return wxString::Format("Unknown (%x)", machine);
}

const wxString Phdr_FlagsToString(u32 flags)
{
	enum {ppu_R = 0x1, ppu_W = 0x2, ppu_E = 0x4};
	enum {spu_E = 0x1, spu_W = 0x2, spu_R = 0x4};
	enum {rsx_R = 0x1, rsx_W = 0x2, rsx_E = 0x4};

#define FLAGS_TO_STRING(f) \
	wxString(f & ##f##_R ? "R" : "-") + \
	wxString(f & ##f##_W ? "W" : "-") + \
	wxString(f & ##f##_E ? "E" : "-")

	const u8 ppu = flags & 0xf;
	const u8 spu = (flags >> 0x14) & 0xf;
	const u8 rsx = (flags >> 0x18) & 0xf;

	wxString ret = wxEmptyString;
	ret += wxString::Format("[0x%x] ", flags);

	flags &= ~ppu;
	flags &= ~spu << 0x14;
	flags &= ~rsx << 0x18;

	if(flags != 0) return wxString::Format("Unknown %s PPU[%x] SPU[%x] RSX[%x]", ret, ppu, spu, rsx);

	ret += "PPU[" + FLAGS_TO_STRING(ppu) + "] ";
	ret += "SPU[" + FLAGS_TO_STRING(spu) + "] ";
	ret += "RSX[" + FLAGS_TO_STRING(rsx) + "]";

	return ret;
}

const wxString Phdr_TypeToString(const u32 type)
{
	switch(type)
	{
	case 0x00000001: return "LOAD";
	case 0x00000004: return "NOTE";
	case 0x00000007: return "TLS";
	case 0x60000001: return "LOOS+1";
	case 0x60000002: return "LOOS+2";
	};

	return wxString::Format("Unknown (%x)", type);
}

Loader::Loader() : f(NULL)
{
}

Loader::Loader(const wxString& path) : f(new wxFile(path))
{
}

void Loader::Open(const wxString& path)
{
	m_path = path;
	f = new wxFile(path);
}

void Loader::Open(wxFile& _f, const wxString& path)
{
	m_path = path;
	f = &_f;
}

LoaderBase* Loader::SearchLoader()
{
	if(!f) return NULL;

	LoaderBase* l = NULL;

	if((l=new ELFLoader(*f))->LoadInfo()) return l;
	safe_delete(l);
	if((l=new SELFLoader(*f))->LoadInfo()) return l;
	safe_delete(l);
	return NULL;
}

bool Loader::Load()
{
	LoaderBase* l = SearchLoader();
	if(!l)
	{
		ConLog.Error("Unknown file type");
		return false;
	}

	if(!l->LoadData())
	{
		ConLog.Error("Broken file");
		safe_delete(l);
		return false;
	}

	machine = l->GetMachine();
	entry = l->GetEntry();
	safe_delete(l);

	const wxString& root = wxFileName(wxFileName(m_path).GetPath()).GetPath();
	const wxString& psf_path = root + "\\" + "PARAM.SFO";
	if(wxFileExists(psf_path))
	{
		PSFLoader psf_l(psf_path);
		if(psf_l.Load())
		{
			CurGameInfo = psf_l.m_info;
			CurGameInfo.root = root;
			psf_l.Close();
		}
	}

	return true;
}

Loader::~Loader()
{
	if(f)
	{
		f->Close();
		f = NULL;
	}
}