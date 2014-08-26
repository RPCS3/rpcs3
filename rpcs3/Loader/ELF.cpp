#include "stdafx.h"
#include "Emu/FS/vfsStream.h"
#include "ELF.h"

void Elf_Ehdr::Show()
{
}

void Elf_Ehdr::Load(vfsStream& f)
{
	e_magic = Read32(f);
	e_class = Read8(f);
}

ELFLoader::ELFLoader(vfsStream& f)
	: elf_f(f)
	, LoaderBase()
	, loader(nullptr)
{
}

bool ELFLoader::LoadInfo()
{
	if(!elf_f.IsOpened()) return false;

	elf_f.Seek(0);
	ehdr.Load(elf_f);
	if(!ehdr.CheckMagic()) return false;

	switch(ehdr.GetClass())
	{
	case CLASS_ELF32: loader = new ELF32Loader(elf_f); break;
	case CLASS_ELF64: loader = new ELF64Loader(elf_f); break;
	}

	if(!loader || !loader->LoadInfo()) return false;

	entry = loader->GetEntry();
	machine = loader->GetMachine();

	return true;
}

bool ELFLoader::LoadData(u64 offset)
{
	if(!loader || !loader->LoadData(offset)) return false;
	return true;
}

bool ELFLoader::Close()
{
	delete loader;
	loader = nullptr;
	return elf_f.Close();
}
