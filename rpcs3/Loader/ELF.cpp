#include "stdafx.h"
#include "Loader.h"
#include "ELF.h"

ELFLoader::ELFLoader(wxFile& f)
	: elf_f(f)
	, LoaderBase()
	, loader(NULL)
{
}

ELFLoader::ELFLoader(const wxString& path)
	: elf_f(*new wxFile(path))
	, LoaderBase()
	, loader(NULL)
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

	return true;
}

bool ELFLoader::LoadData()
{
	if(!loader || !loader->LoadData()) return false;
	entry = loader->GetEntry();
	machine = loader->GetMachine();
	return true;
}

bool ELFLoader::Close()
{
	safe_delete(loader);
	return elf_f.Close();
}
