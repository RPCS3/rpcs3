#pragma once
#include "ELF64.h"
#include "ELF32.h"
#include "Emu/FS/vfsStream.h"

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

	virtual void Load(vfsStream& f)
	{
		e_magic	= Read32(f);
		e_class	= Read8(f);
	}

	bool CheckMagic() const { return e_magic == 0x7F454C46; }

	ElfClass GetClass() const
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
	vfsStream& elf_f;
	LoaderBase* loader;

public:
	Elf_Ehdr ehdr;

	ELFLoader(vfsStream& f);
	virtual ~ELFLoader() {Close();}

	virtual bool LoadInfo();
	virtual bool LoadData(u64 offset = 0);
	virtual bool Close();
};