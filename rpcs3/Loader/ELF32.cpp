#include "stdafx.h"
#include "ELF32.h"

void WriteEhdr(wxFile& f, Elf32_Ehdr& ehdr)
{
	Write32(f, ehdr.e_magic);
	Write8(f, ehdr.e_class);
	Write8(f, ehdr.e_data);
	Write8(f, ehdr.e_curver);
	Write8(f, ehdr.e_os_abi);
	Write64(f, ehdr.e_abi_ver);
	Write16(f, ehdr.e_type);
	Write16(f, ehdr.e_machine);
	Write32(f, ehdr.e_version);
	Write32(f, ehdr.e_entry);
	Write32(f, ehdr.e_phoff);
	Write32(f, ehdr.e_shoff);
	Write32(f, ehdr.e_flags);
	Write16(f, ehdr.e_ehsize);
	Write16(f, ehdr.e_phentsize);
	Write16(f, ehdr.e_phnum);
	Write16(f, ehdr.e_shentsize);
	Write16(f, ehdr.e_shnum);
	Write16(f, ehdr.e_shstrndx);
}

void WritePhdr(wxFile& f, Elf32_Phdr& phdr)
{
	Write32(f, phdr.p_type);
	Write32(f, phdr.p_offset);
	Write32(f, phdr.p_vaddr);
	Write32(f, phdr.p_paddr);
	Write32(f, phdr.p_filesz);
	Write32(f, phdr.p_memsz);
	Write32(f, phdr.p_flags);
	Write32(f, phdr.p_align);
}

void WriteShdr(wxFile& f, Elf32_Shdr& shdr)
{
	Write32(f, shdr.sh_name);
	Write32(f, shdr.sh_type);
	Write32(f, shdr.sh_flags);
	Write32(f, shdr.sh_addr);
	Write32(f, shdr.sh_offset);
	Write32(f, shdr.sh_size);
	Write32(f, shdr.sh_link);
	Write32(f, shdr.sh_info);
	Write32(f, shdr.sh_addralign);
	Write32(f, shdr.sh_entsize);
}

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

	if(ehdr.IsLittleEndian())
		ConLog.Warning("ELF32 LE");

	switch(ehdr.e_machine)
	{
	case MACHINE_MIPS:
	case MACHINE_PPC64:
	case MACHINE_SPU:
	case MACHINE_ARM:
		machine = (Elf_Machine)ehdr.e_machine;
	break;

	default:
		machine = MACHINE_Unknown;
		ConLog.Error("Unknown elf32 machine: 0x%x", ehdr.e_machine);
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
		phdr_arr.emplace_back();
		if(ehdr.IsLittleEndian())
			phdr_arr.back().LoadLE(elf32_f);
		else
			phdr_arr.back().Load(elf32_f);
	}

	if(/*!Memory.IsGoodAddr(entry)*/ entry & 0x1)
	{
		//entry is physical, convert to virtual

		entry &= ~0x1;

		for(size_t i=0; i<phdr_arr.size(); ++i)
		{
			if(phdr_arr[i].p_paddr >= entry && entry < phdr_arr[i].p_paddr + phdr_arr[i].p_memsz)
			{
				entry += phdr_arr[i].p_vaddr;
				ConLog.Warning("virtual entry = 0x%x", entry);
				break;
			}
		}
	}

	return true;
}

bool ELF32Loader::LoadShdrInfo()
{
	elf32_f.Seek(ehdr.e_shoff);
	for(u32 i=0; i<ehdr.e_shnum; ++i)
	{
		shdr_arr.emplace_back();
		if(ehdr.IsLittleEndian())
			shdr_arr.back().LoadLE(elf32_f);
		else
			shdr_arr.back().Load(elf32_f);

	}

	if(ehdr.e_shstrndx >= shdr_arr.size())
	{
		ConLog.Warning("LoadShdr32 error: shstrndx too big!");
		return true;
	}

	for(u32 i=0; i<shdr_arr.size(); ++i)
	{
		elf32_f.Seek(shdr_arr[ehdr.e_shstrndx].sh_offset + shdr_arr[i].sh_name);
		std::string name;
		while(!elf32_f.Eof())
		{
			char c;
			elf32_f.Read(&c, 1);
			if(c == 0) break;
			name.push_back(c);
		}
		shdr_name_arr.push_back(name);
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

bool ELF32Loader::LoadPhdrData(u64 _offset)
{
	const u64 offset = machine == MACHINE_SPU ? _offset : 0;

	for(u32 i=0; i<phdr_arr.size(); ++i)
	{
		phdr_arr[i].Show();

		if(phdr_arr[i].p_type == 0x00000001) //LOAD
		{
			if(phdr_arr[i].p_vaddr < min_addr)
			{
				min_addr = phdr_arr[i].p_vaddr;
			}

			if(phdr_arr[i].p_vaddr + phdr_arr[i].p_memsz > max_addr)
			{
				max_addr = phdr_arr[i].p_vaddr + phdr_arr[i].p_memsz;
			}

			if(phdr_arr[i].p_vaddr != phdr_arr[i].p_paddr)
			{
				ConLog.Warning
				( 
					"LoadPhdr32 different load addrs: paddr=0x%8.8x, vaddr=0x%8.8x", 
					phdr_arr[i].p_paddr, phdr_arr[i].p_vaddr
				);
			}

			switch(machine)
			{
			case MACHINE_SPU: Memory.MainMem.AllocFixed(phdr_arr[i].p_vaddr + offset, phdr_arr[i].p_memsz); break;
			case MACHINE_MIPS: Memory.PSPMemory.RAM.AllocFixed(phdr_arr[i].p_vaddr + offset, phdr_arr[i].p_memsz); break;
			case MACHINE_ARM: Memory.PSVMemory.RAM.AllocFixed(phdr_arr[i].p_vaddr + offset, phdr_arr[i].p_memsz); break;

			default:
				continue;
			}

			elf32_f.Seek(phdr_arr[i].p_offset);
			elf32_f.Read(&Memory[phdr_arr[i].p_vaddr + offset], phdr_arr[i].p_filesz);
		}
		else if(phdr_arr[i].p_type == 0x00000004)
		{
			elf32_f.Seek(phdr_arr[i].p_offset);
			Elf32_Note note;
			if(ehdr.IsLittleEndian()) note.LoadLE(elf32_f);
			else note.Load(elf32_f);

			if(note.type != 1)
			{
				ConLog.Error("ELF32: Bad NOTE type (%d)", note.type);
				break;
			}

			if(note.namesz != sizeof(note.name))
			{
				ConLog.Error("ELF32: Bad NOTE namesz (%d)", note.namesz);
				break;
			}

			if(note.descsz != sizeof(note.desc) && note.descsz != 32)
			{
				ConLog.Error("ELF32: Bad NOTE descsz (%d)", note.descsz);
				break;
			}

			//if(note.desc.flags)
			//{
			//	ConLog.Error("ELF32: Bad NOTE flags (0x%x)", note.desc.flags);
			//	break;
			//}

			if(note.descsz == sizeof(note.desc))
			{
				ConLog.Warning("name = %s", std::string((const char *)note.name, 8).c_str());
				ConLog.Warning("ls_size = %d", note.desc.ls_size);
				ConLog.Warning("stack_size = %d", note.desc.stack_size);
			}
			else
			{
				ConLog.Warning("desc = '%s'", std::string(note.desc_text, 32).c_str());
			}
		}
#ifdef LOADER_DEBUG
		ConLog.SkipLn();
#endif
	}

	return true;
}

bool ELF32Loader::LoadShdrData(u64 offset)
{
	for(u32 i=0; i<shdr_arr.size(); ++i)
	{
		Elf32_Shdr& shdr = shdr_arr[i];

#ifdef LOADER_DEBUG
		if(i < shdr_name_arr.size()) ConLog.Write("Name: %s", shdr_name_arr[i].c_str());
		shdr.Show();
		ConLog.SkipLn();
#endif
		if((shdr.sh_type == SHT_RELA) || (shdr.sh_type == SHT_REL))
		{
			ConLog.Error("ELF32 ERROR: Relocation");
			continue;
		}
		if((shdr.sh_flags & SHF_ALLOC) != SHF_ALLOC) continue;

		if(shdr.sh_addr < min_addr)
		{
			min_addr = shdr.sh_addr;
		}

		if(shdr.sh_addr + shdr.sh_size > max_addr)
		{
			max_addr = shdr.sh_addr + shdr.sh_size;
		}
	}

	//TODO
	return true;
}
