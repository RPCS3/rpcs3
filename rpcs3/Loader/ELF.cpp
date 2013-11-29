#include "stdafx.h"
#include "Loader.h"
#include "ELF.h"

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
	_text_section_offset = loader->GetTextEntry();

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
