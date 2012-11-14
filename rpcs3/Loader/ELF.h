#pragma once
#include "ELF64.h"
#include "ELF32.h"

enum ElfClass
{
	CLASS_Unknown,
	CLASS_ELF32,
	CLASS_ELF64,
};

struct Elf_Ehdr
{
	u32 e_magic;
	u8  e_class;

	virtual void Show()
	{
	}

	virtual void Load(wxFile& f)
	{
		e_magic	= Read32(f);
		e_class	= Read8(f);
	}

	bool CheckMagic() const { return e_magic == 0x7F454C46; }

	ElfClass GetClass()
	{
		switch(e_class)
		{
		case 1: return CLASS_ELF32;
		case 2: return CLASS_ELF64;
		}

		return CLASS_Unknown;
	}
};

class ELFLoader : public LoaderBase
{
	wxFile& elf_f;
	LoaderBase* loader;

public:
	Elf_Ehdr ehdr;

	ELFLoader(wxFile& f);
	ELFLoader(const wxString& path);
	~ELFLoader() {Close();}

	virtual bool LoadInfo();
	virtual bool LoadData();
	virtual bool Close();
};