#include "stdafx.h"
#include "ELF32.h"

ELF32Loader::ELF32Loader(vfsStream& f)
	: elf32_f(f)
	, LoaderBase()
{
}

bool ELF32Loader::LoadInfo()
{
	if(!elf32_f.IsOpened()) return false;

	if(!LoadEhdrInfo()) return false;
	if(!LoadPhdrInfo()) return false;
	if(!LoadShdrInfo()) return false;

	return true;
}

bool ELF32Loader::LoadData(u64 offset)
{
	if(!elf32_f.IsOpened()) return false;

	if(!LoadEhdrData(offset)) return false;
	if(!LoadPhdrData(offset)) return false;
	if(!LoadShdrData(offset)) return false;

	return true;
}

bool ELF32Loader::Close()
{
	return elf32_f.Close();
}

bool ELF32Loader::LoadEhdrInfo()
{
	elf32_f.Seek(0);
	ehdr.Load(elf32_f);

	if(!ehdr.CheckMagic()) return false;

	switch(ehdr.e_machine)
	{
	case MACHINE_PPC64:
	case MACHINE_SPU:
		machine = (Elf_Machine)ehdr.e_machine;
	break;

	default:
		machine = MACHINE_Unknown;
		ConLog.Error("Unknown elf32 type: 0x%x", ehdr.e_machine);
		return false;
	}

	entry = ehdr.GetEntry();
	if(entry == 0)
	{
		ConLog.Error("elf32 error: entry is null!");
		return false;
	}

	return true;
}

bool ELF32Loader::LoadPhdrInfo()
{
	if(ehdr.e_phoff == 0 && ehdr.e_phnum)
	{
		ConLog.Error("LoadPhdr32 error: Program header offset is null!");
		return false;
	}

	elf32_f.Seek(ehdr.e_phoff);
	for(uint i=0; i<ehdr.e_phnum; ++i)
	{
		Elf32_Phdr* phdr = new Elf32_Phdr();
		phdr->Load(elf32_f);
		phdr_arr.Move(phdr);
	}

	return true;
}

bool ELF32Loader::LoadShdrInfo()
{
	elf32_f.Seek(ehdr.e_shoff);
	for(u32 i=0; i<ehdr.e_shnum; ++i)
	{
		Elf32_Shdr* shdr = new Elf32_Shdr();
		shdr->Load(elf32_f);
		shdr_arr.Move(shdr);
	}

	if(ehdr.e_shstrndx >= shdr_arr.GetCount())
	{
		ConLog.Error("LoadShdr64 error: shstrndx too big!");
		return false;
	}

	for(u32 i=0; i<shdr_arr.GetCount(); ++i)
	{
		elf32_f.Seek(shdr_arr[ehdr.e_shstrndx].sh_offset + shdr_arr[i].sh_name);
		wxString name = wxEmptyString;
		while(!elf32_f.Eof())
		{
			char c;
			elf32_f.Read(&c, 1);
			if(c == 0) break;
			name += c;
		}

		shdr_name_arr.Add(name);
	}

	return true;
}

bool ELF32Loader::LoadEhdrData(u64 offset)
{
#ifdef LOADER_DEBUG
	ConLog.SkipLn();
	ehdr.Show();
	ConLog.SkipLn();
#endif
	return true;
}

bool ELF32Loader::LoadPhdrData(u64 offset)
{
	for(u32 i=0; i<phdr_arr.GetCount(); ++i)
	{
		phdr_arr[i].Show();

		if(phdr_arr[i].p_vaddr < min_addr)
		{
			min_addr = phdr_arr[i].p_vaddr;
		}

		if(phdr_arr[i].p_vaddr + phdr_arr[i].p_memsz > max_addr)
		{
			max_addr = phdr_arr[i].p_vaddr + phdr_arr[i].p_memsz;
		}

		if(phdr_arr[i].p_type == 0x00000001) //LOAD
		{
			if(phdr_arr[i].p_vaddr != phdr_arr[i].p_paddr)
			{
				ConLog.Warning
				( 
					"LoadPhdr32 different load addrs: paddr=0x%8.8x, vaddr=0x%8.8x", 
					phdr_arr[i].p_paddr, phdr_arr[i].p_vaddr
				);
			}

			Memory.MainMem.Alloc(phdr_arr[i].p_vaddr + offset, phdr_arr[i].p_memsz);
			elf32_f.Seek(phdr_arr[i].p_offset);
			elf32_f.Read(&Memory[phdr_arr[i].p_vaddr + offset], phdr_arr[i].p_filesz);
		}
#ifdef LOADER_DEBUG
		ConLog.SkipLn();
#endif
	}

	return true;
}

bool ELF32Loader::LoadShdrData(u64 offset)
{
	for(u32 i=0; i<shdr_arr.GetCount(); ++i)
	{
		Elf32_Shdr& shdr = shdr_arr[i];

#ifdef LOADER_DEBUG
		if(i < shdr_name_arr.GetCount()) ConLog.Write("Name: %s", shdr_name_arr[i]);
		shdr.Show();
		ConLog.SkipLn();
#endif
		if((shdr.sh_flags & SHF_ALLOC) != SHF_ALLOC) continue;

		if(shdr.sh_addr < min_addr)
		{
			min_addr = shdr.sh_addr;
		}

		if(shdr.sh_addr + shdr.sh_size > max_addr)
		{
			max_addr = shdr.sh_addr + shdr.sh_size;
		}

		//const s64 addr = shdr.sh_addr;
		//const s64 size = shdr.sh_size;
		//MemoryBlock* mem = nullptr;

		/*
		switch(shdr.sh_type)
		{
		case SHT_NOBITS:
			if(size == 0) continue;
			memset(&Memory[addr + offset], 0, size);
		case SHT_PROGBITS:
		break;
		}
		*/
	}

	//TODO
	return true;
}