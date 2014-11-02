#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/FS/vfsStream.h"
#include "Emu/Memory/Memory.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "ELF32.h"

//#define LOADER_DEBUG

void Elf32_Ehdr::Show()
{
#ifdef LOADER_DEBUG
	LOG_NOTICE(LOADER, "Magic: %08x", e_magic);
	LOG_NOTICE(LOADER, "Class: %s", "ELF32");
	LOG_NOTICE(LOADER, "Data: %s", Ehdr_DataToString(e_data).c_str());
	LOG_NOTICE(LOADER, "Current Version: %d", e_curver);
	LOG_NOTICE(LOADER, "OS/ABI: %s", Ehdr_OS_ABIToString(e_os_abi).c_str());
	LOG_NOTICE(LOADER, "ABI version: %lld", e_abi_ver);
	LOG_NOTICE(LOADER, "Type: %s", Ehdr_TypeToString(e_type).c_str());
	LOG_NOTICE(LOADER, "Machine: %s", Ehdr_MachineToString(e_machine).c_str());
	LOG_NOTICE(LOADER, "Version: %d", e_version);
	LOG_NOTICE(LOADER, "Entry point address: 0x%x", e_entry);
	LOG_NOTICE(LOADER, "Program headers offset: 0x%08x", e_phoff);
	LOG_NOTICE(LOADER, "Section headers offset: 0x%08x", e_shoff);
	LOG_NOTICE(LOADER, "Flags: 0x%x", e_flags);
	LOG_NOTICE(LOADER, "Size of this header: %d", e_ehsize);
	LOG_NOTICE(LOADER, "Size of program headers: %d", e_phentsize);
	LOG_NOTICE(LOADER, "Number of program headers: %d", e_phnum);
	LOG_NOTICE(LOADER, "Size of section headers: %d", e_shentsize);
	LOG_NOTICE(LOADER, "Number of section headers: %d", e_shnum);
	LOG_NOTICE(LOADER, "Section header string table index: %d", e_shstrndx);
#endif
}

void Elf32_Ehdr::Load(vfsStream& f)
{
	e_magic  = Read32(f);
	e_class  = Read8(f);
	e_data   = Read8(f);
	e_curver = Read8(f);
	e_os_abi = Read8(f);

	if(IsLittleEndian())
	{
		e_abi_ver   = Read64LE(f);
		e_type      = Read16LE(f);
		e_machine   = Read16LE(f);
		e_version   = Read32LE(f);
		e_entry     = Read32LE(f);
		e_phoff     = Read32LE(f);
		e_shoff     = Read32LE(f);
		e_flags     = Read32LE(f);
		e_ehsize    = Read16LE(f);
		e_phentsize = Read16LE(f);
		e_phnum     = Read16LE(f);
		e_shentsize = Read16LE(f);
		e_shnum     = Read16LE(f);
		e_shstrndx  = Read16LE(f);
	}
	else
	{
		e_abi_ver   = Read64(f);
		e_type      = Read16(f);
		e_machine   = Read16(f);
		e_version   = Read32(f);
		e_entry     = Read32(f);
		e_phoff     = Read32(f);
		e_shoff     = Read32(f);
		e_flags     = Read32(f);
		e_ehsize    = Read16(f);
		e_phentsize = Read16(f);
		e_phnum     = Read16(f);
		e_shentsize = Read16(f);
		e_shnum     = Read16(f);
		e_shstrndx  = Read16(f);
	}
}

void Elf32_Desc::Load(vfsStream& f)
{
	revision = Read32(f);
	ls_size = Read32(f);
	stack_size = Read32(f);
	flags = Read32(f);
}

void Elf32_Desc::LoadLE(vfsStream& f)
{
	revision = Read32LE(f);
	ls_size = Read32LE(f);
	stack_size = Read32LE(f);
	flags = Read32LE(f);
}

void Elf32_Note::Load(vfsStream& f)
{
	namesz = Read32(f);
	descsz = Read32(f);
	type = Read32(f);
	f.Read(name, 8);

	if (descsz == 32)
	{
		f.Read(desc_text, descsz);
	}
	else
	{
		desc.Load(f);
	}
}

void Elf32_Note::LoadLE(vfsStream& f)
{
	namesz = Read32LE(f);
	descsz = Read32LE(f);
	type = Read32LE(f);
	f.Read(name, 8);

	if (descsz == 32)
	{
		f.Read(desc_text, descsz);
	}
	else
	{
		desc.Load(f);
	}
}

void Elf32_Shdr::Load(vfsStream& f)
{
	sh_name = Read32(f);
	sh_type = Read32(f);
	sh_flags = Read32(f);
	sh_addr = Read32(f);
	sh_offset = Read32(f);
	sh_size = Read32(f);
	sh_link = Read32(f);
	sh_info = Read32(f);
	sh_addralign = Read32(f);
	sh_entsize = Read32(f);
}

void Elf32_Shdr::LoadLE(vfsStream& f)
{
	sh_name = Read32LE(f);
	sh_type = Read32LE(f);
	sh_flags = Read32LE(f);
	sh_addr = Read32LE(f);
	sh_offset = Read32LE(f);
	sh_size = Read32LE(f);
	sh_link = Read32LE(f);
	sh_info = Read32LE(f);
	sh_addralign = Read32LE(f);
	sh_entsize = Read32LE(f);
}

void Elf32_Shdr::Show()
{
#ifdef LOADER_DEBUG
	LOG_NOTICE(LOADER, "Name offset: 0x%x", sh_name);
	LOG_NOTICE(LOADER, "Type: 0x%d", sh_type);
	LOG_NOTICE(LOADER, "Addr: 0x%x", sh_addr);
	LOG_NOTICE(LOADER, "Offset: 0x%x", sh_offset);
	LOG_NOTICE(LOADER, "Size: 0x%x", sh_size);
	LOG_NOTICE(LOADER, "EntSize: %d", sh_entsize);
	LOG_NOTICE(LOADER, "Flags: 0x%x", sh_flags);
	LOG_NOTICE(LOADER, "Link: 0x%x", sh_link);
	LOG_NOTICE(LOADER, "Info: %d", sh_info);
	LOG_NOTICE(LOADER, "Address align: 0x%x", sh_addralign);
#endif
}

void Elf32_Phdr::Load(vfsStream& f)
{
	p_type = Read32(f);
	p_offset = Read32(f);
	p_vaddr = Read32(f);
	p_paddr = Read32(f);
	p_filesz = Read32(f);
	p_memsz = Read32(f);
	p_flags = Read32(f);
	p_align = Read32(f);
}

void Elf32_Phdr::LoadLE(vfsStream& f)
{
	p_type = Read32LE(f);
	p_offset = Read32LE(f);
	p_vaddr = Read32LE(f);
	p_paddr = Read32LE(f);
	p_filesz = Read32LE(f);
	p_memsz = Read32LE(f);
	p_flags = Read32LE(f);
	p_align = Read32LE(f);
}

void Elf32_Phdr::Show()
{
#ifdef LOADER_DEBUG
	LOG_NOTICE(LOADER, "Type: %s", Phdr_TypeToString(p_type).c_str());
	LOG_NOTICE(LOADER, "Offset: 0x%08x", p_offset);
	LOG_NOTICE(LOADER, "Virtual address: 0x%08x", p_vaddr);
	LOG_NOTICE(LOADER, "Physical address: 0x%08x", p_paddr);
	LOG_NOTICE(LOADER, "File size: 0x%08x", p_filesz);
	LOG_NOTICE(LOADER, "Memory size: 0x%08x", p_memsz);
	LOG_NOTICE(LOADER, "Flags: %s", Phdr_FlagsToString(p_flags).c_str());
	LOG_NOTICE(LOADER, "Align: 0x%x", p_align);
#endif
}

void WriteEhdr(rFile& f, Elf32_Ehdr& ehdr)
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

void WritePhdr(rFile& f, Elf32_Phdr& phdr)
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

void WriteShdr(rFile& f, Elf32_Shdr& shdr)
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
		LOG_WARNING(LOADER, "ELF32 LE");

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
		LOG_ERROR(LOADER, "Unknown elf32 machine: 0x%x", ehdr.e_machine);
		return false;
	}

	entry = ehdr.GetEntry();
	if(entry == 0)
	{
		LOG_ERROR(LOADER, "elf32 error: entry is null!");
		return false;
	}

	return true;
}

bool ELF32Loader::LoadPhdrInfo()
{
	if(ehdr.e_phoff == 0 && ehdr.e_phnum)
	{
		LOG_ERROR(LOADER, "LoadPhdr32 error: Program header offset is null!");
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

	if(entry & 0x1)
	{
		//entry is physical, convert to virtual

		entry &= ~0x1;

		for(size_t i=0; i<phdr_arr.size(); ++i)
		{
			if(phdr_arr[i].p_paddr >= entry && entry < phdr_arr[i].p_paddr + phdr_arr[i].p_memsz)
			{
				entry += phdr_arr[i].p_vaddr;
				LOG_WARNING(LOADER, "virtual entry = 0x%x", entry);
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
		LOG_WARNING(LOADER, "LoadShdr32 error: shstrndx too big!");
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
	LOG_NOTICE(LOADER, "");
	ehdr.Show();
	LOG_NOTICE(LOADER, "");
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
				LOG_WARNING
				(
					LOADER, 
					"LoadPhdr32 different load addrs: paddr=0x%8.8x, vaddr=0x%8.8x", 
					phdr_arr[i].p_paddr, phdr_arr[i].p_vaddr
				);
			}

			switch(machine)
			{
			case MACHINE_SPU: break;
			case MACHINE_MIPS: Memory.PSP.RAM.AllocFixed(phdr_arr[i].p_vaddr + offset, phdr_arr[i].p_memsz); break;
			case MACHINE_ARM: Memory.PSV.RAM.AllocFixed(phdr_arr[i].p_vaddr + offset, phdr_arr[i].p_memsz); break;

			default:
				continue;
			}

			elf32_f.Seek(phdr_arr[i].p_offset);
			elf32_f.Read(vm::get_ptr<void>(phdr_arr[i].p_vaddr + offset), phdr_arr[i].p_filesz);
		}
		else if(phdr_arr[i].p_type == 0x00000004)
		{
			elf32_f.Seek(phdr_arr[i].p_offset);
			Elf32_Note note;
			if(ehdr.IsLittleEndian()) note.LoadLE(elf32_f);
			else note.Load(elf32_f);

			if(note.type != 1)
			{
				LOG_ERROR(LOADER, "ELF32: Bad NOTE type (%d)", note.type);
				break;
			}

			if(note.namesz != sizeof(note.name))
			{
				LOG_ERROR(LOADER, "ELF32: Bad NOTE namesz (%d)", note.namesz);
				break;
			}

			if(note.descsz != sizeof(note.desc) && note.descsz != 32)
			{
				LOG_ERROR(LOADER, "ELF32: Bad NOTE descsz (%d)", note.descsz);
				break;
			}

			//if(note.desc.flags)
			//{
			//	LOG_ERROR(LOADER, "ELF32: Bad NOTE flags (0x%x)", note.desc.flags);
			//	break;
			//}

			if(note.descsz == sizeof(note.desc))
			{
				LOG_WARNING(LOADER, "name = %s", std::string((const char *)note.name, 8).c_str());
				LOG_WARNING(LOADER, "ls_size = %d", note.desc.ls_size);
				LOG_WARNING(LOADER, "stack_size = %d", note.desc.stack_size);
			}
			else
			{
				LOG_WARNING(LOADER, "desc = '%s'", std::string(note.desc_text, 32).c_str());
			}
		}
#ifdef LOADER_DEBUG
		LOG_NOTICE(LOADER, "");
#endif
	}

	return true;
}

bool ELF32Loader::LoadShdrData(u64 offset)
{
	u32 fnid_addr = 0;

	for(u32 i=0; i<shdr_arr.size(); ++i)
	{
		Elf32_Shdr& shdr = shdr_arr[i];

#ifdef LOADER_DEBUG
		if(i < shdr_name_arr.size()) LOG_NOTICE(LOADER, "Name: %s", shdr_name_arr[i].c_str());
		shdr.Show();
		LOG_NOTICE(LOADER, "");
#endif
		if((shdr.sh_type == SHT_RELA) || (shdr.sh_type == SHT_REL))
		{
			LOG_ERROR(LOADER, "ELF32 ERROR: Relocation");
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

		// probably should be in LoadPhdrData()
		if (machine == MACHINE_ARM && !strcmp(shdr_name_arr[i].c_str(), ".sceFNID.rodata"))
		{
			fnid_addr = shdr.sh_addr;
		}
		else if (machine == MACHINE_ARM && !strcmp(shdr_name_arr[i].c_str(), ".sceFStub.rodata"))
		{
			list_known_psv_modules();

			auto fnid = vm::psv::ptr<const u32>::make(fnid_addr);
			auto fstub = vm::psv::ptr<const u32>::make(shdr.sh_addr);

			for (u32 j = 0; j < shdr.sh_size / 4; j++)
			{
				u32 nid = fnid[j];
				u32 addr = fstub[j];

				if (auto func = get_psv_func_by_nid(nid))
				{
					LOG_NOTICE(LOADER, "Imported function 0x%x (addr=0x%x)", nid, addr);

					// writing Thumb code (temporarily, because it should be ARM)
					vm::psv::write16(addr + 0, 0xf870); // HACK (special instruction that calls HLE function
					vm::psv::write16(addr + 2, (u16)get_psv_func_index(func));
					vm::psv::write16(addr + 4, 0x4770); // BX LR
					vm::psv::write16(addr + 6, 0); // null
				}
				else
				{
					LOG_ERROR(LOADER, "Unimplemented function 0x%x (addr=0x%x)", nid, addr);

					// writing Thumb code (temporarily - it shouldn't be written in this case)
					vm::psv::write16(addr + 0, 0xf06f); // MVN r0,#0x0
					vm::psv::write16(addr + 2, 0x0000);
					vm::psv::write16(addr + 4, 0x4770); // BX LR
					vm::psv::write16(addr + 6, 0); // null
				}
			}
		}
		else if (machine == MACHINE_ARM && !strcmp(shdr_name_arr[i].c_str(), ".sceRefs.rodata"))
		{
			// basically unknown struct

			struct reloc_info
			{
				u32 code; // 0xff
				u32 data; // address that will be written
				u32 code1; // 0x2f
				u32 addr1; // address of movw r12,# instruction to be replaced
				u32 code2; // 0x30
				u32 addr2; // address of movt r12,# instruction to be replaced
				u32 code3; // 0
			};

			auto rel = vm::psv::ptr<const reloc_info>::make(shdr.sh_addr);

			for (u32 j = 0; j < shdr.sh_size / sizeof(reloc_info); j++)
			{
				if (rel[j].code == 0xff && rel[j].code1 == 0x2f && rel[j].code2 == 0x30 && rel[j].code3 == 0)
				{
					const u32 data = rel[j].data;
					vm::psv::write16(rel[j].addr1 + 0, 0xf240 | (data & 0x800) >> 1 | (data & 0xf000) >> 12); // MOVW
					vm::psv::write16(rel[j].addr1 + 2, 0x0c00 | (data & 0x700) << 4 | (data & 0xff));
					vm::psv::write16(rel[j].addr2 + 0, 0xf2c0 | (data & 0x8000000) >> 17 | (data & 0xf0000000) >> 28); // MOVT
					vm::psv::write16(rel[j].addr2 + 2, 0x0c00 | (data & 0x7000000) >> 12 | (data & 0xff0000) >> 16);
				}
				else
				{
					LOG_ERROR(LOADER, "sceRefs: unknown code found (code=0x%x, code1=0x%x, code2=0x%x, code3=0x%x)", rel[j].code, rel[j].code1, rel[j].code2, rel[j].code3);
				}
			}
		}
	}

	//TODO
	return true;
}
