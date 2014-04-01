#include "stdafx.h"
#include "Loader.h"
#include "ELF.h"
#include "SELF.h"
#include "PSF.h"
#include "Emu/FS/vfsLocalFile.h"

static const u64 g_spu_offset = 0x10000;

const std::string Ehdr_DataToString(const u8 data)
{
	if(data > 1) return fmt::Format("%d's complement, big endian", data);
	if(data < 1) return "Data is not found";

	return fmt::Format("%d's complement, small endian", data);
}

const std::string Ehdr_TypeToString(const u16 type)
{
	switch(type)
	{
	case 0: return "NULL";
	case 2: return "EXEC (Executable file)";
	};

	return fmt::Format("Unknown (%d)", type);
}

const std::string Ehdr_OS_ABIToString(const u8 os_abi)
{
	switch(os_abi)
	{
	case 0x0 : return "UNIX System V";
	case 0x66: return "Cell OS LV-2";
	};

	return fmt::Format("Unknown (%x)", os_abi);
}

const std::string Ehdr_MachineToString(const u16 machine)
{
	switch(machine)
	{
	case MACHINE_MIPS:	return "MIPS";
	case MACHINE_PPC64:	return "PowerPC64";
	case MACHINE_SPU:	return "SPU";
	case MACHINE_ARM:	return "ARM";
	};

	return fmt::Format("Unknown (%x)", machine);
}

const std::string Phdr_FlagsToString(u32 flags)
{
	enum {ppu_R = 0x1, ppu_W = 0x2, ppu_E = 0x4};
	enum {spu_E = 0x1, spu_W = 0x2, spu_R = 0x4};
	enum {rsx_R = 0x1, rsx_W = 0x2, rsx_E = 0x4};

#define FLAGS_TO_STRING(f) \
	std::string(f & f##_R ? "R" : "-") + \
	std::string(f & f##_W ? "W" : "-") + \
	std::string(f & f##_E ? "E" : "-")

	const u8 ppu = flags & 0xf;
	const u8 spu = (flags >> 0x14) & 0xf;
	const u8 rsx = (flags >> 0x18) & 0xf;

	std::string ret;
	ret += fmt::Format("[0x%x] ", flags);

	flags &= ~ppu;
	flags &= ~spu << 0x14;
	flags &= ~rsx << 0x18;

	if(flags != 0) return fmt::Format("Unknown %s PPU[%x] SPU[%x] RSX[%x]", ret.c_str(), ppu, spu, rsx);

	ret += "PPU[" + FLAGS_TO_STRING(ppu) + "] ";
	ret += "SPU[" + FLAGS_TO_STRING(spu) + "] ";
	ret += "RSX[" + FLAGS_TO_STRING(rsx) + "]";

	return ret;
}

const std::string Phdr_TypeToString(const u32 type)
{
	switch(type)
	{
	case 0x00000001: return "LOAD";
	case 0x00000004: return "NOTE";
	case 0x00000007: return "TLS";
	case 0x60000001: return "LOOS+1";
	case 0x60000002: return "LOOS+2";
	};

	return fmt::Format("Unknown (%x)", type);
}

Loader::Loader()
	: m_stream(nullptr)
	, m_loader(nullptr)
{
}

Loader::Loader(vfsFileBase& stream)
	: m_stream(&stream)
	, m_loader(nullptr)
{
}

Loader::~Loader()
{
	delete m_loader;
	m_loader = nullptr;
}

void Loader::Open(vfsFileBase& stream)
{
	m_stream = &stream;
}

LoaderBase* Loader::SearchLoader()
{
	if(!m_stream) return nullptr;

	LoaderBase* l;

	if((l=new ELFLoader(*m_stream))->LoadInfo()) return l;
	delete l;

	if((l=new SELFLoader(*m_stream))->LoadInfo()) return l;
	delete l;

	return nullptr;
}

bool Loader::Analyze()
{
	delete m_loader;

	m_loader = SearchLoader();

	if(!m_loader)
	{
		ConLog.Error("Unknown file type");
		return false;
	}

	machine = m_loader->GetMachine();
	entry = m_loader->GetMachine() == MACHINE_SPU ? m_loader->GetEntry() + g_spu_offset : m_loader->GetEntry();

	return true;
}

bool Loader::Load()
{
	if(!m_loader)
		return false;

	if(!m_loader->LoadData(m_loader->GetMachine() == MACHINE_SPU ? g_spu_offset : 0))
	{
		ConLog.Error("Broken file");
		return false;
	}

	/*
	const std::string& root = fmt::ToUTF8(wxFileName(wxFileName(m_stream->GetPath()).GetPath()).GetPath());
	std::string ps3_path;
	const std::string& psf_path = root + "/" + "PARAM.SFO";
	vfsFile f(psf_path);
	if(f.IsOpened())
	{
		PSFLoader psf_l(f);
		if(psf_l.Load())
		{
			CurGameInfo = psf_l.m_info;
			CurGameInfo.root = root;
			psf_l.Close();
		}
	}
	*/
	return true;
}
